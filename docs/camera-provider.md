# The camera provider: how app visibility is gated

Final layer. [reverse-engineering.md](reverse-engineering.md) showed the mono (IR) frames are locked below Android. This shows the layer *above* — the one that decides which cameras an app can see and open — and confirms it has no concept of the IR cameras at all.

## It's AOSP's Virtual Camera, reused by Meta

The cameras an app sees (`device@1.1/virtual/50`, `51`, `1` — the `virtual` provider in the `adb` inventory) are published by `/system/bin/virtual_camera`, started by `/system/etc/init/virtual_camera.hal.rc` as the stock AOSP service. The binary implements `android.hardware.camera.provider.ICameraProvider` and the class `android::companion::virtualcamera::VirtualCameraService` — this is AOSP's **Virtual Camera HAL** (the VirtualDeviceManager camera framework added in Android 14), not a Meta-custom provider.

Meta reused it and added:
- a **`cameraSource`** field to `VirtualCameraConfiguration`, and
- the vendor tags `com.meta.extra_metadata` / `camera_source` / position (`aPosition`) that the Passthrough Camera API reads.

## The camera-source enum and the permission gate

`VirtualCameraService::registerCamera` decides what a caller may publish. Decompiled (`evidence/virtual_camera/decompiled-gate-functions.c`), the gate is:

```c
// cameraSource lives at VirtualCameraConfiguration+0x80
if (*(uint *)(config + 0x80) < 2) {            // PASSTHROUGH(0) or AVATAR(1)
    if (!checkCallingPermission("android.permission.CREATE_VIRTUAL_DEVICE")) {
        // "caller (pid, uid) doesn't hold %s permission which is necessary to
        //  create cameraSource PASSTHROUGH or AVATAR. Consider using cameraSource APP_DEFINED"
        return EX_SECURITY;
    }
}
...
if (*(int *)(config + 0x80) == 2) {            // APP_DEFINED
    if (*(uint *)(config + 0x44) != 2)         // lensFacing must be External
        return "APP_DEFINED cameras must set LensFacing to External";
}
```

So the enum is:

| `cameraSource` | Meaning | Who may register it |
|---|---|---|
| `0` PASSTHROUGH | the headset's color passthrough cameras (ids 50/51) | only a holder of `android.permission.CREATE_VIRTUAL_DEVICE` |
| `1` AVATAR | the avatar/selfie virtual camera (id 1) | only a holder of `CREATE_VIRTUAL_DEVICE` |
| `2` APP_DEFINED | a camera whose frames the **app itself** supplies | any app; must set LensFacing = External, may not set a cameraId |

Every service method — `registerCamera`, `unregisterCamera`, `getCameraId`, `getSessionLogId` — first checks `android.permission.CREATE_VIRTUAL_DEVICE` **or** `horizonos.permission.CREATE_VIRTUAL_CAMERA`. `validateCameraId` additionally rejects a caller-chosen camera id unless it holds `CREATE_VIRTUAL_DEVICE` ("Camera producer is not allowed to set cameraId").

A registered camera is a **producer→consumer** pipe: the registrant supplies frames as GL textures (the service requires `GL_OES_EGL_image_external`, `..._essl3`, `GL_EXT_YUV_target`). The passthrough cameras exist because a privileged Meta component (holding `CREATE_VIRTUAL_DEVICE` — the CMS headset service, `system_ext/.../libcmsservice-headset.so` + `CMSHeadset.apk`) registered them as `cameraSource = PASSTHROUGH` and feeds them color frames from the passthrough pipeline. An ordinary app then *opens* them (read side), gated separately by `horizonos.permission.HEADSET_CAMERA` (see [passthrough-camera-api.md](passthrough-camera-api.md)).

## Why this closes the IR question

The enum that defines every camera an app can see has exactly three values — **PASSTHROUGH, AVATAR, APP_DEFINED**. There is no "tracking", "mono", or "IR" source. The framework that publishes cameras to the Android camera service literally cannot represent the IR tracking cameras.

The only two things an app can do here are both dead ends for night vision:

1. **Register an APP_DEFINED camera** — but then the *app* is the producer; it supplies the frames. It cannot name a real sensor, and it has no way to get mono/IR pixels to feed in (those are locked below Android, per [reverse-engineering.md](reverse-engineering.md)).
2. **Open the existing PASSTHROUGH cameras** — color only, can't see IR.

Registering a *new* PASSTHROUGH-type camera needs `CREATE_VIRTUAL_DEVICE` (a privileged system permission) **and** you'd still have to be the one feeding it frames — which again requires the mono frames you can't obtain. Every path loops back to the same locked door.

## Full chain, end to end

1. **Sensors** — 2× IMX471 color + 4× mono IR (OG01A1B, OV7251). ([firmware-analysis.md](firmware-analysis.md))
2. **XR camera HAL** (`libxrcamerahal.so`) drives all six; color vs mono is a fixed per-camera flag; mono is bound to tracking; system-property knobs only tune processing, none exposes mono. ([reverse-engineering.md](reverse-engineering.md))
3. **Virtual Camera provider** (`/system/bin/virtual_camera`) publishes cameras to apps with `cameraSource` ∈ {PASSTHROUGH, AVATAR, APP_DEFINED} — no IR source, PASSTHROUGH gated by `CREATE_VIRTUAL_DEVICE`.
4. **Apps** open PASSTHROUGH (color, via `HEADSET_CAMERA`) — the only headset camera data they can get, and it can't see IR.

There is no supported or firmware-level path from an IR illuminator to a visible passthrough image on the Quest 3. The repo ends here.
