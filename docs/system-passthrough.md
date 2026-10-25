# RAM-only system passthrough replacement

`tools/system_passthrough.py` substitutes live stereo IR for the camera textures
sampled by the Quest system compositor. The user confirmed IR in normal system
passthrough, upright orientation, stereo, corrected proportions, removal of
the earlier severe flicker, and correct hand/silhouette alignment after calibration. Virtual Desktop is not required.

## Run

Requirements on this setup: an authorized ADB connection with the server running, temporary
root already active, Python with `frida==17.17.0` and NumPy, and LLVM clang.
Firmware build `51154110174900520` is the only accepted build.

Wireless ADB is selected by the host-only `QUEST3_ADB_SERIAL` environment value,
then `%LOCALAPPDATA%/quest3-ir/adb-target.txt`, falling back to the USB serial.
`--serial IP:PORT` overrides the target. No persistent headset ADB property is set.
Enable a temporary TCP transport before restoring root: restarting ADB afterward
can terminate the temporary su daemon. The saved root payload requires Wi-Fi off
during exploitation; enable Wi-Fi after successful root recovery.

```powershell
python C:\Users\remus\quest3-ir-passthrough\tools\system_passthrough.py
```

Enable normal Quest passthrough and leave the command running. `IR ACTIVE` means
the compositor has actually sampled our IR textures. Ctrl+C returns to RGB. A
second terminal can stop it or inspect its counters:

```powershell
python C:\Users\remus\quest3-ir-passthrough\tools\system_passthrough.py --stop
python C:\Users\remus\quest3-ir-passthrough\tools\system_passthrough.py --status
```

`--seconds 30` runs a bounded test. `--frequency 60` is now the default;
`--frequency auto` preserves the lighting-frequency decision and `--frequency 50`
selects the earlier 50 Hz behavior temporarily. `--output-size 1280x960` is the
default GPU remap size; `640x512` and `2328x1748` are also available. This output
size does not change the 640x512 camera source. `--no-fast-mode` preserves the original camera
schedule. `--no-calibration` selects the earlier rotated/cropped view, with
`--rotate 0|90|180|270` (default 90), `--mirror`, and `--stretch` available for that
fallback. `--zoom`, `--swap-eyes`, and `--no-normalize` are also available.
Restart after changing options. Experimental `--ir-delay-ms 20` buffers the IR
frames for timing diagnosis; default is 0. `--set-delay 20` changes this delay on
a running controller. The user reported no improvement from the 20 ms delay;
it is currently zero.

Historical pose correction is now enabled by default with factory calibration;
`--no-pose-correction` disables it for diagnostics. `--pose-correction` remains
available as an explicit option.
It queries historical tracking at each IR publication's observed DMA time,
with an estimated exposure offset (`--pose-age-ms 8`, adjustable live with
`--set-pose-age 8`). This observed time is not a hardware exposure timestamp.
The earlier shadow/direct pose-buffer implementations initially looked smooth,
but black rectangles/flickering later recurred in the room and menus. A fenced
direct-write experiment and a GPU-copy experiment also failed visually despite
passing mocked checks and reporting no GL errors. A controlled no-pose run
restored a full room without rectangles, with the expected head-motion lag.

The new default `--pose-method image` rotates our IR pixels into the native
RGB capture orientation. It reads the native camera-row rotations without
writing or substituting the runtime uniform buffer. Native eye/menu/depth data
stay intact. The first 15-second live run and automatic RGB cleanup succeeded;
a subsequent run held 90Hz display rendering, zero GL/pose errors and about
58 changed image samples/sec per eye. These counters do not establish visual
correctness: the user reported improvement with some remaining tearing. The
current version latches new captures at the start of each eye/frame, rather than
allowing the 12ms poll timer to switch images between scanout halves. Its first
measurement held about 60 changed image samples/sec and 90 latched frames/sec per
eye, with no GL/pose failures. The user then confirmed "seems perfect now" with
this version active. This confirms the current visual test; other app layouts and
long-term behavior remain unverified. The low-resolution
source still limits image detail. `shadow` and `direct` remain explicit legacy
diagnostics with known rendering artifacts.

