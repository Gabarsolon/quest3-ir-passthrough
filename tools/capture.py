import subprocess, time
import numpy as np
import cv2

SERIAL = '2G0YC5ZF9Q01J0'
W, H = 640, 512
N = W * H


def read_exact(f, n):
    b = b''
    while len(b) < n:
        c = f.read(n - len(b))
        if not c:
            return b
        b += c
    return b


cmd = ['adb', '-s', SERIAL, 'exec-out', 'while true; do cat /data/local/tmp/ir.raw; done']
p = subprocess.Popen(cmd, stdout=subprocess.PIPE, bufsize=0)
frames = []
t0 = time.time()
for i in range(60):
    buf = read_exact(p.stdout, N)
    img = np.frombuffer(buf, dtype=np.uint8)
    frames.append(img.copy())
    print('%2d mean=%6.1f sd=%6.1f' % (i, img.mean(), img.std()))
dt = time.time() - t0
print('60 frames in %.2fs = %.1f fps' % (dt, 60 / dt))

tiles = []
for i in range(0, 60, 4):
    img = frames[i].reshape(H, W).astype(np.float32)
    lo, hi = np.percentile(img, [1, 99])
    hi = max(hi, lo + 1)
    v = np.clip((img - lo) * 255 / (hi - lo), 0, 255).astype(np.uint8)
    v = cv2.cvtColor(v, cv2.COLOR_GRAY2BGR)
    cv2.putText(v, '#%d' % i, (5, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
    tiles.append(v)
rows = [np.hstack(tiles[j:j + 4]) for j in range(0, len(tiles), 4)]
cv2.imwrite('cap_montage.png', np.vstack(rows))
print('montage saved')
