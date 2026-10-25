"""Image-space pose math and native read-only checks; no headset required.

Run with Python + NumPy + host Clang: python -B tests/test_image_pose.py
The native harness compiles production quaternion/buffer-reader functions.
"""
import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

import numpy as np

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / 'tools'))
from ir_calibration import image_warp_parameters, make_maps, project, unproject


def rotation(axis, angle):
    axis = np.asarray(axis, dtype=float)
    axis /= np.linalg.norm(axis)
    x, y, z = axis
    cross = np.array([[0, -z, y], [z, 0, -x], [-y, x, 0]])
    return np.eye(3) + np.sin(angle)*cross + (1-np.cos(angle))*(cross @ cross)


def fixture():
    cameras = []
    for i in [0, 1, 4, 5]:
        pose = np.eye(4)
        pose[:3, :3] = rotation([1, 2, 3], (i-2)*0.025)
        ir = i < 2
        cameras.append(dict(Id=str(i), DeviceFromCamera=pose.ravel().tolist(),
            ImageSize=[1280, 1024] if ir else [2328, 1748],
            Projection=dict(Model='PinholeSymmetric',
                Coefficients=[450, 640, 512] if ir else [1300, 1164, 874]),
            Distortion=dict(Model='Fisheye62',
                Coefficients=[0.01, -0.002, 0.0003, -0.00002, 0.000001,
                              -0.00000001, 0.0002, -0.0003])))
    return dict(CameraCalibration=cameras)


