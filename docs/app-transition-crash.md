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

No IR hook or exploit was restarted after this report. Reboot clears the temporary
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

## Concrete gaps to investigate

The hook intercepts `glDrawElements` throughout the compositor process. It caches
program IDs and reuses owned textures/framebuffer resources without recording the
EGL context that created them. Its eye/layout classification assumes a horizontal
stereo viewport and scissor behavior validated in the Home trace. Program object
lifetimes, additional contexts, different layer layouts, sampler variants and
capture-mode transitions are not yet covered by app-transition validation.

These are implementation gaps and hypotheses, not proof that context reuse
caused this reboot. A green eye also does not establish a specific YUV/texture
fault. The next useful evidence is the reboot reason, available crash/previous-boot
logs and a bounded read-only baseline trace of each app's normal RGB rendering.

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
