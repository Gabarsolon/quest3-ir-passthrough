# Firmware analysis: the six cameras, from the OTA

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

## Bottom line

The firmware doesn't reveal a hidden door. It confirms the lock:

- The IR-capable sensors are real (OG01A1B, OV7251) and the headset reads them continuously for tracking.
- They are owned by the XR camera HAL / tracking pipeline below Android, with CV-specific tuning, and are deliberately not exposed as Android cameras.
- The only cameras an app can open are the two IMX471 color sensors, which can't see IR.

So there is no app-level route to IR night vision. Reaching the mono frames would mean operating inside Meta's XR camera HAL (`libxrcamerahal.so` / `libmonocameracontrol.so`) on a modified device — a privileged, system-level surface, not an API. That is the realistic boundary, and it matches what every external report has found.

## If someone wanted to go further (not done here)

The honest next step would be static analysis of `libxrcamerahal.so` and `libmonocameracontrol.so` to see how the mono streams are published internally and whether any system service re-exposes them. That is reverse engineering of a privileged component on your own device; it would not produce a portable app and is out of scope for this repo.
