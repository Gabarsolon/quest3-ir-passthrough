"""Map RGB sensor pixels into IR using factory Fisheye62 camera calibration.

Uses the six radial/two tangential model documented by Meta's Project Aria:
https://facebookresearch.github.io/projectaria_tools/docs/tech_insights/camera_intrinsic_models
Rotation-only ray transfer preserves alignment at distance. Camera translation
and scene depth remain the compositor's RGB assumptions.
"""
import numpy as np


def camera_pose_parameters(calibration):
    """RGB camera-from-device quaternion/translation for the remapped IR view."""
    cameras = {str(c['Id']): c for c in calibration['CameraCalibration']}
    result = []
    for camera_id in ['4', '5']:
        pose = np.asarray(cameras[camera_id]['DeviceFromCamera']).reshape(4, 4)
        rotation = pose[:3, :3].T
        # Stable matrix to xyzw quaternion conversion, including 180 degrees.
        trace = np.trace(rotation)
        if trace > 0:
            s = np.sqrt(trace + 1) * 2
            q = [(rotation[2, 1]-rotation[1, 2])/s,
                 (rotation[0, 2]-rotation[2, 0])/s,
                 (rotation[1, 0]-rotation[0, 1])/s, s/4]
        else:
            i = int(np.argmax(np.diag(rotation)))
            j, k = (i+1) % 3, (i+2) % 3
            s = np.sqrt(1+rotation[i, i]-rotation[j, j]-rotation[k, k])*2
            q = [0., 0., 0., (rotation[k, j]-rotation[j, k])/s]
            q[i] = s/4
            q[j] = (rotation[j, i]+rotation[i, j])/s
            q[k] = (rotation[k, i]+rotation[i, k])/s
        result.append(q + (-rotation @ pose[:3, 3]).tolist())
    return result


def distort(x, y, coefficients):
    p, q = coefficients[6:]
    radius2 = x*x + y*y
    return (x + p*(2*x*x + radius2) + 2*q*x*y,
            y + q*(2*y*y + radius2) + 2*p*x*y)


def unproject(camera, u, v):
    f, cx, cy = camera['Projection']['Coefficients']
    k = np.asarray(camera['Distortion']['Coefficients'])
    target_x, target_y = (u-cx)/f, (v-cy)/f
    x, y = target_x.copy(), target_y.copy()
    for _ in range(8):
        dx, dy = distort(x, y, k)
        p, q = k[6:]
        a, b, d = 1+6*p*x+2*q*y, 2*(q*x+p*y), 1+6*q*y+2*p*x
        ex, ey = target_x-dx, target_y-dy
        determinant = a*d-b*b
        x += (d*ex-b*ey)/determinant
        y += (a*ey-b*ex)/determinant
    radius = np.hypot(x, y)
    # Bracket the physically meaningful forward hemisphere, avoiding unstable
    # Newton iterations on the high-order radial polynomial near the corners.
    low, high = np.zeros_like(radius), np.full_like(radius, np.pi/2)
    for _ in range(45):
        theta = (low+high)*0.5
        polynomial = np.ones_like(theta)
        for i in range(6):
            polynomial += k[i]*theta**(2*i+2)
        projected = theta*polynomial
        low = np.where(projected < radius, theta, low)
        high = np.where(projected >= radius, theta, high)
    theta = (low+high)*0.5
    scale = np.sin(theta)/np.maximum(radius, 1e-12)
    return np.stack((x*scale, y*scale, np.cos(theta)), axis=-1)


def project(camera, rays):
    k = np.asarray(camera['Distortion']['Coefficients'])
    radius = np.hypot(rays[..., 0], rays[..., 1])
    theta = np.arctan2(radius, rays[..., 2])
    polynomial = np.ones_like(theta)
    for i in range(6):
        polynomial += k[i]*theta**(2*i+2)
    scale = theta*polynomial/np.maximum(radius, 1e-12)
    x, y = distort(rays[..., 0]*scale, rays[..., 1]*scale, k)
    f, cx, cy = camera['Projection']['Coefficients']
    return np.stack((f*x+cx, f*y+cy), axis=-1)


def make_maps(calibration, width=640, height=512):
    cameras = {str(c['Id']): c for c in calibration['CameraCalibration']}
    maps = []
    for ir_id, rgb_id in [('0', '4'), ('1', '5')]:
        ir, rgb = cameras[ir_id], cameras[rgb_id]
        for camera in [ir, rgb]:
            if (camera['Distortion']['Model'] != 'Fisheye62'
                    or camera['Projection']['Model'] != 'PinholeSymmetric'):
                raise ValueError('Unsupported factory calibration model')
        if ir['ImageSize'] != [1280, 1024] or rgb['ImageSize'] != [2328, 1748]:
            raise ValueError('Unexpected calibrated sensor dimensions')
        cols, rows = np.meshgrid((np.arange(width)+0.5)/width,
                                 (np.arange(height)+0.5)/height)
        rays = unproject(rgb, cols*2328, rows*1748)
        ir_pose = np.asarray(ir['DeviceFromCamera']).reshape(4, 4)
        rgb_pose = np.asarray(rgb['DeviceFromCamera']).reshape(4, 4)
        rotation = ir_pose[:3, :3].T @ rgb_pose[:3, :3]
        transformed = rays @ rotation.T
        pixels = project(ir, transformed)
        uv = pixels / np.asarray(ir['ImageSize'])
        if not np.isfinite(uv).all():
            raise ValueError('Non-finite calibration map')
        uv[transformed[..., 2] <= 0] = -1
        # Half precision quantizes normalized UVs to a noticeable fraction of
        # an IR pixel, creating a grid when this map is enlarged for display.
        maps.append(uv.astype('<f4').tobytes())
    return maps