Image reconstruction can be selected at launch with `--filter linear|cubic|sgsr`
or changed live with `--set-filter linear|cubic|sgsr`. Linear is the default;
cubic and Snapdragon GSR are opt-in for each run. The launcher does not remember
the selection or write it to headset preferences. Stop restores normal RGB.
Cubic uses the checked `GL_IMG_texture_filter_cubic` extension only on the R8 IR
image; the floating-point calibration map stays linear. The user reported no
clear visual improvement from cubic, though motion/menu rendering stayed smooth.

SGSR uses a monochrome adaptation of Qualcomm's SGSR 1 edge-direction shader in
the existing remap pass. It samples the current IR image without adding a history
buffer. With `--filter sgsr`, sharpness defaults to1.5; values1 through2 are accepted when
launching. The12-tap luminance filter uses an8/255 edge threshold and the upstream
VR contrast clamp of+/-23/255. See [vendor attribution](../tools/vendor/sgsr1/README.md)
and its BSD-3-Clause license. The user confirmed improvement, with some remaining
edge aliasing/outlines. This can reconstruct
edges but cannot add uncaptured detail or perform temporal denoising. SGSR2 would
need matching depth/motion data and a different history pipeline.

The SGSR hardware test kept display rendering at72Hz, with no stale frames,
zeroGL/pose errors and about2ms timewarp. Both directions of live switching
between linear and SGSR were checked successfully. Current source resolution
remains640x512. Sampler bindings are explicitly saved/restored so these options
use our own image filters even if another app has bound sampler objects.

`--filter sgsr --sgsr-sharpness 1.0 --aa edge` selects the final tested enhancement
combination: gentler sharpening with a spatial,
directional anti-aliasing pass. It reconstructs into an owned R8 image, then
resolves high-contrast edges into the final YUV image. Flat regions bypass the
edge filter. This adds about2.5MB of image storage at1280x960 for both eyes and
one GPU pass per updated image. The initial test held72Hz display rendering,
about2.6ms timewarp and zeroGL/pose errors. Normal stop and startup with AA on/off
were checked. The user accepted the final image and ended tuning. At the user's
request, these enhancements remain optional: the default is linear with `--aa off`.
The current running session keeps its selected options until stopped; changing
launch defaults does not change an already running controller.

The launcher reuses existing `/data/local/tmp/su` and Frida helpers. It does not
run the saved root exploit, which writes userdata files. Temporary root must
already be active; root restoration remains a separate operation.

## Capture and camera schedule

The launcher verifies root and `/dev` tmpfs, builds its mono-only AArch64 reader
on the laptop, and puts new device files under `/dev/quest3-ir-session/system`.
It discovers the current sensors PID and verifies both distinct 13-buffer pools.
Readers map tracking buffers read-only. The system build omits the CPU RGBA
rotation/conversion used by the separate PC viewer. The historical
`.raw.rgba.seq` filename remains the publication counter even in mono-only mode.

On this firmware, `trackingfidelityservice` provides a validated
`TrackingFidelityTranslatorBase::applyMuxMode(unsigned char)` interface. The
launcher discovers the worldTracking translator and its configured mode names
at runtime. When the original mode is `lowLightHands` or `lowLightHandsObject`, it requests the
existing `lowLightHandsDouble` mode, increasing active tracking slots from two
to four. It starts fresh readers after that transition, avoiding a brightness
hold triggered by exposure changes. While active, a temporary hook at the same internal mode-selection interface
redirects requests for that exact two-slot hands mode to its four-slot variant.
A final check caught the service reverting to 25 images/sec without this policy.
Controller, depth, and disabled modes are left to the service. For the Home
hands/object mode, the explicit IR frame-rate request takes priority over dynamic
object/keyboard tracking until stop; the original mode is then restored. Meta
documents fast-motion and dynamic-object tracking as incompatible.

