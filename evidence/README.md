# Evidence

Trimmed output from a stock Quest 3 (Horizon OS v79, build `51154110174900520`), collected with read-only `adb` commands. Device serials are left out.

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
