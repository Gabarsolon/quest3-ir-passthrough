# Reverse engineering `libxrcamerahal.so`

Follow-up to [firmware-analysis.md](firmware-analysis.md). After confirming the six sensors, the next question was whether the XR camera HAL — the component that actually drives all six — has any switch that would expose the IR (monochrome) cameras to apps, or route their frames into passthrough. It does not. This is what the code shows.

## Method

- `vendor/lib64/libxrcamerahal.so` and `libmonocameracontrol.so` (AArch64, Horizon OS v79) were pulled from `vendor.img`.
- Both are **not stripped**: full C++ symbols are present (547 and 307 exported symbols). Symbols and `.rodata` strings dumped with pyelftools; key functions decompiled with Ghidra 12.1 via pyghidra.
- Evidence: [../evidence/libxrcamerahal/](../evidence/libxrcamerahal/) (`key-symbols.txt`, `camera-strings.txt`, `decompiled-key-functions.c`).

## What the HAL is

`libxrcamerahal.so` implements `CameraHal::XrCameraHal`, built on the standard Android camera HAL3 structures (`camera3_stream`, `camera_metadata`, `camera3_capture_request`). It is the layer that drives **all** of the headset's cameras — color and mono alike — and feeds frames to the tracking/CV stack. Source paths baked into the binary confirm the origin: `vendor/meta/libs/xrthal-qc/camera/xrcamerahal/src/...`.

Each camera is described by an `XrCameraConfiguration`. Streams are typed by an `OVR::Peripherals::Config::Models::StreamType` enum (19 values), and cameras by a `CameraHal::CameraType` enum (22 values). The HAL already knows, per camera, whether it is color or mono.

### Color vs mono is an internal flag

`XrCameraLogical::onIsBayerCamera(cameraId)` returns a single byte at offset `0x40` of that camera's `XrCameraConfiguration`:

```c
// onIsBayerCamera: look up the camera's config, return config->isBayer (offset 0x40)
return *(undefined1 *)(*(long *)plVar3[3] + 0x40);
```

"Bayer" = color (the IMX471 passthrough sensors); not-Bayer = mono (OG01A1B / OV7251). That flag gates the whole pipeline: `overrideBayerFeatureSetFromSystemProperties` returns immediately if the flag is 0, and `overrideMonoCameraConfigurationFromSystemProperties` runs its body only if it is 0. The HAL treats color and mono as two separate pipelines from the start.

## The system-property knobs (and why they don't help)

The most promising-looking symbols were the `override*FromSystemProperties` functions — the HAL does read system properties at configure time. But every one of them only **tunes the processing of a camera that is already in its configured role**. None adds a camera, exposes mono to Android, or sends mono frames to passthrough.

`overrideMonoCameraConfigurationFromSystemProperties` reads (via generated `oculus::sysprop::XrtHalQcProperties` accessors):

| Knob | Effect on the **mono/tracking** pipeline |
|---|---|
| `enable_fastpath` | RAW10 fast path; mutually exclusive with gamma/secure/ultralite |
| `mono_use_gamma` | replaces 10-bit mono with 8-bit mono (gamma compression) |
| `enable_secure` | routes mono through a secure buffer (flag `0x4000`) |
| `enable_ultralite` | IFE-lite path |

It also calls `OVR::OS::isWorldTrackingCamera(cameraId)` — i.e. these only apply to the tracking cameras, confirming the mono sensors stay bound to tracking.

`overrideBayerFeatureSetFromSystemProperties` reads knobs for the **color/passthrough** pipeline: `teton_output_yuv`, `rgb_enable_3a`, `teton_enable_gme_mctf` (temporal filter), `teton_enable_fastai`, `oculus_camera_enable_rgb_crop`, `isp_foveation_mode`, plus `persist.vendor.camera.rgb.min_sensitivity`. These change YUV output, 3A, denoise, foveation and cropping of the color stream — nothing about IR.

So the knobs that exist are developer/tuning switches for each pipeline's image processing. There is **no property that turns a mono sensor into an app-visible camera, and none that feeds mono frames into the passthrough/color path.** The split is structural, decided by the per-camera Bayer flag, long before any property is read.

## Where app visibility is actually decided

`XrCameraHal::GetSupportedCameraIds()` simply copies a vector that was built at init from the camera configuration:

```c
// GetSupportedCameraIds: return a copy of the pre-built id vector at this+0x90
__src = *(void **)(lVar1 + 0x30);          // configuration_.ids.begin()
__n   = *(long *)(lVar1 + 0x38) - __src;   // size
memcpy(operator_new(__n), __src, __n);     // return a copy
```

That is the HAL's **internal** list of every camera it manages (all six). It is not what Android apps see. The two RGB cameras that reach the Android camera service (ids `50`/`51`, tagged with the `camera_source` vendor tag — see [camera-inventory.md](camera-inventory.md)) are published by a Meta camera provider/service that sits *above* this HAL, in the system image. That upper layer is where the decision "apps get the two color cameras and nothing else" is enforced; `libxrcamerahal` below it faithfully drives all six for Meta's own tracking and passthrough use.

## Conclusion

The HAL confirms the lock rather than opening it:

- Color and mono are separate pipelines, split by a per-camera Bayer flag.
- The mono (IR) sensors are bound to world/controller/hand tracking (`isWorldTrackingCamera`), with only image-processing knobs exposed (bit depth, gamma, secure, fastpath, foveation…).
- Nothing in the HAL exposes mono as an Android camera or merges it into passthrough.
- The app-visible camera set is fixed above this HAL and contains only the two color sensors.

Turning the IR cameras into a usable night-vision view would therefore mean injecting into Meta's XR camera HAL on a modified/rooted headset **and** building a separate display path for the result — not a property toggle, not an app, and not portable. That is the real boundary, and it is consistent with the firmware, the `adb` camera inventory, and every external report. This repo stops here.