The ABI and offsets are specific to the checked build. The result has a
32-bit status in its low half; upper bits are not an error code. No arbitrary
sensor frame-rate value is passed to a driver, and no service is restarted.
The validated utility-frequency manager normally chooses 50 or 60 Hz from
lighting/geolocation/preferences. The launcher temporarily sets only its cached
user frequency to 60 and invokes its existing update function; no preference is
saved. A native hook maintains that cache while active. Both the cached value
and original camera mode are restored on stop. The original mode is saved and restored on normal stop. A pinned in-process
Frida script also restores it when host heartbeats stop.

## Compositor and calibration

`system_passthrough.js` and its RAM-compiled CModule hook the actual
`libGLESv3.so` draw calls in `com.oculus.vrruntimeservice`. Programs with a
`PassthroughColorImage` sampler are matched, and the draw's stereo scissor region
selects the eye. Texture numbers alone are not reliable eye identifiers.

Each eye gets our own R8 upload texture and YUV AHardwareBuffer/EGLImage. A
`GL_EXT_YUV_target` shader writes normalized IR luminance with neutral chroma
into these owned textures. The original draw samples the replacement; its
original RGB binding is restored immediately afterward. Camera-owned RGB
buffers remain untouched. Moving the per-draw hook into native code and using
owned images removed the earlier buffer-write flicker and much of the slowdown.
Relevant GL state is saved and restored around our uploads and draws.

The pose correction uses the checked firmware's v9 historical head
tracking query. Factory camera extrinsics convert the historical head pose into
the remapped camera pose. Both row poses are identical for the IR global shutter.
The image method uses `q_IR_from_world * inverse(q_native_RGB_from_world)` to
rotate a native RGB ray into the IR capture orientation. A per-eye RG32F lookup
stores unprojected RGB ray slopes. Our shader rotates these rays and applies the
factory IR Fisheye62 projection. Top/bottom native rows use sign-aligned normalized
quaternion interpolation. When native camera rows change, our remap can be redrawn
using the same IR upload; image uploads, remaps and native pose changes have
separate counters. New captures are selected at the observed scissor-Y frame wrap
and held for both scanout halves. Native-row changes between layers reproject the
latched capture without polling new pixels. Whole-height paths use an idle-gap
fallback; their app coverage is unverified. `latchedFrames` counts each eye's
detected frame starts. Uniform offsets, live mapping, range and capacity are checked
before reading; native buffers are never written or rebound to replacement storage
by this method. Legacy direct/shadow code remains for diagnostics only.

`ir_calibration.py` reads the existing factory camera calibration without
modifying it. It generates a separate RGB-to-IR ray lookup for each eye using
Fisheye62 intrinsics and camera rotations. RG32F lookup textures perform this
remap on the GPU, correcting camera angle and lens differences instead of
assuming a center crop. Projection/unprojection round trips were checked to
less than 1e-6 pixel over the tested central grid for all four cameras.

This is a rotation-only remap. It does not replace the compositor's depth model
or correct all translation/parallax between camera locations. Close hands may
still differ from the tracking silhouette. Out-of-sensor rays are black rather
than stretched edge pixels. The user confirmed: "YO ITS ALIGNED CORRECTLY NOW!"

## Restore and RAM scope

Ctrl+C / `--stop` removes the hook, releases owned GL resources, restores the
original tracking mode, and stops the controller's readers. The unchanged RGB
camera textures are immediately available again. A four-second compositor
heartbeat watchdog disables substitution if the host stalls. Both IR publishers
are monitored; sustained stalls stop the controller. Abruptly killing the host
was tested: the original `lowLightHands` mode returned automatically. An abrupt
host kill can leave RAM reader processes until cleanup or reboot.

New device binaries, frame files, and logs stay under `/dev`. Build artifacts and
host diagnostics go under `%TEMP%/quest3-ir-system`. No APK installation,
persistent property, firmware/partition write, or service restart is performed.
The launcher uses the separately requested proximity-close broadcast and wake
key; it does not change persistent sleep settings. Stopping passthrough does not
revoke previously established temporary root or undo the proximity override.
Reboot clears the hooks, reader processes, and tmpfs files.

