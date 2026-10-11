import glob, os
import numpy as np
import cv2

H, W = 512, 640
files = sorted(glob.glob('fdump/f_*.raw'))
fds, vecs, stats = [], [], []
for fp in files:
    fd = os.path.basename(fp)[2:-4]
    img = np.fromfile(fp, dtype=np.uint8)
    if img.size != H * W:
        print('skip', fd, img.size)
        continue
    a = cv2.resize(img.reshape(H, W), (40, 32), interpolation=cv2.INTER_AREA).astype(np.float32).ravel()
    fds.append(fd)
    stats.append((img.mean(), img.std()))
    v = a - a.mean()
    n = np.linalg.norm(v)
    vecs.append(v / (n if n > 1e-6 else 1))
V = np.array(vecs)

print('fd    mean   sd    | pairwise best match (corr)')
for i, fd in enumerate(fds):
    sims = V @ V[i]
    order = np.argsort(-sims)
    best = [fds[j] for j in order[1:4]]
    print('%4s %6.1f %6.1f  -> %s (%.3f) %s (%.3f) %s (%.3f)' % (
        fd, stats[i][0], stats[i][1],
        best[0], sims[order[1]], best[1], sims[order[2]], best[2], sims[order[3]]))

# kmeans k=2 on z-norm vectors
rng = np.random.default_rng(0)
best_labels, best_inertia = None, None
for _ in range(50):
    c = V[rng.choice(len(V), 2, replace=False)]
    for _ in range(100):
        d = ((V[:, None, :] - c[None, :, :]) ** 2).sum(2)
        lab = d.argmin(1)
        newc = np.array([V[lab == k].mean(0) if (lab == k).any() else c[k] for k in range(2)])
        if np.allclose(newc, c):
            break
        c = newc
    inertia = ((V - c[lab]) ** 2).sum()
    if best_inertia is None or inertia < best_inertia:
        best_inertia, best_labels = inertia, lab.copy()

print('\nkmeans k=2 groups:')
for k in range(2):
    print('group', k, [fds[i] for i in range(len(fds)) if best_labels[i] == k])