class ImagePoseTests(unittest.TestCase):
    def test_identity_reproduces_static_calibration(self):
        calibration = fixture()
        parameters = image_warp_parameters(calibration)
        old = make_maps(calibration, 53, 41)
        slopes = make_maps(calibration, 53, 41, rays_only=True)
        cameras = calibration['CameraCalibration']
        for eye in range(2):
            xy = np.frombuffer(slopes[eye], '<f4').reshape(41, 53, 2)
            rays = np.concatenate((xy, np.ones((41, 53, 1))), axis=-1)
            matrix = np.array(parameters[eye]['rotation']).reshape(3, 3).T
            transformed = rays @ matrix.T
            uv = project(cameras[eye], transformed) / [1280, 1024]
            uv[transformed[..., 2] <= 0] = -1
            np.testing.assert_allclose(uv, np.frombuffer(old[eye], '<f4').reshape(41, 53, 2), atol=1e-7)

    def test_shader_projection_polynomial(self):
        camera = fixture()['CameraCalibration'][0]
        rays = np.random.default_rng(1).uniform(-1, 1, (1000, 3)).astype('f4')
        rays[:, 2] = np.abs(rays[:, 2]) + 0.1
        radius = np.linalg.norm(rays[:, :2], axis=-1)
        theta = np.arctan2(radius, rays[:, 2])
        theta2 = theta*theta
        k = np.array(camera['Distortion']['Coefficients'], dtype='f4')
        polynomial = 1+theta2*(k[0]+theta2*(k[1]+theta2*(k[2]+theta2*(k[3]+theta2*(k[4]+theta2*k[5])))))
        p = rays[:, :2]*(theta*polynomial/np.maximum(radius, 1e-12))[:, None]
        radius2 = np.sum(p*p, axis=-1)[:, None]
        d = p+k[6:]*(2*p*p+radius2)+2*k[6:][::-1]*(p[:, 0]*p[:, 1])[:, None]
        shader = (450*d+[640, 512])/[1280, 1024]
        np.testing.assert_allclose(shader, project(camera, rays)/[1280, 1024], atol=3e-7)

    def test_native_reads_and_quaternion_delta(self):
        source = (REPO / 'tools/system_passthrough.c').read_text()

        def function(name):
            start = source.rfind('\n', 0, source.index(name)) + 1
            opening = source.index('{', start)
            depth, end = 1, opening+1
            while depth:
                depth += (source[end] == '{') - (source[end] == '}')
                end += 1
            return source[start:end]+'\n'

        # On image mode's path the buffer accessor has no write operation.
        harness = source[:source.index('static Q now_ms')]
        for name in ['integer(', 'fail(', 'multiply_quaternion(', 'image_pose_delta(', 'read_native_pose(']:
            harness += function(name)
        harness += r'''
int _fltused=0;
Control control; PoseRecord pose_records[1];
static unsigned char memory_a[4096], memory_b[4096], saved[4096];
static unsigned char *mapped=memory_a;
static int generic=73, capacity=4096;
void glGetIntegerv(U key,int *out){*out=generic;}
void glGetIntegeri_v(U key,U index,int *out){*out=7;}
void glGetInteger64i_v(U key,U index,long long *out){*out=key==0x8a29?128:3184;}
void glBindBuffer(U target,U name){generic=name;}
void glGetBufferPointerv(U target,U key,void **out){*out=mapped;}
void glGetBufferParameteriv(U target,U key,int *out){*out=key==0x911f?0xc2:capacity;}
static float absval(float x){return x<0?-x:x;}
static void set_rows(unsigned char *p,float x,float w){
  int i; for(i=0;i<8;i++) *(float*)(p+128+2912+(i/4)*32+(i%4)*4)=i%4==0?x:(i%4==3?w:0);
}
#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
void calculate_delta(float *ir,float *native,float *out){
  Source *s=&control.sources[1];int i;float bottom[4];
  for(i=0;i<4;i++){s->camera_pose[i]=ir[i];s->native_pose[i]=s->native_pose[4+i]=native[i];}
  image_pose_delta(s,out,bottom);
}
#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
int run_tests(void){
  Source *s=&control.sources[0]; int i;
  float top[4],bottom[4]; control.enabled=1;
  for(i=0;i<4096;i++) memory_a[i]=(unsigned char)(i*17);
  set_rows(memory_a,0,1);
  for(i=0;i<4096;i++) saved[i]=memory_a[i];
  if(!read_native_pose(s)||generic!=73||!s->native_valid||!s->remap_dirty||s->native_changes!=1)return 10;
  s->remap_dirty=0;
  if(!read_native_pose(s)||s->remap_dirty||s->native_changes!=1)return 11;
  for(i=0;i<4096;i++)if(saved[i]!=memory_a[i])return 12;
  // Same native name, different current allocation: must query mapping again.
  mapped=memory_b; set_rows(mapped,0.6f,0.8f);
  if(!read_native_pose(s)||!s->remap_dirty||s->native_changes!=2)return 13;
  s->camera_pose[0]=0.6f;s->camera_pose[3]=0.8f;
  image_pose_delta(s,top,bottom);
  if(absval(top[0])+absval(top[1])+absval(top[2])+absval(top[3]-1)>0.00001f)return 14;
  // Opposite quaternion signs encode identical rotations; interpolation stays valid.
  for(i=4;i<8;i++)s->native_pose[i]=-s->native_pose[i];
  image_pose_delta(s,top,bottom);
  for(i=0;i<4;i++)if(absval(top[i]-bottom[i])>0.00001f)return 15;
  s->camera_pose[0]=0;s->camera_pose[3]=1;
  image_pose_delta(s,top,bottom);
  if(absval(top[0]+0.6f)+absval(top[3]-0.8f)>0.00001f)return 16;
  // Reject invalid bounds / non-unit rows without changing native memory.
  for(i=0;i<4096;i++)saved[i]=memory_b[i];
  capacity=2048;
  if(read_native_pose(s)||control.enabled||generic!=73)return 17;
  for(i=0;i<4096;i++)if(saved[i]!=memory_b[i])return 18;
  capacity=4096; control.enabled=1;set_rows(mapped,2,2);
  if(read_native_pose(s)||control.enabled||generic!=73)return 19;
  return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='quest-ir-image-test-') as tmp:
            folder = Path(tmp)
            (folder / 'gum').mkdir()
            (folder / 'gum/guminterceptor.h').write_text('typedef struct _GumInvocationContext GumInvocationContext;\n')
            c_file = folder / 'harness.c'
            c_file.write_text(harness)
            compiler = shutil.which('clang') or r'C:\Program Files\LLVM\bin\clang.exe'
            library = folder / ('test.dll' if os.name == 'nt' else 'test.so')
            command = [compiler, '-shared', '-nostdlib', '-fno-builtin', '-O2', '-I', tmp, str(c_file), '-o', str(library)]
            if os.name == 'nt':
                command[1:1] = ['-target', 'x86_64-pc-windows-msvc', '-fuse-ld=lld']
                command.append('-Wl,/noentry')
            else:
                command.append('-fPIC')
            subprocess.run(command, check=True)
            loaded = ctypes.CDLL(str(library))
            try:
                self.assertEqual(loaded.run_tests(), 0)
                pointer = ctypes.POINTER(ctypes.c_float)
                loaded.calculate_delta.argtypes = [pointer, pointer, pointer]
                random = np.random.default_rng(9)
                for _ in range(50):
                    axes = random.normal(size=(2, 3))
                    axes /= np.linalg.norm(axes, axis=1)[:, None]
                    angles = random.uniform(-2, 2, 2)
                    ir, native = [np.r_[axis*np.sin(angle/2), np.cos(angle/2)].astype('f4')
                                  for axis, angle in zip(axes, angles)]
                    delta = np.zeros(4, dtype='f4')
                    loaded.calculate_delta(*(q.ctypes.data_as(pointer) for q in [ir, native, delta]))
                    x, y, z, w = delta.astype(float)
                    cross = np.array([[0, -z, y], [z, 0, -x], [-y, x, 0]])
                    actual = np.eye(3)+2*w*cross+2*cross@cross
                    ir_rotation, native_rotation = [rotation(axis, angle) for axis, angle in zip(axes, angles)]
                    # Independent matrix result: transfer native capture rays into IR capture coordinates.
                    np.testing.assert_allclose(actual, ir_rotation @ native_rotation.T, atol=6e-7)
            finally:
                if os.name == 'nt':
                    ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(loaded._handle))


if __name__ == '__main__':
    unittest.main()
