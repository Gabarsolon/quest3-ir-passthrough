# quest3-ir-passthrough

Research notes on one question: **can a Meta Quest 3 show passthrough from its infrared-sensitive tracking cameras, so the room is visible at night under an IR illuminator?**

The PSVR2 does this out of the box: its passthrough comes from monochrome cameras that see near-infrared, so an IR lamp lights the room for the headset while it stays dark to your eyes. The Quest 3 doesn't. Its passthrough comes from the two RGB cameras, so at night the view is black even with an IR illuminator running.

This repo is documentation plus the small tools we used. It collects what the headset exposes, what the official APIs allow, what other people have tried, and the code that reads the tracking-camera frames and displays them live.

## RAM-only system replacement (rooted v79 prototype)

The current prototype replaces the system compositor's RGB passthrough camera
textures with live stereo IR. The user confirmed upright stereo IR, corrected proportions, and aligned hand
tracking in normal Quest passthrough. The current hands-mode pipeline has measured
about 60 changed IR images/sec per eye. Earlier pose-buffer methods initially
looked smooth but later produced recurring black rectangles in the room and menu.
The replacement `--pose-method image` is now the default: it corrects our IR
image while preserving native camera and menu poses. Holding each IR capture
across a whole eye/frame cleared the remaining tearing in the user's test
("seems perfect now"). Source resolution remains
640x512. Optional monochrome
Snapdragon GSR1 reconstruction improved edges while keeping motion smooth.
Linear filtering is the default, with extra anti-aliasing off. Select
`--filter cubic` or `--filter sgsr` for an individual run; the final tested
enhancement combination is `--filter sgsr --sgsr-sharpness 1.0 --aa edge`.
These options are not saved to the headset or remembered by later launches.
It uses temporary root and an in-memory hook; Virtual Desktop is
not required. Run it from PowerShell with the headset connected and temporary
root already active:

```powershell
python C:\Users\remus\quest3-ir-passthrough\tools\system_passthrough.py
```

Ctrl+C restores RGB. A second terminal can run the same command with `--stop` or
`--status`. New headset files stay in tmpfs. See
[system passthrough](docs/system-passthrough.md) for setup, validation, and current
geometry/app-coverage limits.

To push committed changes, make this repository public and publish the
`v0.1.0-preview.1` prerelease, run in a normal PowerShell terminal:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\publish_github.ps1
```

The script offers browser login if needed, verifies the remote history and tag,
and reuses an already published matching release. `-DryRun` checks the local
plan without network access. It does not access the headset.

## Public API background (unrooted headset)

**No supported route exists.**

- Android apps on the Quest 3 can open exactly three camera devices: the left and right **RGB** passthrough cameras (ids `50`, `51`) and a virtual avatar camera (id `1`). See [docs/camera-inventory.md](docs/camera-inventory.md).
- The monochrome tracking cameras, which do see IR, are **not Android camera devices**. The tracking stack uses them internally and no public API returns their pixels.
- Meta's **Passthrough Camera API** is a thin layer over Android Camera2 and covers only the RGB cameras. See [docs/passthrough-camera-api.md](docs/passthrough-camera-api.md).
- Screen-capture tricks (media projection) return the composited RGB passthrough view, so they hit the same limit.
- On the Quest 3, an IR illuminator **helps tracking** in the dark, but the passthrough view stays black.

Analysis of the v79 system image confirms this at the hardware level: the Quest 3 has **six cameras** — two Sony IMX471 **color** sensors (the passthrough cameras, exposed to apps) and four OmniVision **monochrome, IR-sensitive** sensors (OG01A1B + OV7251) used for tracking. The IR-capable cameras exist and work in the dark, but they are owned by Meta's XR camera HAL / tracking pipeline below Android and are never published as Android cameras. See [docs/firmware-analysis.md](docs/firmware-analysis.md).

The tools in this repository now read those buffers under temporary root and
perform the substitution in the shared compositor. Older research documents
describe the earlier public-API investigation; their statements that this work
stops at that boundary are historical.

## What would change the answer

- Meta exposes the tracking cameras through the Passthrough Camera API, for example as another `camera_source` value. Nothing announced so far.
- Meta adds a low-light or IR passthrough mode at the system level.
- A different headset whose passthrough cameras see IR. The PSVR2 (see PSVR2Toolkit Room View) and the Quest 1/2 work this way.

## Contents

| File | What it covers |
|---|---|
| [docs/camera-inventory.md](docs/camera-inventory.md) | Camera devices, permissions and features reported by a real Quest 3 over `adb` |
| [docs/firmware-analysis.md](docs/firmware-analysis.md) | The six physical camera sensors, read out of the v79 OTA system image |
| [docs/reverse-engineering.md](docs/reverse-engineering.md) | Decompiling `libxrcamerahal.so`: why no HAL switch exposes the IR cameras |
| [docs/camera-provider.md](docs/camera-provider.md) | The AOSP Virtual Camera gate: `cameraSource` has no IR option, end-to-end chain |
| [docs/passthrough-camera-api.md](docs/passthrough-camera-api.md) | How the official Passthrough Camera API works and why it can't give you IR |
| [docs/prior-art.md](docs/prior-art.md) | Who has tried this and what they found, with links |
| [docs/root-access.md](docs/root-access.md) | Work log: getting a temporary root shell (IonStack / CVE-2026-43499) to reach the tracking-camera HAL |
| [docs/system-passthrough.md](docs/system-passthrough.md) | RAM-only script replacing shared system passthrough camera textures with IR |
| [docs/ir-camera-mapping.md](docs/ir-camera-mapping.md) | Mapping the tracking service's dma-bufs to the physical IR cameras (left/right eye) |
| [docs/vd-layer-injection.md](docs/vd-layer-injection.md) | Work log: injecting an XR composition layer behind the Virtual Desktop image (route toward showing IR) |
| [docs/live-ir-feed.md](docs/live-ir-feed.md) | Work log: reading the tracking cameras live and viewing the feed on a PC (`irfeed` daemon + viewer) |
| [tools/](tools/) | The code: `irfeed.c` daemon, launcher scripts, PC viewer, and dump/cluster helpers |
| [evidence/](evidence/) | Trimmed `dumpsys` output the inventory is based on |

## Test device

| | |
|---|---|
| Headset | Meta Quest 3 (`eureka`) |
| Horizon OS | v79, build `UP1A.231005.007.A1` / `51154110174900520` |
| Android base | 14, security patch 2025-01-06 |
| Access | locked bootloader (`verifiedbootstate=green`), temporary root and RAM hooks |

The original inventory was read-only. Current prototypes use volatile root,
readers, and compositor hooks; firmware and system partitions remain unchanged.

## License

MIT. See [LICENSE](LICENSE).
