# IR tracking-camera frame-buffer mapping

Work log: identifying which dma-bufs held by the tracking service correspond to which physical IR camera. Collected on the test Quest 3 (v79, build `51154110174900520`) with the temporary root shell from [root-access.md](root-access.md). **Read-only**: nothing on the headset was modified; the mapping is volatile and disappears on reboot.

## Background

The monochrome tracking cameras are not Android camera devices (see [camera-inventory.md](camera-inventory.md)). With root, their output buffers are reachable as dma-bufs held by the sensors service:

| Process | PID | Role |
|---|---|---|
| `vendor.oculus.hardware.sensors@1.0-service` | 852 | hosts `libxrcamerahal.so` |
| `trackingservice` | 1082 | consumer; holds the camera dma-buf fds |

`/proc/<pid>/fd/N` for a dma-buf cannot be `open()`ed (synthetic path). We dup the fd with `pidfd_open(434)` + `pidfd_getfd(438)`, then `mmap` it — done by a freestanding aarch64 helper (`dumpdbg`, and `dumpm` for a fast in-process mean). `stat -L -c '%s %i' /proc/1082/fd/N` gives each buffer's size and inode.

## Buffer inventory (PID 1082)

| Size (bytes) | Dimensions | Count (distinct inodes) | Camera |
|---|---|---|---|
| 327680 | 640 × 512 | 26 = 2 × 13 | the two world-tracking **IR** cameras (left + right eye) |
| 307200 | 640 × 480 | 26 = 2 × 13 | the **hand-tracking / proximity** pair (leftmost + rightmost) |

The 640×512 set splits into two **contiguous inode ranges of 13 buffers each** — one per camera.

## Cover test

Procedure: with the headset awake and streaming, cover one lens at a time and read each fd's frame mean (5 samples/fd). A covered lens drives that camera's fds to ~0 (the rest stay bright).

Result:

| Camera | dma-buf fds (640×512) | buffers | mean, right lens covered |
|---|---|---|---|
| **Left eye** | 258, 635, 426, 428, 267, 536, 538, 647, 974, 277, 978, 281, 550 | 13 | 114 (bright) |
| **Right eye** | 552, 450, 452, 665, 293, 778, 889, 566, 464, 303, 305, 790, 309 | 13 | 11 (black) |

Earlier run, left lens covered: the left-eye set dropped to ~50 while the right-eye set stayed bright — consistent.

## Caveats

- **Streaming requires the headset to be awake.** On standby the driver stops and releases every sensor (`CAM_STOP_DEV`/`CAM_RELEASE_DEV` for `og01a1b 0x7750` and `ov7251 0xd`), so the dma-bufs freeze at their last frame. Covering the proximity sensor is not enough on its own — the headset must actually be streaming.
- **fd numbers are per-process and change across reboots.** Re-map after every restart using the size histogram plus the cover test. The invariant is the two 13-buffer 640×512 inode ranges.
- These are still *not* Android cameras: this only reads the tracking pipeline's output from userspace.

See [live-ir-feed.md](live-ir-feed.md) for the live feed, the freshness-based `irfeed` daemon, and the per-eye buffer groups (A/B = left eye, C/D = right eye) at pid 852.
