# Root access on the Quest 3 (work log)

The rest of this repo is read-only research. This document is the exception: it
records the effort to get a root shell on the headset, which is the only way to
reach the IR tracking cameras inside Meta's XR camera HAL (see
[reverse-engineering.md](reverse-engineering.md) and
[firmware-analysis.md](firmware-analysis.md)).

**Status: temporary root achieved.** No partition was written; nothing survives a
reboot.

## Test device

| | |
|---|---|
| Headset | Meta Quest 3 (`eureka`, `ro.board.platform=anorak`) |
| Serial | `2G0YC5ZF9Q01J0` |
| Horizon OS | v79, build `51154110174900520` |
| Kernel | `5.10.226-g9505bf1c2b1a` |
| Android base | 14, security patch 2025-01-06 |
| SELinux | originally Enforcing (permissive once rooted) |

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

## The dead end that came first: DirtyFrag (CVE-2026-43284)

The original plan was the DirtyFrag page-cache write (`V4bel/dirtyfrag`,
`Henry1887/fuguquest`) to poison a loadable module and load it unsigned. It is
blocked on v79 by SELinux policy, not by the bug itself.

The chain needs one process that can both `open()` the target module file (from
`vendor_file`) and create an `AF_INET` socket to stage the ESP SA. On v79 the
only netdomain-ish daemons that can open module files are tracked under
`u:r:trackingserver:s0`, and the AVC we kept hitting was:

```
denied { read } ... comm="trackingservice" name="llcc_perfmon.ko"
scontext=u:r:trackingserver:s0 tcontext=u:object_r:vendor_file:s0
```

No domain does both, and only `u:r:kernel` can flip Enforcing. We asked
"Given the open()+AF_INET conflict is fundamental on v79, how should I proceed?"
and chose *search upstream / other LPE*. That search is what led to IonStack,
which sidesteps the carrier problem entirely because it needs no module load.

## Cleanup

Left on `/data/local/tmp` (all transient; wiped on reboot):

- From this work: `preload`, `ionstack.conf`, `su`, `temp_su.sock`
  (`su_daemon.log` if written).
- From the earlier DirtyFrag attempt: `dfpoison`, `inject.spec`, `libas.spec`,
  `e2e.dex`, `cve_test32`.

Wi-Fi is currently still disabled from the successful run; re-enable with
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
- `V4bel/dirtyfrag`, `Henry1887/fuguquest` - DirtyFrag (CVE-2026-43284)
