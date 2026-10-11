import subprocess, sys, time
import numpy as np
import cv2

SERIAL = '2G0YC5ZF9Q01J0'
W, H = 640, 512
N = W * H
REMOTE = '/data/local/tmp/ir.raw'

ROT = [
    ('none', None),
    ('cw', cv2.ROTATE_90_CLOCKWISE),
    ('ccw', cv2.ROTATE_90_COUNTERCLOCKWISE),
    ('180', cv2.ROTATE_180),
]


def stream_cmd():
    return ['adb', '-s', SERIAL, 'exec-out',
            'while true; do cat %s; done' % REMOTE]


def read_exact(f, n):
    b = b''
    while len(b) < n:
        c = f.read(n - len(b))
        if not c:
            return b
        b += c
    return b


def main():
    save = '--save' in sys.argv
    p = subprocess.Popen(stream_cmd(), stdout=subprocess.PIPE, bufsize=0)
    frames = 0
    t0 = time.time()
    ri = 1
    glo = ghi = None
    while True:
        buf = read_exact(p.stdout, N)
        if len(buf) < N:
            print('stream ended')
            break
        img = np.frombuffer(buf, dtype=np.uint8).reshape(H, W)
        lo, hi = np.percentile(img, [1, 99])
        if hi <= lo:
            hi = lo + 1
        if glo is None:
            glo, ghi = lo, hi
        else:
            glo = 0.85 * glo + 0.15 * lo
            ghi = 0.85 * ghi + 0.15 * hi
        if ghi <= glo:
            ghi = glo + 1
        vis = np.clip((img.astype(np.float32) - glo) * (255.0 / (ghi - glo)), 0, 255).astype(np.uint8)
        name, code = ROT[ri]
        if code is not None:
            vis = cv2.rotate(vis, code)
        vis = cv2.resize(vis, (vis.shape[1] * 2, vis.shape[0] * 2), interpolation=cv2.INTER_NEAREST)
        frames += 1
        if save:
            cv2.imwrite('ir_view.png', vis)
            print('saved ir_view.png  mean=%.1f' % img.mean())
            break
        fps = frames / max(1e-3, time.time() - t0)
        cv2.putText(vis, 'IR %dx%d  mean=%.1f  %.1ffps  rot=%s  (r=rotate ESC=quit)'
                    % (W, H, img.mean(), fps, name),
                    (10, 24), cv2.FONT_HERSHEY_SIMPLEX, 0.6, 255, 2)
        cv2.imshow('Quest3 IR feed', vis)
        k = cv2.waitKey(1) & 0xFF
        if k == 27:
            break
        if k == ord('r'):
            ri = (ri + 1) % len(ROT)
    cv2.destroyAllWindows()


main()
