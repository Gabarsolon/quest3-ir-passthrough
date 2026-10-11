# Injecting a background layer into Virtual Desktop (OpenXR composition)

Work log for the current approach to the repo's question: instead of trying to
make the system passthrough show IR, take over the **XR composition layer** that
sits *behind* the Virtual Desktop image and draw our own content there — with the
goal of eventually drawing the IR tracking-camera feed.

Status: **working prototype of the injection**. We can render our own layer
behind the VD desktop without breaking VD. Feeding the IR cameras in is the next
step.

## Why this route

- System passthrough is owned by Meta and cannot be switched to IR
  (see [firmware-analysis.md](firmware-analysis.md), [camera-provider.md](camera-provider.md)).
- Virtual Desktop renders its screen as XR composition layers. If we can add our
  own layer *below* VD's desktop and *above* its passthrough layer, anything we
  draw there appears as the "background room", while the VD desktop keeps
  working on top.
- This needs a root shell (to attach) but is entirely **volatile**: nothing is
  written to disk; a reboot returns the headset to stock.

## Tooling

- Temporary root via IonStack / CVE-2026-43499 (see [root-access.md](root-access.md)).
- `frida-server` **17.17.0** (`android-arm64`) pushed to `/data/local/tmp`, run as
  root: `su -c "nohup /data/local/tmp/frida-server -D >/data/local/tmp/frida.log 2>&1 &"`,
  then `adb forward tcp:27042 tcp:27042`.
- Host: `frida` Python 17.17.0.

### Frida gotchas hit along the way

- `device.enumerate_processes()` and `device.spawn()` throw
  `java.lang.NullPointerException ... PackageManager.queryIntentActivities`
  (frida-server has no app context). **Attach by PID** instead
  (`pidof VirtualDesktop.Android`).
- Frida 17 removed `Module.getExportByName` / `Module.findExportByName`. Use
  `Process.getModuleByName(mod).findExportByName(name)`. `Module.findGlobalExportByName`
  returns null for the loader's `xr*` symbols (they are `RTLD_LOCAL`).
- A FATAL: assigning to an undeclared variable (e.g. `session = ses;`) throws
  `ReferenceError` in the Frida script and aborts setup; declare every script
  global with `var`.

## What VD submits

Virtual Desktop (package literally `VirtualDesktop.Android`, a Mono/Xenko app)
renders with **OpenGL ES** and submits several XR composition layers per frame.
Layer `type` values observed:

| type | meaning |
|---|---|
| `1000118003` | `XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB` |
| `1000017000` | `XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR` — VD's curved virtual monitor (**the desktop**) |
| `35` | `XR_TYPE_COMPOSITION_LAYER_PROJECTION` (empty/transparent in VD) |

Layer composite order follows the array: later index = drawn on top.

## The hook

Attach to the running VD process and `Interceptor.attach` the exported
`xrEndFrame` trampoline in `libopenxr_loader.so`. In `onEnter`, rewrite the
`XrFrameEndInfo` layer array (the loader forwards the same `info` pointer to the
runtime, so in-place edits take effect).

`XrFrameEndInfo` layout used:
`type@0, next@8, displayTime@16, environmentBlendMode@24 (int), layerCount@28 (u32), layers@32 (const XrCompositionLayerBaseHeader** )`.

### Building our own layer

Using loader trampolines (no instance handle needed):

1. `xrEnumerateSwapchainFormats` → pick `0x8058` (`GL_RGBA8`).
2. `xrCreateSwapchain` with `XrSwapchainCreateInfo{ type=9, usage=0x21 (SAMPLED|COLOR_ATTACHMENT), format, sampleCount=1, w=640, h=512, faceCount=1, arraySize=1, mipCount=1 }`.
3. `xrEnumerateSwapchainImages` (type `1000024002` = `XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR`) → GL texture ids.
4. `xrCreateReferenceSpace` (`XR_TYPE_REFERENCE_SPACE_CREATE_INFO`=37, type=1 VIEW).
5. Upload with `glTexSubImage2D`, then a `XR_TYPE_COMPOSITION_LAYER_QUAD` (=36).
6. Per frame: `xrAcquireSwapchainImage` → **then** `xrWaitSwapchainImage`
   (order matters; wait-first returns `XR_ERROR_CALL_ORDER_INVALID` = -37),
   `glTexSubImage2D`, `xrReleaseSwapchainImage`.

### Struct offset traps

- `XrReferenceSpaceCreateInfo`: `poseInReferenceSpace` is at **offset 20**
  (`type@0, next@8, referenceSpaceType@16 (int), pose@20`). Writing it at 24
  produced a zero quaternion → `XR_ERROR_POSE_INVALID` (-39).
- `XrCompositionLayerQuad`: `XrPosef` is **28 bytes** (quaternion 16 + vector3f
  12), so `XrExtent2Df size` is at **offset 100**, not 104. Wrong offset → size
  (0,8) → `XR_ERROR_LAYER_INVALID` (-23) and an invisible/degenerate layer.
- `glTexSubImage2D` takes **9** arguments (target, level, x, y, w, h, format,
  type, pixels); declaring 8 throws Frida's "bad argument count".

## The result that matters

Where the new quad is inserted decides everything (all verified live in-headset):

| insertion point | result |
|---|---|
| replace the passthrough layer (`1000118003`) | no visible change (RGB passthrough stays) |
| append as the last layer | drawn **on top of everything**, covers the VD desktop and menus |
| insert before the projection layer (`35`) | still covers the desktop — because the desktop is the **cylinder**, drawn above it |
| **insert immediately after the passthrough layer, before the cylinder** | ✅ our image fills the background, **VD desktop + menus keep working** |

So the winning order is:

```
[passthrough]  [ OUR QUAD ]  [ cylinder = VD desktop ]  [ projection ]
```

A 640×512 `GL_RGBA8` quad, oriented to face the user, pushed to `z = -8 m` and
sized large, renders as the room background behind an intact Virtual Desktop.
`xrEndFrame` returns 0 and the frame rate is unaffected.

## Next

- Feed the **live IR tracking-camera frames** into the swapchain texture each
  frame instead of the test pattern. IR pixels come from the mono dma-bufs
  mapped in [ir-camera-mapping.md](ir-camera-mapping.md); because SELinux is
  permissive under the temp root, a root helper can publish frames to a
  world-readable path (e.g. `/data/local/tmp/ir.raw`) that the Frida script
  reads and uploads each frame.
- Then handle geometry (a flat quad vs. VD's curved surroundings) and stereo.

## Artifacts

- `inject.js` — Frida script (swapchain + quad + `xrEndFrame` layer rewrite).
- `fweave.py` — attaches to the running VD PID, loads `inject.js`, runs for N
  seconds.

(These live in the session scratch dir, not the repo.)
