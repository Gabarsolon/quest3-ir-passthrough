# Browser / Virtual Desktop transition failure

Reported on October 25, 2026 with image pose correction, per-eye capture
latching, 60 Hz utility schedule and a 120 Hz display request. Rendering source
is unchanged between the accepted Home run and commit `e431e3c`; the latter
added the resolution audit, offline inspector and documentation updates.

## Observed sequence

1. Opening Browser as an ordinary flat window inside Home stopped the IR view.
   Closing Browser restored it. This was not an immersive Browser session.
2. Opening Virtual Desktop produced a green right eye in the IR view.
3. It recovered briefly, then diagonal tearing/black rectangles flickered.
4. The whole headset froze and rebooted automatically.

This supersedes any interpretation of the earlier Home acceptance as general
app compatibility or a lasting global tearing fix. No crash cause has yet been
established from device logs.

## Evidence available so far

The saved host controller log reports destroyed Frida scripts, then device-offline
errors during cleanup. These establish a lost device/session, not which component
failed first. They are not a GPU crash dump. The initial ADB inventory contained no
connected devices. Host stdout, stderr, settings and recent hook messages were
preserved in the host temporary directory before any relaunch.

Initially no IR hook or exploit was restarted after this report. Reboot clears the temporary
root and RAM hooks; the old detached controller has exited. The user subsequently
confirmed that RGB passthrough, Browser and Virtual Desktop work normally with IR
off. This implicates the hooked configuration without identifying a specific bug.

## Post-reboot collection over paired wireless ADB

Android TLS wireless debugging was already paired and connected; no new pairing
code, `adb tcpip`, adbd restart or exploit was necessary. Device identity and
firmware were checked before saving the connection as the host-only target.
The build remains `51154110174900520`. Sensors, tracking and compositor PIDs were
re-read for this boot; no su or Frida server process was running. The display
override is empty after reboot, and current kernel display messages report 90 Hz.

Ordinary shell diagnostics were saved to the host on October 25 at 19:49 UTC:

- Boot reason properties all say `reboot`; they provide no component-level cause.
- The current crash buffer is empty. Previous-boot logcat is unavailable, and
  `/sys/fs/pstore` and `/data/tombstones` are denied to ordinary shell.
- DropBox records a boot at 21:28 local time. The nearby native report has an
  **internal timestamp of October 12**, a sensors PID different from the active
  pre-reboot sensor process, and abort text `Unsupported utility frequency: 1`.
  Another report imported earlier has the same old date and abort. These records
  cannot reliably be attributed to this app-transition incident. No 90/120 camera
  frequency was requested during the resolution/rate investigation.
- A compositor SIGSEGV report has an internal date of October 25 at 18:07, also
  before the reported reboot and with a different process ID from the final Home
  session. Its payload is truncated by the DropBox export. It does not establish
  the cause of the later freeze.
- A post-boot `BlackScreen` soft-error record concerns the new compositor's
  interstitial renderer. Its timing does not establish a pre-reboot failure.

The available records confirm a new boot but do not establish this reboot's
cause. Keep DropBox import dates separate from the timestamps inside native
reports. Full reports stay in host temporary storage, outside the public repo.
The paired transport later went offline; the user will reconnect later. No
renderer change has been deployed.

## USB recovery and wireless-root verification

The user reconnected by USB later that evening. The updated ordinary-shell
collector completed, including native protobuf summaries; no new incident
tombstone was present. Before restoring root, the firmware and saved preload
checksum were checked. The unchanged saved recovery payload succeeded on the
fifth attempt, with Wi-Fi disabled during exploitation. Failed attempts rebooted
the headset; logs from those recovery boots must not be attributed to the earlier
app-transition freeze.

Wi-Fi was re-enabled afterward. Wireless debugging needed enabling again and
displayed a network-approval popup, which the user accepted. Paired TLS wireless
ADB then reconnected automatically. `su -c id` over that explicitly selected
wireless transport returned `uid=0(root)` in the same boot as USB verification.
The detached su daemon and adbd retained their PIDs throughout this Wi-Fi step;
no `adb tcpip` or adbd restart was used. The host preference now uses the paired
mDNS transport name so a changed connection port need not be hard-coded.

Rooted inspection found pstore empty and confirmed the newest retained raw
tombstone is the earlier 18:07 compositor crash. It supplies no newer evidence
for the app-transition freeze. A bounded RGB-only compositor probe then recorded
17,440 draws, 15,696 matching the passthrough sampler, with no reported probe
errors or compositor restart. Nine matching programs shared one EGL context and
render thread, external YUV sampler type `0x8be7`, a 3184-byte UBO, and camera
offsets 2912/2928/2944/2960. The viewport was 4128x2208 with 2064x1104 stereo
scanout halves. This establishes the Home baseline only; Browser and Virtual
Desktop traces remain pending. The probe queried GL state without texture/buffer
binds, uploads, native mapped-memory reads or camera changes, then detached.
IR substitution remains off.

## Context and protected-output investigation

Bounded RGB-only traces of Home, flat Browser and Virtual Desktop found the same
external YUV sampler type, 3184-byte camera UBO and offsets, 4128x2208 viewport
and 2064x1104 scanout halves. Browser selected a different EGL context from
Home/Virtual Desktop. Sanitized comparisons are in
[app-context-layouts.json](../evidence/app-context-layouts.json).

