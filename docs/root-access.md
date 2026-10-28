# Root access on the Quest 3 (work log)

This records temporary-root recovery used to reach the IR tracking-camera
buffers. The project now also includes RAM-only readers and compositor hooks;
[system passthrough](system-passthrough.md) describes that implementation.
The examples below are chronological recovery notes, not instructions to rerun
an exploit whenever IR needs restarting. The launcher checks existing root first.

**Status: temporary root achieved.** Root and runtime patches are cleared by a
reboot. Helper files in `/data/local/tmp` survive; boot and system partitions
were not modified.

> **Hard constraint: runtime changes stay temporary.** No firmware/partition
> writes, persistent root, flashing, or bootloader unlock. New experiment binaries,
> frames and device logs stay in `/dev/quest3-ir-session` tmpfs. Existing
> `/data/local/tmp` helpers survive reboot; root and RAM hooks do not.

## Test device

| | |
|---|---|
| Headset | Meta Quest 3 (`eureka`, `ro.board.platform=anorak`) |
| Serial | `2G0YC5ZF9Q01J0` |
| Horizon OS | v79, build `51154110174900520` |
| Kernel | `5.10.226-g9505bf1c2b1a` |
| Android base | 14, security patch 2025-01-06 |
| SELinux | originally Enforcing (permissive once rooted) |

## Run the saved IonStack payload

