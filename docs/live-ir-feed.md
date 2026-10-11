# Live IR feed: reading the tracking cameras and viewing them

Follow-on to [ir-camera-mapping.md](ir-camera-mapping.md). Instead of one-off dumps, this
runs a daemon that continuously republishes the *newest* frame of one IR camera to a file,
streams that file to a PC over `adb`, and shows it live. Collected on the test Quest 3 (v79,
build `51154110174900520`) with the temporary root shell from
[root-access.md](root-access.md). **Read-only** on the headset: nothing was modified, all
state is volatile and disappears on reboot.

## Result

The IR tracking cameras can be shown live at ~45-60 fps. Under an IR illuminator the room is
clearly visible. See the caveats at the end for the conditions the cameras need to stream.

## How the pixels are reached

The monochrome cameras are not Android camera devices; their frames are dma-bufs held by the
sensors service. `irfeed` maps every 640x512 (`327680`-byte) dma-buf of the target pid
persistently, using only raw syscalls:

1. `pidfd_open(434)` on the service pid, then `pidfd_getfd(438)` per fd (the `/proc/<pid>/fd/N`
   dma-buf path is synthetic and cannot be `open()`ed), falling back to `openat` where allowed.
2. `mmap` each buffer once, `MAP_SHARED`, `PROT_READ`.
3. Each tick: sample every 64th byte of every buffer and compare with the previous tick's
   sample. A buffer whose sample **changed** just received a new frame. Publish the brightest
   changed buffer (freshest frame) to `<out>.tmp` then `renameat` it onto `<out>` atomically.
4. **Hold-last-good**: if the freshly-arrived frame is a dark idle burst (brightness under half
   of the recent good level), skip the write and keep the previous frame, so the feed does not
   flash black. After ~1.6 s of genuine darkness it accepts the dark frame.

`irfeed.c` is freestanding aarch64 (no libc):

```sh
clang -target aarch64-linux-gnu -nostdlib -static -fno-stack-protector -fno-builtin -O2 \
      -Wl,-e,_start -Wl,--build-id=none -o irfeed irfeed.c
```

`run_irfeed.sh` either enumerates the pid's `dmabuf` fds by size (`stat -c %s`), or, if
`/data/local/tmp/ir_fds.txt` exists, uses that explicit fd list (much faster start-up, and how
a single camera is locked - see below). `start_irfeed.sh` kills any previous feeder and launches
`irfeed` detached with `setsid`.

## The cameras

The 26 x 640x512 buffers are **two cameras** (left and right eye), each appearing at **two
exposure levels** (A/B for left, C/D for right). Dumping all 26 at once and clustering the
downsampled frames by correlation gives:

| Group | mean (lit) | buffers | fds | camera |
|---|---|---|---|---|
| A | ~95.5 | 7 | 262 274 286 304 310 322 328 | left eye |
| B | ~100 | 3 | 268 292 316 | left eye |
| C | ~102.8 | 6 | 340 364 376 388 394 406 | right eye |
| D | ~107 | 3 | 346 370 412 | right eye |

plus 7 buffers that read ~4 (an off camera): 280 298 334 352 358 382 400.

Within a group every buffer is identical (corr = 1.000). A<->B and C<->D correlate 1.000
(same viewpoint, different exposure); left<->right correlate ~0.85 (stereo parallax). Because
of that parallax, sampling *all* buffers makes the picture **jump between the two eyes**
("shake"). Locking to one eye fixes it:

```sh
adb shell '/data/local/tmp/su -c "cat > /data/local/tmp/ir_fds.txt"' <<'EOF'
262 268 274 286 292 304 310 316 322 328
EOF
```

(`tools/ir_fds.txt` holds that left-eye list.) The sensor is also mounted rotated 90 degrees,
so the display is rotated.

## Viewing on the PC

`tools/viewer.py` streams the file over `adb exec-out` (`while true; do cat ...; done`, no
per-frame sleep) and shows it with OpenCV:

- `read_exact` keeps the stream aligned to 327680-byte frames,
- percentiles auto-gain is **smoothed (EMA)** so dark frames are not amplified into noise,
- `r` cycles rotation (default 90 cw), `ESC` quits.

```sh
python tools/viewer.py            # live window
python tools/viewer.py --save     # write one frame to ir_view.png
```

`tools/capture.py` dumps a burst of consecutive frames with per-frame mean/sd for diagnostics;
`tools/analyze.py` / `group_sim.py` cluster dumped buffers; `tools/montage.py` builds the
side-by-side camera comparison.

## Caveats

- **The cameras only stream while the headset is awake and tracking.** On standby the driver
  stops and releases the sensors, so the dma-bufs freeze. An IR illuminator lights the room for
  the cameras, not for the RGB passthrough.
- **fd numbers are per-process and change across reboots.** Re-derive the groups (size
  histogram + correlation cluster) after a restart; the invariant is the two 640x512 exposures
  per eye. An explicit `ir_fds.txt` must be regenerated if the fds move.
- Reading is by root only; this never touches the tracking pipeline's own consumption of the
  frames.
