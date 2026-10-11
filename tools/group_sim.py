import numpy as np
import cv2

H, W = 512, 640
groups = {
    'A': [262, 274, 286, 304, 310, 322, 328],
    'B': [268, 292, 316],
    'C': [340, 364, 376, 388, 394, 406],
    'D': [346, 370, 412],
}


def load(fd):
    img = np.fromfile('fdump/f_%d.raw' % fd, dtype=np.uint8).reshape(H, W)
    a = cv2.resize(img, (64, 51), interpolation=cv2.INTER_AREA).astype(np.float32).ravel()
    v = a - a.mean()
    return v / (np.linalg.norm(v) + 1e-6)


for g, fds in groups.items():
    va = np.mean([load(f) for f in fds], axis=0)
    groups[g] = va / np.linalg.norm(va)

names = list(groups)
print('group  ' + '  '.join('%6s' % n for n in names))
for a in names:
    row = []
    for b in names:
        row.append('%6.3f' % float(groups[a] @ groups[b]))
    print('%5s  ' % a + '  '.join(row))

# also per-group internal (are members identical?)
for g, fds in [('A', [262, 274, 286, 304, 310, 322, 328]), ('B', [268, 292, 316]),
               ('C', [340, 364, 376, 388, 394, 406]), ('D', [346, 370, 412])]:
    vs = [load(f) for f in fds]
    sims = [float(vs[i] @ vs[j]) for i in range(len(vs)) for j in range(i + 1, len(vs))]
    print('internal %s: min=%.3f max=%.3f' % (g, min(sims), max(sims)))
