# quest3-ir-passthrough

Replace Quest 3 system RGB passthrough with a live stereo feed from its
infrared-sensitive tracking cameras. With an IR illuminator, this provides a
monochrome view of the room in the dark.

This is a **firmware-specific prototype using temporary root and RAM hooks**.
It runs in the shared system compositor; Virtual Desktop is not required.
Normal system passthrough has been visually tested. Other applications and
rendering layouts still need individual validation.

**Known app-transition failure:** opening a flat Browser window has dropped the
IR view; switching to Virtual Desktop produced a green right eye, diagonal
rectangles/flicker, a freeze and an automatic headset reboot. The Home result
does not establish app-transition stability. See the
[crash investigation](docs/app-transition-crash.md). IR is currently left off
while collecting post-reboot diagnostics.

## Current result

The user confirmed upright stereo IR, corrected proportions, aligned hands,
and removal of the recurring rectangles/tearing in the latest tested setup.
They called the current configuration the best version so far. Earlier motion
lag reports remain relevant; this is not a claim of perfect motion in every scene.

| Stage | Verified size / rate on the tested headset |
|---|---|
| Factory calibration for the front IR pair | 1280×1024 coordinates |
| Image input actually read by this project | **640×512 per eye, about 60 distinct images/sec** |
| Owned, calibrated remap texture | 1280×960 per eye; larger output does not add captured detail |
| Preferred tested display timing | **120 Hz**, selected with `--display-refresh 120` |

The OG01A1B sensor is rated for 1280×1024/120 FPS, but that is a hardware
specification, not an unlocked Quest mode. The live HAL and the shipped sensor
module currently expose only a 640×513 RAW mode for this pair; the exported
image is 640×512. **No existing full-resolution IR stream was found.** See the
[resolution investigation](docs/ir-resolution.md) and
[recorded evidence](evidence/ir-resolution-2026-10-25.json).

The default `--pose-method image` corrects our IR pixels while leaving native
camera/menu uniform buffers untouched. Each eye holds one IR capture across its
rendered frame. Legacy `direct` and `shadow` methods produced recurring artifacts
and remain diagnostic options.

## Run

Requirements: Quest 3 on build **51154110174900520** (Horizon OS v79), active
temporary root, an authorized ADB connection, Python with NumPy and
`frida==17.17.0`, and LLVM clang. Temporary root must already be established;
the passthrough launcher does not run the exploit.

From the repository in PowerShell, launch the preferred tested configuration:

```powershell
python .\tools\system_passthrough.py --display-refresh 120
```

Enable normal system passthrough. `IR ACTIVE` means the compositor has sampled
the replacement textures. Keep the command running. Ctrl+C restores RGB;
a second terminal can inspect or stop it:

```powershell
python .\tools\system_passthrough.py --status
python .\tools\system_passthrough.py --stop
```

Wireless ADB uses `QUEST3_ADB_SERIAL`, then the host file
`%LOCALAPPDATA%/quest3-ir/adb-target.txt`, then the USB serial.
Use `--serial IP:PORT` to select a target explicitly. Disconnecting USB alone
does not remove detached temporary root; the controller still needs its wireless
connection for IR. Reboot or power loss clears root and RAM hooks.

## Options and cleanup

- **Display:** `--display-refresh 120` improves frame spacing for the 60 Hz IR
  input. It changes display timing, not camera capture rate. Omit it to preserve
  native display settings. The exact prior override is restored on normal stop
  or startup failure; after a forced host termination or lost connection, reboot
  clears the nonpersistent override.
- **Filtering:** linear is the default, with AA off. `--filter cubic` and
  `--filter sgsr` are optional for each run. The previously accepted enhancement
  combination was `--filter sgsr --sgsr-sharpness 1.0 --aa edge`; it has not been
  revalidated with every later rendering change. Options are not remembered.
- **Resolution:** `--output-size` changes the remap texture, not the camera
  input. It cannot select a 1280×1024 capture stream.
- **RAM scope:** new headset binaries, frames, and logs stay in tmpfs under
  `/dev/quest3-ir-session`. No firmware flashing, bootloader unlock, persistent
  root, or boot/system/vendor/odm partition writes are performed. Existing
  `/data/local/tmp` root helpers survive reboot.

See [system passthrough](docs/system-passthrough.md) for recovery, timing
measurements, and the current geometry/translation limits.

## Documentation and evidence

| Document | Contents |
|---|---|
| [System passthrough](docs/system-passthrough.md) | Maintained launcher, rendering methods, options, cleanup |
| [IR resolution](docs/ir-resolution.md) | Sensor specification versus live modes; full-resolution investigation |
| [Session notes](docs/session-2026-10-25.md) | Chronological experiments, failures, measurements, visual feedback |
| [IR camera mapping](docs/ir-camera-mapping.md) | Physical eye mapping and DMA buffers; fd/PID examples are historical |
| [Live IR feed](docs/live-ir-feed.md) | Earlier PC reader/viewer work |
| [Root access](docs/root-access.md) | Temporary-root work log and recovery context |
| [Firmware analysis](docs/firmware-analysis.md) | Camera modules identified in the v79 image |
| [HAL analysis](docs/reverse-engineering.md) | Internal camera pipeline and public API boundary |
| [Camera provider](docs/camera-provider.md) | App-visible virtual camera registration on the tested build |
| [Camera inventory](docs/camera-inventory.md), [PCA](docs/passthrough-camera-api.md) | Public RGB camera API observations on v79 |
| [Earlier VD prototype](docs/vd-layer-injection.md), [prior art](docs/prior-art.md) | Historical approaches and background |

Stock Camera2/Passthrough Camera API access on the tested v79 headset exposes
RGB cameras, not the tracking cameras. This project reaches IR with temporary
root below that API. Older documents are research snapshots; the README and
system-passthrough guide describe the maintained implementation.

## Publishing

To push committed changes and create a new public prerelease from your normal
PowerShell terminal, choose an unused tag, for example:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\publish_github.ps1 -Tag v0.1.0-preview.2
```

`v0.1.0-preview.1` describes an older commit. The script does not replace a tag
that points elsewhere. `-DryRun` checks local prerequisites without network
access; publishing offers GitHub login if needed. It does not access the headset.

## License

MIT. See [LICENSE](LICENSE). The optional SGSR shader has its own
[upstream attribution and license](tools/vendor/sgsr1/README.md).
