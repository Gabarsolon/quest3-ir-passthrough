# Evidence

Observations from Quest 3 build `51154110174900520` (Horizon OS v79). Earlier
inventory uses read-only ADB; later camera metadata inspection uses temporary
root and bounded read-only native probes. Device serials and camera images are
not included in the resolution evidence.

## IR resolution audit

[ir-resolution-2026-10-25.json](ir-resolution-2026-10-25.json) records live RAW
capabilities, configured streams, and host-decoded sensor-module dimensions and
hashes. No full-resolution IR stream was found. See
[the investigation](../docs/ir-resolution.md). The proprietary module binaries
are not committed; the offline inspector is in
[tools/inspect_sensor_module.py](../tools/inspect_sensor_module.py).

## `vendor/` — from the OTA system image

Files pulled from the `vendor` partition of the v79 OTA (see [../docs/firmware-analysis.md](../docs/firmware-analysis.md)):

- `vendor-camera-files.txt` — the camera/tracking-related file listing from `vendor.img`, including the six `com.qti.sensormodule.*` sensor modules (IMX471 ×2 color, OG01A1B ×2 + OV7251 ×2 monochrome/IR).
- `cvaectuning_slam.json`, `cvaectuning_ht.json` — Meta's CV auto-exposure tuning for the SLAM and hand-tracking (monochrome) camera paths.

The partition images and `payload.bin` themselves are not committed (they're large and are Meta's firmware); `.gitignore` excludes them.

## `adb shell dumpsys media.camera` (excerpt)

```
Number of camera devices visible to API1: 3
Device 0 maps to "1"
Device 1 maps to "50"
Device 2 maps to "51"

== Camera HAL device device@1.1/virtual/50 (v3.1) static information: ==
  Resource cost: 100
  Conflicting devices: None
  Facing: Back
  android.lens.facing (80005): byte[1]                   [BACK ]
  android.request.availableCapabilities (c000c): byte[1] [BACKWARD_COMPATIBLE ]
  android.sensor.info.pixelArraySize (f0006): int32[2]   [1280 960 ]
  android.info.supportedHardwareLevel (150000): byte[1]  [EXTERNAL ]
  ... -2147463936 (camera_source) ...
  virtual_camera 50 belongs to virtual device 0
  SupportedStreamConfiguration{width: 1280, height: 960, pixelFormat: YUV_420_888, maxFps: 30}

== Camera HAL device device@1.1/virtual/51 (v3.1) static information: ==
  Resource cost: 100
  Conflicting devices: None
  Facing: Back
  android.lens.facing (80005): byte[1]                   [BACK ]
  android.request.availableCapabilities (c000c): byte[1] [BACKWARD_COMPATIBLE ]
  android.sensor.info.pixelArraySize (f0006): int32[2]   [1280 960 ]
  android.info.supportedHardwareLevel (150000): byte[1]  [EXTERNAL ]
  ... -2147463936 (camera_source) ...
  virtual_camera 51 belongs to virtual device 0
  SupportedStreamConfiguration{width: 1280, height: 960, pixelFormat: YUV_420_888, maxFps: 30}

== Camera HAL device device@1.1/virtual/1 (v3.1) static information: ==
  Resource cost: 100
  Conflicting devices: None
  Facing: Front
  android.lens.facing (80005): byte[1]                   [FRONT ]
  android.request.availableCapabilities (c000c): byte[1] [BACKWARD_COMPATIBLE ]
  android.sensor.info.pixelArraySize (f0006): int32[2]   [1024 768 ]
  android.info.supportedHardwareLevel (150000): byte[1]  [EXTERNAL ]
  ... -2147463936 (camera_source) -2147463933 (camera_name) ...
  virtual_camera 1 belongs to virtual device 0
  SupportedStreamConfiguration{width: 1024, height: 768, pixelFormat: RGBA_8888, maxFps: 30}
  SupportedStreamConfiguration{width: 1280, height: 720, pixelFormat: RGBA_8888, maxFps: 30}
```

## `adb shell getprop` (build identity)

```
ro.product.model             Quest 3
ro.product.device            eureka
ro.build.version.release     14
ro.build.display.id          UP1A.231005.007.A1
ro.build.version.incremental 51154110174900520
ro.build.version.security_patch 2025-01-06
ro.vros.build.version        79
ro.boot.verifiedbootstate    green
ro.boot.flash.locked         1
ro.debuggable                0
ro.build.type                user
```
