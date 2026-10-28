# Headset-only controller (preview release)

The Quest IR control app runs the reader, factory calibration calculation,
Frida injection, monitoring, and heartbeat on the Quest itself. It does not need
Python, a laptop controller, Frida server, ADB forwarding, Wi-Fi, or a USB cable
while IR is running. This uses the same calibrated rendering hooks as the
maintained laptop implementation; it does not unlock another sensor mode.

**Temporary root must already be active.** The app does not obtain root. After
reboot, restore root with the existing supported recovery route, then open the
app. A completely PC-free root recovery after reboot remains separate work.

The ordinary control APK persists in userdata, as approved on October 29. Root,
camera scheduling changes, compositor hooks, and all extracted native binaries,
images, and logs remain temporary. No firmware or system partition is modified.

## Use

1. Download `quest-ir-v0.2.0-preview.1.apk` from the
   [release page](https://github.com/Gabarsolon/quest3-ir-passthrough/releases/tag/v0.2.0-preview.1)
   and install it with authorized ADB or your usual APK installer. Its launcher
   name is **Quest IR** (under Unknown Sources in the normal Quest app library).
   For ADB: `adb install -r quest-ir-v0.2.0-preview.1.apk`.
2. Stop the laptop IR controller first. Enable normal system passthrough.
3. Open Quest IR, choose brightness and optional filtering/display timing, then
   select **Start IR**. Options apply to that run. Linear filtering, no edge AA,
   and no display override are the app's defaults.
4. Close the app or unplug the laptop; the root supervisor keeps running.
5. Open the app and select **Stop IR / restore RGB** to stop. Normal app starts
   have **no time limit**. Short automatic stops used in development tests come
   from the native `--seconds` option, not from the app.

If an old laptop reader is still running, native startup refuses to install a
second set of hooks. The laptop controller also checks for native IR first.
The existing loopback `system_passthrough.py --stop` command controls the laptop
controller only; use the app or native stop command for headset IR.

The source remains 640x512 per eye, about 60 distinct camera images/sec in the
previous measurements. The remap texture is 1280x960 per eye. The optional
120 Hz display setting changes display timing, not camera frame rate.

## RAM paths and recovery

- `/dev/quest3-ir-session/headset`: root-only controller, reader executable,
  Frida temporary files, logs, PID lock and status. Native code is extracted
  from the APK and checked against its build's SHA-256 digests.
- `/dev/quest3-ir-session/headset-frames`: image, sequence and timestamp files
  readable by the compositor's different uid. This directory is separate from
  the private controller directory; making all runtime files root-only prevents
  IR substitution.
- A local heartbeat refreshes the existing four-second hook watchdog. Controller
  failure stops substitution; it cannot leave IR dependent on a PC connection.
- Reader children receive a parent-death signal. The display restoration guard
  restores the exact prior override if the controller dies, including SIGKILL.
- Reboot clears root, hooks and runtime files. The APK itself stays installed.

Diagnostic commands, with root already active:

```powershell
python tools/headset_ir.py --status
python tools/headset_ir.py --stop
python tools/headset_ir.py --deploy --check-only
python tools/headset_ir.py --start -- --brightness 1.2 --display-refresh 120
```

These use ADB for deployment or inspection; the launched supervisor continues
independently after the command exits. On the headset, the equivalent command
is `/dev/quest3-ir-session/headset/ir-headset`, run through the saved temporary
root helper. `--status`, `--stop`, and `--check-only` do not install hooks.

## Build

```powershell
python tools/build_headset.py --download
python tools/build_headset_app.py
```

The Windows build uses official Android NDK r30 and Frida Core 17.24.3 with
pinned archive checksums. The app build downloads the required official SDK
platform/build tools, checks their published SHA-1 digests, and signs a
preview APK with a local development key under `%LOCALAPPDATA%/quest3-ir/build`.
Do not commit the key or local per-headset calibration.
The local build output is `dist/quest-ir-headset-dev.apk`; release assets use
the versioned filename above. Independently built APKs have a different signing
key and cannot update an existing installation signed with the release key.

The app bundles the controller and reader as assets. At first start after each
boot, it extracts them directly from its installed APK into tmpfs through the
existing temporary-root helper. Each launch reads this headset's current
factory calibration and discovers the current sensor PID and DMA descriptors.
No stored device-specific fd list or another headset's calibration is used.

An optional root-preflight smoke test runs from the app's own uid:

```powershell
adb shell am instrument -w -r com.gabarsolon.questir/.SmokeTest
```

This uses Android's authorized instrumentation entry point, checks the root
helper and native preflight, and does not exploit, inject, or restart services.
The native `--calibration-json` diagnostic exports a 53x41 ray-map sample plus
camera poses/rotations so they can be compared with the NumPy implementation.
`--rpc-self-test` instruments only a newly spawned, owned `sleep` process and
checks 2,000 replies, including 1,000 binary payloads. Completion is published
under a mutex after copying each result; Frida callbacks can run on a different
thread from the supervisor. The earlier missing synchronization could stop IR
after a few seconds even when no duration limit was requested.

The user confirmed that the corrected headset build works on October 29 after
the RPC synchronization fix. The native supervisor also stayed active for
several minutes with zero reported rendering or pose errors. This is a preview
for the tested firmware; extended operation, individual controls, and repeated
app transitions still need broader validation.

## Upstream components

The native executable links the official [Frida Core 17.24.3 devkit](https://github.com/frida/frida/releases/tag/17.24.3).
Frida's source and license are available in
[frida-core](https://github.com/frida/frida-core/tree/17.24.3) and its
[COPYING](https://github.com/frida/frida-core/blob/17.24.3/COPYING).
Optional SGSR retains the [existing attribution](../tools/vendor/sgsr1/README.md).
