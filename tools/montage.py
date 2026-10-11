import numpy as np
import cv2

H, W = 512, 640
reps = [('A 262', 'fdump/f_262.raw'), ('B 268', 'fdump/f_268.raw'),
        ('C 340', 'fdump/f_340.raw'), ('D 346', 'fdump/f_346.raw'),
        ('black 280', 'fdump/f_280.raw')]
tiles = []
for name, fp in reps:
    img = np.fromfile(fp, dtype=np.uint8).reshape(H, W)
    lo, hi = np.percentile(img, [1, 99])
    hi = max(hi, lo + 1)
    for rot, tag in [(cv2.ROTATE_90_CLOCKWISE, name)]:
        v = np.clip((img.astype(np.float32) - lo) * 255 / (hi - lo), 0, 255).astype(np.uint8)
        v = cv2.rotate(v, rot)
        v = cv2.cvtColor(v, cv2.COLOR_GRAY2BGR)
        cv2.putText(v, tag, (5, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
        tiles.append(v)
big = np.hstack(tiles[:5]) if all(t.shape == tiles[0].shape for t in tiles) else None
if big is not None:
    cv2.imwrite('cam_montage.png', big)
    print('saved cam_montage.png', big.shape)
else:
    for i, t in enumerate(tiles):
        cv2.imwrite('cam_%d.png' % i, t)
    print('shapes differ, saved individually')
