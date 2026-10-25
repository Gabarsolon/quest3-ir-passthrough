# Firmware analysis: the six cameras, from the OTA

This is the v79 hardware/API investigation. The project now implements rooted,
RAM-only system IR passthrough; see [the maintained guide](system-passthrough.md).
The sensor specification and an available capture stream are different; see
[the resolution audit](ir-resolution.md).

This is what the Quest 3 system image says about its own cameras. It confirms, at the hardware level, why apps only ever see two RGB cameras.

## Source

Full OTA for build `51154110174900520` (Horizon OS v79), a standard Android A/B update:

- `payload.bin` is an `update_engine` payload (magic `CrAU`, version 2), **not** a raw image. `binwalk` cannot parse it; carving it yields noise because each partition is stored as chunked, compressed install operations.
- It was unpacked with a payload dumper (`payload_dumper`), which reads the protobuf manifest and writes one image per partition. The manifest lists **29 partitions**; the camera stack lives in **`vendor`** (ext4, 483 MB).
- `vendor.img` was read with a userspace ext4 library (no mounting).

## The sensor map

`/vendor/etc/camera/` names every camera sensor module on the headset:

| Module file | Sensor | CSI | Side | Type |
|---|---|---|---|---|
| `com.qti.sensormodule.cam00_none_eureka_og01a1b_left_csi4` | OmniVision **OG01A1B** | 4 | left | mono, global shutter, NIR-sensitive |
| `com.qti.sensormodule.cam01_none_eureka_og01a1b_right_csi1` | OmniVision **OG01A1B** | 1 | right | mono, global shutter, NIR-sensitive |
| `com.qti.sensormodule.cam02_liteon_eureka_ov7251_left_csi5` | OmniVision **OV7251** | 5 | left | mono VGA, global shutter, NIR-sensitive |
| `com.qti.sensormodule.cam03_liteon_eureka_ov7251_right_csi2` | OmniVision **OV7251** | 2 | right | mono VGA, global shutter, NIR-sensitive |
| `com.qti.sensormodule.cam04_none_eureka_imx471_left_csi0` | Sony **IMX471** | 0 | left | 4 MP RGB (color) |
| `com.qti.sensormodule.cam05_none_eureka_imx471_right_csi3` | Sony **IMX471** | 3 | right | 4 MP RGB (color) |

So the Quest 3 has **six head-mounted cameras in three stereo pairs**:

- **IMX471 ×2** — the 4 MP **color** cameras. These are the passthrough cameras, and the only ones published to Android (ids `50`/`51`, see [camera-inventory.md](camera-inventory.md)). Color sensors have an IR-cut response, so under an IR illuminator they record almost nothing.
- **OG01A1B ×2** — 1 MP global-shutter **monochrome** cameras for head/world tracking (SLAM).
- **OV7251 ×2** — VGA global-shutter **monochrome** cameras for controller and hand tracking.

The four monochrome sensors are the IR-sensitive ones. They are exactly what an IR illuminator lights up, and exactly what the PSVR2 uses for its passthrough. On the Quest 3 they exist, they work in the dark, but they are wired to the tracking system, not to passthrough.

## Where the mono cameras go

The mono sensors are driven and consumed by Meta's XR camera stack, not the Android camera provider:

- `/vendor/lib64/libmonocameracontrol.so` — control for the monochrome cameras.
- `/vendor/lib64/libxrcamerahal.so` — Meta's XR camera HAL.
- `/vendor/etc/camera/cvaectuning_slam.json` and `cvaectuning_ht.json` — computer-vision auto-exposure tuning for the **SLAM** and **hand-tracking** camera paths. (Saved under [evidence/vendor/](../evidence/vendor/).)
- The color path uses the standard Qualcomm CameraX stack (`/vendor/lib64/hw/camera.qcom.so`, `com.qti.chi.override.so`, the `libcamx*` libraries) plus Meta's `com.meta.node.*` processing nodes. This is the stack behind the IMX471 passthrough cameras exposed to Android.

The mono camera frames never reach Android's `CameraManager`. That is why no Camera2 id, and no Passthrough Camera API `camera_source` value, corresponds to them.

## Access boundary and implemented follow-up

The firmware lists real IR-sensitive cameras owned by the XR tracking stack.
They are not published through the stock Android camera provider on this build.
That explains why the public RGB camera API cannot select them.

The project subsequently read their exported DMA buffers with temporary root
and substituted owned IR textures in the system compositor. This route is
implemented in [system passthrough](system-passthrough.md); the earlier conclusion
that further work was out of scope no longer describes this repository.

The full-resolution follow-up found factory calibration at 1280x1024, but the
live front-IR RAW configuration and shipped module each provide a single
640x513 mode. The reader receives a 640x512 image. A 1280x1024 live IR stream
has not been accessed. See [the audit and evidence](ir-resolution.md).