Connect this Quest by **USB**, allow USB debugging, then run in PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "C:\Users\remus\quest3-ir-passthrough\tools\root_ionstack.ps1"
```

This starts the local ADB server and explicitly selects the physical USB serial,
checks for existing root, verifies the exact build/kernel and payload SHA-256,
and compares all 28 saved config values with the successful historical config.
It uses the already-staged `/data/local/tmp/preload` and `ionstack.conf`; no
payload download, firmware update or Magisk installation occurs. The saved
payload creates its existing su helpers in userdata; root itself clears on reboot.

Wi-Fi is disabled before exploitation because this payload previously panicked
with Wi-Fi on. Up to five attempts are allowed, with another attempt only after
a detected reboot and fresh checks. A 150-second timeout stops the launcher;
it does not prove the payload stopped. If it hangs, hold Power to reboot, then
rerun. Logs are saved on the laptop under `%TEMP%/quest3-ir-root`. On failure
Wi-Fi remains off to avoid disturbing a possibly running payload. On success,
Wi-Fi is enabled even if it was initially off, and Android Wireless debugging is
requested using the existing pairing; adbd and su are not restarted. Accept the
network approval popup if shown, then paired wireless ADB can reconnect.

`-CheckOnly` validates without running the exploit or changing Wi-Fi.
`-Attempts 1` limits it to one attempt. `-KeepWifiOff` suppresses Wi-Fi restoration.
An already-rooted headset skips exploitation and enables Wi-Fi unless suppressed.
`-CheckOnly` never changes Wi-Fi, including when root is already active. This script does not start IR;
use `tools/run_ir.ps1` afterward.

## Route taken: IonStack (CVE-2026-43499)

`CVE-2026-43499` ("GhostLock") is a stack use-after-free in the rtmutex PI code
(`remove_waiter()` clears the wrong task's `pi_blocked_on`). Meta fixed it in
Quest 3 build `52345320040100520` (Q3S `3697600032300610`); our build is older
and therefore vulnerable.

We used `F-19-F/IonStackQuest3`, adapted from the `IonStack` / CyberMeowfia work.
A preload library turns the UAF into kernel read/write, disables SELinux
(`selinux_state.enforcing = 0`) and patches `cred` to reach `uid=0`. It lives
only in kernel memory.

### What was done

1. Cloned `F-19-F/IonStackQuest3` to `C:\Users\remus\IonStackQuest3`.
2. Generated `ionstack.conf` from our own kernel image with
   `gen_ionstack_config.py` (28/28 symbols resolved) against
   `%TEMP%\opencode\dbg\vmlinux.elf`. `kimage_text_base = 0xffffffc008000000`,
   `mm_struct` order-2 (`MM_STRUCT_SZ 0x400`, `MM_ORDER 2`).
3. Downloaded the prebuilt `preload` (release `1.0`, 3948800 bytes, sha256
   `2FC76B94BBC1CF5A3CEB1AFD9892B5984FF0A1B4EC68E1421D30AF89D5C04243`).
   The release's `kernelsu.ko` was **not** used (it is the persistent path).
4. Pushed to `/data/local/tmp/`: `preload` (0755) and `ionstack.conf`.
5. Disabled Wi-Fi (`adb shell svc wifi disable`) and ran
   `/data/local/tmp/preload` right after a fresh boot.

### Result and usage

```
$ adb -s 2G0YC5ZF9Q01J0 shell /data/local/tmp/su -c 'id; getenforce'
uid=0(root) gid=0(root) groups=0(root) context=u:r:kernel:s0
Permissive
```

- One-off command: `adb -s 2G0YC5ZF9Q01J0 shell /data/local/tmp/su -c '<cmd>'`
- Shell tools (`pm`, `am`) need the shell domain:
  `adb -s 2G0YC5ZF9Q01J0 shell runcon u:r:shell:s0 /system/bin/sh`
- Daemon socket `/data/local/tmp/temp_su.sock`, su client
  `/data/local/tmp/su`, log `/data/local/tmp/su_daemon.log`.
- Re-gain after reboot: Wi-Fi off -> run `preload` (crashes the first couple of
  tries; the maintainer says 3-8 attempts is normal).

### Gotchas

- **Keep Wi-Fi off while running `preload`.** With Wi-Fi on it panicked every
  time at `prepare_pipe_buffer_page` / `replace_fake_fops`; with Wi-Fi off it
  succeeded on the first attempt. It stayed off across the retry reboots.
- Run right after boot; the exploit is known to hang occasionally (force reboot).
- Do **not** run any Magisk/manager installer. That is the persistent path and
  the real brick risk (AVB / anti-rollback -> hard brick, no recovery). The
  transient `preload` route at worst panics and reboots.

## Route B, re-examined: DirtyFrag (CVE-2026-43284) on v79

The original plan was the DirtyFrag page-cache write (`V4bel/dirtyfrag`,
`Henry1887/fuguquest`) to poison a loadable module and load it unsigned. A first
attempt on v79 failed and its notes under-claimed the cause: the bug itself was
fine, the *hook host* was not.

The chain needs (a) a daemon domain whose `open()` covers the carrier and config
files, being the process that reloads a poisoned code page, and (b) a function
in a file that `uid=2000 shell` may `open()`, so the poison can be staged. What
the earlier attempt measured as "only `trackingserver` can open module files"
was actually about which daemon mapped the hook library. A live probe (2026-10)
established:

- shell can `open()`: `system_lib_file`, `same_process_hal_file`,
  `vndk_sp_file`; it cannot open `vendor_file`, `vendor_wcnss_service_exec`
  or HAL exec files. That is enough to *stage* poisons, not enough to *load*
  them into a carrier.
- daemon domains that have both `vendor_file` open+read and UDP:
  `rild`, `set-usb-cdc-mac`, `vendor_wcnss_service`, `vendor_wifidisplayhalservice`,
  `hal_oculus_wifi_default` (`vendor.oculus.wifi-hal-1-0`).
- the old hook host `libeva.so` (`same_process_hal_file`) is mapped by
  `trackingservice`, whose domain cannot open `vendor_file`, hence the earlier
  `comm="trackingservice" name="llcc_perfmon.ko"` AVC. The bug's page-cache
  write was never the blocker.

The adapted chain hooks **`/system/lib64/libnetd_client.so`** instead — shell
opens it fine (`system_lib_file`), it has a 3412-byte executable gap at
`0x32ac`, and both `vendor.oculus.wifi-hal-1-0` (`hal_oculus_wifi_default`) and
`cnss-daemon` (`vendor_wcnss_service`) map it. `libnetd_client` is BIND_NOW
with no `DT_INIT`/`INIT_ARRAY`, so the hook is a PLT-entry overwrite rather
than an init-array redirect: the first word of the `socket@LIBC` PLT entry
(entry `0x6bb0`, GOT `0x7260`) becomes a `b` into the gap stub; the stub keeps
a one-shot flag in the lib's single `.data` byte at `0x8370` (on-disk value `1`,
rewritten to `0` per process) and returns through the original PLT tail with
`x16` re-derived, so normal socket calls keep working after the first call.

Restart is driven by the same `libandroid_servers::dump` stub as upstream
(`dumpsys input` makes *system_server* itself send `SETPROP2
ctl.restart vendor.oculus.wifi-hal-1-0` on the property socket), then shell does
`setprop ctl.start insmod_sh` for the root carrier, all already-proven for this
OS family. Port lives in `C:\Users\remus\fuguquest` (target
`targets/q3_51154110174900520.json`, `inject_lib` = libnetd_client PLT-hook
fields; orchestrate selects the variant when `inject_lib.plt_off` exists).
This local Fugu port and its `tools/root_fugu.ps1` developer wrapper are not
included in the published IR release. IonStack remains the supported recovery
launcher for the tested headset.

Offline checks passed: build clean, stub fits (est. ~1.7 KB of the 3412-byte
gap), guard/adrp/PLT-word encodings validated byte-for-byte. **Not yet run on
the headset** — validation checklist when next connected:

1. `tools/root_fugu.ps1 -CheckOnly` (build, SELinux = Enforcing, serial match).
2. `tools/root_fugu.ps1` (default `--adb-root`); expect a `vendor.oculus.wifi-hal-1-0`
   restart and AVCs from `hal_oculus_wifi_default` reading the carrier, then
   `Permissive`, then new `adb shell` = uid 0.
3. Recovery check: reboot restores Enforcing; Wi-Fi HAL restart via settings
   toggle must still pass RGB/camera checks per AGENTS.md.
4. `cnss-daemon` is a fallback hook host (no rc service name found, so it
   cannot be `ctl.restart`-triggered; `vendor.oculus.wifi-hal-1-0` is the
   restartable one).

IonStack (route A above) is kept as the primary launcher until route B is
validated on-device.

## Cleanup

Left on `/data/local/tmp` (helper files survive reboot; root must be restored):

- From this work: `preload`, `ionstack.conf`, `su`, `temp_su.sock`
  (`su_daemon.log` if written).
- From the earlier DirtyFrag attempt: `dfpoison`, `inject.spec`, `libas.spec`,
  `e2e.dex`, `cve_test32`.

At that point Wi-Fi was still disabled from the successful run; re-enable with
`adb shell svc wifi enable`.

## Why this matters here

With a root/kernel context we can look past the Android `camera_source`
enumeration (see [camera-provider.md](camera-provider.md)) directly at the XR
camera HAL and the tracking pipeline that owns the four OmniVision IR sensors.
That is the prerequisite for any attempt to get IR-sensitive frames out of the
headset. See [firmware-analysis.md](firmware-analysis.md) for the sensor map.

## References

- `F-19-F/IonStackQuest3` - Quest 3 port of IonStack
- `XiaoMeow-1145/One-click-temporary-Root-for-Meta-Quest3-3s`
- `NebuSec/CyberMeowfia` - IonStack / CVE-2026-43499 writeups
- `V4bel/dirtyfrag`, `Henry1887/fuguquest` - DirtyFrag (CVE-2026-43284);
  v79 port: local `C:\Users\remus\fuguquest` (+ commit-matched upstream),
  launcher `tools/root_fugu.ps1`