The old hook had no context ownership guard. Resources, numeric GL object IDs,
program locations and pose/frame caches are now separate per context. Relink,
deletion and context-handle reuse invalidate the appropriate cache/lifetime.
A 20-second Virtual Desktop test passed visually in both eyes, but Browser over
Virtual Desktop still made the entire IR room green, despite zero GL/pose errors.

Browser draws also enabled native clip distances. Disabling/restoring these and
sample masking around our full-screen remap did not resolve the green room;
those state-isolation changes remain appropriate for our shader, which does not
write clip distances. Alpha test and sample masks were disabled in the normal
RGB traces. The compatible viewport/UBO layout did not explain the failure.

A subsequent context query returned `EGL_PROTECTED_CONTENT_EXT = 1` and
`GL_CONTEXT_FLAGS = 0x10` for Browser's context. Our output buffers were allocated
without protection. Protected fragment stages may read ordinary input images,
but must write protected outputs; an invalid write need not produce a GL error.
See [EGL_EXT_protected_content](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_protected_content.txt)
and [GL_EXT_protected_textures](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_protected_textures.txt).
This supplies a specific output-allocation defect, without proving the earlier
headset reboot's cause.

The renderer now queries each context's protection state. Its owned IR YUV
AHardwareBuffers add `AHARDWAREBUFFER_USAGE_PROTECTED_CONTENT` and their EGLImages
add the matching protection attribute when needed. Optional AA creates protected
immutable R8 texture storage in that context too. Upload/calibration textures
remain ordinary read inputs. No native app image is read back or modified.
The user confirmed **IR visible and stable in both eyes** with Browser over
Virtual Desktop during a 30-second test. GL and pose errors remained zero and
normal RGB was restored afterward.

An earlier diagnostic sampled only our own unprotected output and found a
uniform green result; a later temporary constant-draw/clear diagnostic hit a
Frida-caught native exception. Those diagnostics were removed before production
testing. Once context protection was identified, no protected-output readback
was attempted. Debug code and raw images are not part of the repository.

## Separate startup failure and Frida runtime update

The following repeat launch crashed the compositor during calibration transfer,
before either eye map was complete and before IR substitution was enabled.
The new tombstone reports SIGSEGV in the timewarp draw call, with an invalid
branch destination containing instruction bytes. The compositor restarted in
the same boot; the user confirmed RGB and menus recovered normally. This is a
new, retained report, separate from the original automatic headset reboot.

The installed Frida 17.17 used multi-instruction ARM64 inline redirects. Frida
17.23 documents a matching online patch race and prefers single-instruction
branches to narrow it; see the [17.23 release notes](https://frida.re/news/2026/10/07/frida-17-23-0-released/).
This is consistent with the observed startup fault, not proof of all earlier
failures. The launcher now rejects Frida older than 17.23 before device access.
The user ran the supplied setup script to prepare isolated 17.24.3 client/server
files on the laptop. The existing userdata helper and global Python installation
were preserved. The newer server is staged only in tmpfs, without restarting
adbd or su. A live query confirmed the draw redirect is now one `b` instruction.

The 17.24.3 remote transport then closed on the old 128 KiB calibration chunks.
A minimal script with no rendering hooks reproduced the cutoff: payloads up to
64 KiB worked; 128 KiB disconnected, while both server and compositor stayed alive.
This matches [Frida issue 3806](https://github.com/frida/frida/issues/3806).
Calibration chunks are now 64 KiB to leave room for RPC framing. Startup and IR
rendering then succeeded on the newer runtime with zero reported GL/pose errors.
The two-minute run ended normally, retained the compositor PID, restored RGB
and restored the empty prior display override. Visual confirmation of repeated
Browser/Virtual Desktop switching is pending.

Inactive contexts may retain GL objects until their native context/share group
dies; cleanup never deletes names through a foreign context. Independent EGL
and AHardwareBuffer references are released, with deferred GL cleanup reported.
Each run is limited to eight live and 64 total created contexts, returning to RGB
if exceeded. Long-running app compatibility and the original reboot remain open.

## Collect logs before another experiment

Ordinary ADB is sufficient to collect the logs exposed to shell. USB or Android
wireless debugging with pairing can be used. Pairing does not restore temporary
root. The pairing port and the normal wireless connection port may differ;
use the addresses shown by the headset. See
[Android's wireless debugging instructions](https://developer.android.com/tools/adb#wireless-android11-command-line).

The host-side collector does not run root, start hooks, clear logs, restart a
service or create device-side report files:

```powershell
python .\tools\collect_crash_context.py --serial DEVICE_OR_IP_AND_PORT
```

It saves available boot properties, the crash buffer, filtered graphics logs,
previous-boot log output, relevant DropBox records, pstore listing and runtime
exit information to a host temporary folder. Missing permissions or unsupported
commands are recorded rather than worked around by changing device settings.
Ordinary collection and manual retrieval of the native protobuf records succeeded
over paired ADB. The collector now also retrieves the actual v79 native-report
tag, `SYSTEM_TOMBSTONE_PROTO_WITH_HEADERS`; the older `SYSTEM_TOMBSTONE` tag was
empty. This updated collection path awaits a complete device rerun after
reconnection. The offline decoder was checked against the saved native reports,
including the truncated compositor record, and synthetic date/truncation tests.

`tools/inspect_tombstones.py REPORT.pb` summarizes the native export without
printing register or memory dumps. It retains both timestamps and labels
truncation. Reports may contain device/application diagnostics and are kept on
the host rather than committed automatically.
