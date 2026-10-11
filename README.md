# quest3-ir-passthrough

Research notes on one question: **can a Meta Quest 3 show passthrough from its infrared-sensitive tracking cameras, so the room is visible at night under an IR illuminator?**

The PSVR2 does this out of the box: its passthrough comes from monochrome cameras that see near-infrared, so an IR lamp lights the room for the headset while it stays dark to your eyes. The Quest 3 doesn't. Its passthrough comes from the two RGB cameras, so at night the view is black even with an IR illuminator running.

This repo is documentation only. It collects what the headset exposes, what the official APIs allow, and what other people have tried.

## Short answer (as of October 2026, Horizon OS v79)

**No supported route exists.**

- Android apps on the Quest 3 can open exactly three camera devices: the left and right **RGB** passthrough cameras (ids `50`, `51`) and a virtual avatar camera (id `1`). See [docs/camera-inventory.md](docs/camera-inventory.md).
- The monochrome tracking cameras, which do see IR, are **not Android camera devices**. The tracking stack uses them internally and no public API returns their pixels.
- Meta's **Passthrough Camera API** is a thin layer over Android Camera2 and covers only the RGB cameras. See [docs/passthrough-camera-api.md](docs/passthrough-camera-api.md).
- Screen-capture tricks (media projection) return the composited RGB passthrough view, so they hit the same limit.
- On the Quest 3, an IR illuminator **helps tracking** in the dark, but the passthrough view stays black.

Analysis of the v79 system image confirms this at the hardware level: the Quest 3 has **six cameras** — two Sony IMX471 **color** sensors (the passthrough cameras, exposed to apps) and four OmniVision **monochrome, IR-sensitive** sensors (OG01A1B + OV7251) used for tracking. The IR-capable cameras exist and work in the dark, but they are owned by Meta's XR camera HAL / tracking pipeline below Android and are never published as Android cameras. See [docs/firmware-analysis.md](docs/firmware-analysis.md).

Getting at the tracking-camera pixels would mean working inside that HAL on a modified device. That is invasive, firmware-dependent and outside this repo's scope.

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
| [evidence/](evidence/) | Trimmed `dumpsys` output the inventory is based on |

## Test device

| | |
|---|---|
| Headset | Meta Quest 3 (`eureka`) |
| Horizon OS | v79, build `UP1A.231005.007.A1` / `51154110174900520` |
| Android base | 14, security patch 2025-01-06 |
| Access | stock, locked bootloader (`verifiedbootstate=green`), `adb` shell user only |

All data was collected with read-only `adb` commands (`dumpsys`, `getprop`, `pm list`). Nothing on the headset was modified.

## License

MIT. See [LICENSE](LICENSE).
