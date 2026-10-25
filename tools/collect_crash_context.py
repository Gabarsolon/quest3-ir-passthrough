"""Save available reboot/crash diagnostics to the host; leave headset state alone.

Works with ordinary ADB, including paired wireless debugging. Does not run root,
install hooks, clear logs, restart services, or create device-side report files.
"""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import tempfile

import adb_transport as adb


COMMANDS = {
    'boot-state': 'getprop ro.build.version.incremental; cat /proc/sys/kernel/random/boot_id; '
                  'uptime; getprop ro.boot.bootreason; getprop sys.boot.reason; '
                  'getprop sys.boot.reason.last; getprop debug.oculus.refreshRate',
    'crash-buffer': 'logcat -b crash -d -v threadtime -t 600',
    'graphics-errors': 'logcat -b main -b system -d -v threadtime -t 5000 | '
                       "grep -i -E 'vrruntime|tracking|gpu|adreno|kgsl|watchdog|fatal|tombstone|reboot|virtualdesktop' | tail -n 500",
    'previous-boot-log': 'logcat -L -b all -d -v threadtime -t 1000',
    'dropbox-inventory': 'dumpsys dropbox',
    'boot-dropbox': 'dumpsys dropbox --print SYSTEM_BOOT | tail -n 300',
    'last-kernel-dropbox': 'dumpsys dropbox --print SYSTEM_LAST_KMSG | tail -n 400',
    'watchdog-dropbox': 'dumpsys dropbox --print system_server_watchdog | tail -n 300',
    'native-crash-dropbox': 'dumpsys dropbox --print SYSTEM_TOMBSTONE | tail -n 400',
    'pstore-list': 'ls -l /sys/fs/pstore',
    'runtime-exit-info': 'dumpsys activity exit-info com.oculus.vrruntimeservice',
}

# v79 stores native reports under this protobuf tag rather than SYSTEM_TOMBSTONE.
# Keep binary data intact. Android may truncate each payload to 256 KiB; the
# inspector labels that explicitly and preserves the report's internal date.
BINARY_COMMANDS = {
    'native-tombstones': 'dumpsys dropbox --proto SYSTEM_TOMBSTONE_PROTO_WITH_HEADERS',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial', default=adb.SERIAL)
    parser.add_argument('--out-dir', type=Path)
    options = parser.parse_args()
    adb.SERIAL = options.serial
    # Check the transport before making a directory or starting slower dumps.
    try:
        identity = adb.shell(COMMANDS['boot-state'], timeout=8)
    except (OSError, RuntimeError, EOFError) as exc:
        parser.exit(1, 'No reachable ADB target: ' + str(exc) + '\n')
    timestamp = datetime.now(timezone.utc)
    directory = options.out_dir or Path(tempfile.gettempdir()) / (
        'quest3-ir-crash-' + timestamp.strftime('%Y%m%d-%H%M%S'))
    directory.mkdir(parents=True, exist_ok=True)
    results = {}
    for name, command in {**COMMANDS, **BINARY_COMMANDS}.items():
        try:
            data = identity if name == 'boot-state' else adb.shell(command, timeout=12)
            suffix = '.pb' if name in BINARY_COMMANDS else '.txt'
            (directory / (name + suffix)).write_bytes(data)
            results[name] = dict(bytes=len(data), command=command)
            print(name + ': saved ' + str(len(data)) + ' bytes', flush=True)
        except (OSError, RuntimeError, EOFError) as exc:
            results[name] = dict(error=str(exc), command=command)
            print(name + ': unavailable (' + str(exc) + ')', flush=True)
    native_path = directory / 'native-tombstones.pb'
    if 'bytes' in results.get('native-tombstones', {}):
        from inspect_tombstones import summarize_dropbox
        try:
            summaries = summarize_dropbox(native_path.read_bytes())
            (directory / 'native-tombstones-summary.json').write_text(
                json.dumps(summaries, indent=2), encoding='utf-8')
        except ValueError as exc:
            results['native-tombstone-summary'] = dict(error=str(exc))
    (directory / 'manifest.json').write_text(json.dumps(dict(
        collectedUtc=timestamp.isoformat(), transport=options.serial,
        rootRequested=False, results=results), indent=2), encoding='utf-8')
    print('Host report: ' + str(directory), flush=True)


if __name__ == '__main__':
    main()