## Measurements and coverage

On October 25, 2026:

- The original two-slot camera mode delivered about 22?24 changed IR images/sec
  through the replacement pipeline.
- In four-slot mode, a three-second full-frame CRC32 check measured about 50
  distinct left-eye input images/sec. Both reader publishers advanced about 50/s.
- With the initial RGBA-producing readers, compositor sampling observed about
  40?43 changed image samples/sec per eye. The mono-only reader subsequently
  delivered about 49 distinct left-eye input images/sec during a repeated CRC
  check. After retaining the four-slot hands mode, a six-second compositor
  measurement delivered 50.10 left / 49.92 right changed image samples/sec, with
  zero GL errors. A simulated normal two-slot request correctly retained the
  four-slot mode. Exact rates vary; these measurements are not a 60 fps guarantee.
- With temporary utility frequency 60, all four IR completion contexts measured
  60.0/s in a private ftrace instance (2300/2300 records, no overflow). A six-second
  full-frame CRC check measured 59.82 distinct left-eye images/sec; the compositor
  observed 60.03/59.85 changed sample images/sec, with a 1280x960 remap output and
  zero GL errors. A stable-buffer check at 2 ms intervals reduces publication
  jitter without counting partial DMA writes as additional frames.
- The owned-texture/native-hook runs reported zero GL errors. Normal timed stop,
  explicit stop, cold Frida startup, and host-loss mode restoration were tested.

`substitutedDraws` counts compositor draw slices, not video frames. `refreshes`
counts refresh checks, `uploads` counts uploaded images, and `changedSamples`
counts changes in sampled image hashes. Publisher counters and full-frame CRC
checks are separate measurements. Display cadence is separately observed in
VrApi logs; it held 72 Hz in the confirmed pose-correction run. This does not
mean that the cameras capture 72 images/sec.

This remains a firmware-specific prototype. Normal system passthrough is
visually confirmed. Other apps using this shared path are expected to sample
the substitution, but each alternate/foveated passthrough path needs its own
validation; not every app has been tested.


## Earlier head-motion and Home recovery work

Despite the measured 60 Hz camera and texture updates, the user reported very
poor apparent frame rate during head motion and persistent pixelation. The
runtime logged 72 Hz rendering with roughly 1.8 ms timewarp and no sustained
stale frames. These measurements do not establish smooth passthrough motion.
Historical pose correction initially improved the head-motion problem, and the
direct camera-field method initially cleared the menu regression from the
shadow-buffer method. Rectangles later recurred; the image method above replaces
these buffer-based approaches. The source stream remains640x512; a larger
remap texture cannot add captured detail.
Two color ISP contexts completed about 72 unique requests/sec on this setup.
That is a processing-cadence measurement, not a verified 120 fps RGB sensor mode.

Home independently reported "Shell is locked" with no panels. Restarting only
com.oculus.vrshell cleared that state (unlocked mode became true), and a headset
screenshot confirmed that its tracking-loss dialog became visible. Full menu
recovery at that point remained dependent on tracking/boundary initialization.

At the user's request for recovery, IR was stopped and both old and new readers
were removed. The native trackingservice_ctl wrapper could not set init control
properties from the existing root helper's kernel SELinux context. A transient
runcon init-context ctl.restart request then restarted trackingservice under
init. Camera service PID 855 and compositor PID 1764 remained unchanged; the
headset did not reboot and temporary root remained active. The restarted tracker
initially reported 3DOF, awaiting positional initialization. IR was left off for
native tracking recovery. No sensor-service restart, firmware
update, boundary deletion, or app-data clearing was performed.


The user then requested a reboot, which restored menus and tracking. Temporary
root was re-established. A stock-schedule37.5Hz test and20ms IR delay failed to
improve the motion problem. The successful version uses the60Hz schedule,
full-precision calibration maps and historical capture poses as described above.
The runtime proximity-close override keeps the headset awake, and the launcher
waits for advancing publishers before replacing RGB. No firmware update or
persistent sleep/root setting was installed.
