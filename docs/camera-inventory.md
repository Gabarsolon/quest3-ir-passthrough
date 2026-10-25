# Camera inventory of a stock Quest 3

Scope: public API observations from the tested Horizon OS v79 build
51154110174900520, rather than a claim about every later SDK/OS version.
The rooted [system IR prototype](system-passthrough.md) runs below this API.

Collected over `adb` from a Quest 3 on Horizon OS v79 (build `51154110174900520`). Commands used:

```sh
adb shell dumpsys media.camera
adb shell pm list permissions -g
adb shell pm list features
adb shell pm list packages
```

## Android camera devices

`dumpsys media.camera` reports **3 camera devices**. All three are *virtual* HAL devices (`device@1.1/virtual/<id>`) belonging to "virtual device 0".

| Camera id | `lens.facing` | Pixel array | Stream configs | Hardware level | Notes |
|---|---|---|---|---|---|
| `50` | BACK | 1280 × 960 | YUV_420_888 @ 30 fps | EXTERNAL | Left RGB passthrough camera |
| `51` | BACK | 1280 × 960 | YUV_420_888 @ 30 fps | EXTERNAL | Right RGB passthrough camera |
| `1` | FRONT | 1024 × 768 | RGBA_8888 1024×768 / 1280×720 @ 30 fps | EXTERNAL | Virtual avatar camera; also carries a `camera_name` tag |

All three report only the `BACKWARD_COMPATIBLE` capability, so there are no RAW, depth or monochrome capabilities.

Every device carries a Meta vendor tag named `camera_source` (tag id `-2147463936`). This is the tag the Passthrough Camera API uses to identify passthrough cameras (source `0`).

### What is missing

The Quest 3 has more image sensors than this: the monochrome tracking cameras and the depth sensor. **None of them is an Android camera device.** The tracking stack consumes them internally, so `CameraManager.getCameraIdList()` cannot list them and the Passthrough Camera API cannot open them.

Those monochrome tracking cameras are the IR-sensitive ones, which is why an IR illuminator improves *tracking* in the dark on a Quest 3 but adds nothing to the *passthrough view*.

## Camera permissions

Camera-related permissions defined on the device:

| Permission | Purpose |
|---|---|
| `android.permission.CAMERA` | Standard Android camera permission, needed for any camera |
| `horizonos.permission.HEADSET_CAMERA` | Runtime permission that unlocks the passthrough cameras (`50`/`51`) |
| `horizonos.permission.AVATAR_CAMERA` | Virtual avatar camera |
| `horizonos.permission.CODEC_AVATAR_CAMERA` | Codec avatar camera |
| `horizonos.permission.USB_CAMERA` | External USB cameras |
| `horizonos.permission.APP_DEFINED_CAMERA` | App-defined virtual cameras |
| `horizonos.permission.CREATE_VIRTUAL_CAMERA` | Creating virtual cameras |
| `com.oculus.permission.ACCESS_CAMERA_DATA` | System-level camera data, not grantable to normal apps |
| `android.permission.SYSTEM_CAMERA` | System cameras, not grantable to normal apps |

No permission names or implies the tracking cameras.

## Feature flags

Camera and passthrough features from `pm list features`:

```
android.hardware.camera.any
android.hardware.camera.external
android.hardware.camera.front
oculus.software.camera.line_frequency_detection
oculus.software.handtracking
oculus.software.passthrough_capture
oculus.software.passthrough_display_sync
oculus.software.passthrough_white_led_signal
```

## Conclusion

From an app's point of view, the Quest 3 has two RGB cameras plus a virtual avatar camera. The IR-sensitive sensors exist in the hardware, but apps cannot reach them.
