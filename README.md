# quest3-ir-passthrough

Replace Quest 3 system RGB passthrough with a live stereo feed from its
infrared-sensitive tracking cameras. With an IR illuminator, this provides a
monochrome view of the room in the dark.

This is a **firmware-specific prototype using temporary root and RAM hooks**.
It runs in the shared system compositor; Virtual Desktop is not required.
Normal system passthrough has been visually tested. Other applications and
rendering layouts still need individual validation.

**App compatibility is experimental.** An earlier app transition caused green
IR, rectangles and an automatic reboot. Per-context resources and protected IR
outputs now pass a short Browser-over-Virtual-Desktop visual test. A later
startup crash on Frida 17.17 prompted an isolated runtime upgrade. A two-minute
17.24.3 run completed without a compositor restart; visual confirmation of
repeated app switches is pending. See the
[crash investigation](docs/app-transition-crash.md).

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

The **Quest IR** headset app runs the controller locally after temporary root
is established. Download the APK from
[v0.2.0-preview.1](https://github.com/Gabarsolon/quest3-ir-passthrough/releases/tag/v0.2.0-preview.1),
then open **Quest IR** under Unknown Sources and select **Start IR**. See
[headset-only operation](docs/headset-controller.md) for controls, RAM scope,
and current validation limits. The user confirmed the corrected headset build
works; root still needs restoring after each reboot.
The instructions below describe the maintained laptop controller.

Requirements: Quest 3 on build **51154110174900520** (Horizon OS v79), active
temporary root, an authorized ADB connection, Python with NumPy, and LLVM clang.
Frida **17.23 or newer** is required for its ARM64 online-hook improvement;
the isolated setup below pins matching client/server **17.24.3**. Temporary root must already be established;
the passthrough launcher does not run the exploit.

Prepare the isolated runtime once in normal PowerShell. It downloads official
Frida packages to the laptop, verifies the server archive against the release's
SHA-256 digest, and leaves global Python and existing headset helpers unchanged:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\update_frida_runtime.ps1
```

Then launch from the repository:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\run_ir.ps1 --display-refresh 120
```

Add `--brightness 1.5` for a brighter IR image or `--brightness 0.7` for a dimmer
one. The default is `1.0`; accepted multipliers are 0 through 4. This adjusts the
IR pixels after normalization for this run, without changing camera exposure.
The wrapper stages its matching server in `/dev/quest3-ir-session/system` on
each boot as needed. It does not update the saved `/data/local/tmp` helper,
restart ADB, or rerun root.

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

If the saved wireless connection is unavailable, the IR launcher can use this
same Quest's connected USB serial. An explicit `--serial` selection is never
silently replaced. To restore temporary root over USB, use
`tools/root_ionstack.ps1`; after success it enables Wi-Fi and requests existing
paired Wireless debugging. Accept the network approval popup when shown.

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
PowerShell terminal, choose an unused tag and first write matching notes in
`docs/releases/<tag>.md`, for example:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\publish_github.ps1 -Tag v0.2.0-preview.2
```

The current published prototype is `v0.2.0-preview.1`; its
[release notes](docs/releases/v0.2.0-preview.1.md) describe the headset app and limits.
The script loads `docs/releases/<tag>.md` and does not replace a tag
that points elsewhere. `-DryRun` checks local prerequisites without network
access; publishing offers GitHub login if needed. It does not access the headset.

## License

MIT. See [LICENSE](LICENSE). The optional SGSR shader has its own
[upstream attribution and license](tools/vendor/sgsr1/README.md).
