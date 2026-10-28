"""Run the saved, verified Quest 3 IonStack payload over USB; root is temporary."""
import argparse
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re
import socket
import tempfile
import time

import adb_transport as adb

PAYLOAD = '/data/local/tmp/preload'
CONFIG = '/data/local/tmp/ionstack.conf'
PAYLOAD_SHA256 = '2fc76b94bbc1cf5a3ceb1afd9892b5984ff0a1b4ec68e1421d30af89d5c04243'
PROFILE = json.loads(Path(__file__).with_name('ionstack-build-51154110174900520.json').read_text())


def shell(command, timeout=6):
    return adb.shell(command, timeout=timeout).decode(errors='replace').strip()


def has_root():
    try:
        output = shell('/data/local/tmp/su -c "id; getenforce"', timeout=3)
        if re.search(r'(?:^|\n)uid=0\(root\)', output) and 'u:r:kernel:s0' in output:
            print('Temporary root verified: ' + output.replace('\n', '; '), flush=True)
            return True
    except (OSError, EOFError, RuntimeError):
        pass
    return False


def enable_wifi(options):
    if not options.keep_wifi_off:
        shell('svc wifi enable')
        shell('settings put global adb_wifi_enabled 1')
        print('Wi-Fi enabled and paired Wireless debugging requested. Allow the network popup if shown; '
              'adbd was not restarted.', flush=True)


def check_target():
    actual = shell('getprop ro.build.version.incremental')
    kernel = shell('uname -r')
    if actual != PROFILE['build'] or kernel != PROFILE['kernel']:
        raise RuntimeError(f'Build/kernel mismatch: {actual}, {kernel}. This saved payload/config is for '
                           f"{PROFILE['build']}, {PROFILE['kernel']} only.")
    fields = shell('sha256sum ' + PAYLOAD).split()
    if not fields or fields[0].lower() != PAYLOAD_SHA256:
        raise RuntimeError('Saved preload is missing or its SHA-256 differs from the previously verified payload.')
    config = shell('cat ' + CONFIG)
    pairs = {}
    for line in config.splitlines():
        line = line.split('#', 1)[0].strip()
        if not line:
            continue
        match = re.fullmatch(r'([a-z0-9_]+)\s*=\s*(0x[0-9a-fA-F]+|[0-9]+)', line)
        if not match or match[1] in pairs:
            raise RuntimeError('Unexpected or duplicate entry in saved ionstack.conf.')
        pairs[match[1]] = int(match[2], 16 if match[2].startswith('0x') else 10)
    digest = hashlib.sha256(json.dumps(pairs, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if len(pairs) != 28 or digest != PROFILE['configValuesSha256']:
        raise RuntimeError('Saved ionstack.conf differs from the 28 kernel values used in the successful session.')
    print(f'Verified build {actual}, kernel, payload and all 28 config values.', flush=True)


def wait_for_boot(timeout=120):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            if shell('getprop sys.boot_completed', timeout=3) == '1':
                return shell('cat /proc/sys/kernel/random/boot_id')
        except (OSError, EOFError, RuntimeError):
            pass
        time.sleep(1)
    raise RuntimeError('USB device did not finish booting. Connect/unlock the Quest and accept USB debugging.')


def assert_no_running_exploit():
    running = shell('pidof preload cve_test32 2>/dev/null || true')
    if running:
        raise RuntimeError('An exploit process is already running. Reboot the headset before another attempt.')


def attempt(log_path, seconds=150):
    started = last_update = time.monotonic()
    last_line = ''
    try:
        with log_path.open('wb') as log, adb.connect('shell:cd /data/local/tmp && ./preload', timeout=2) as stream:
            while time.monotonic() - started < seconds:
                try:
                    chunk = stream.recv(65536)
                except socket.timeout:
                    chunk = None
                if chunk == b'':
                    return 'exited'
                if chunk:
                    log.write(chunk)
                    log.flush()
                    lines = chunk.decode(errors='replace').splitlines()
                    if lines:
                        last_line = lines[-1][-180:]
                if time.monotonic() - last_update >= 10:
                    print(f'  {time.monotonic() - started:.0f}s: {last_line}', flush=True)
                    if has_root():
                        return 'root'
                    last_update = time.monotonic()
    except (OSError, EOFError, RuntimeError) as error:
        print('Payload connection ended: ' + str(error), flush=True)
        return 'disconnected'
    return 'timeout'


def run(options):
    # Wi-Fi must be disabled during this payload: do not select a saved wireless
    # transport, even when that is the IR launcher's normal preference.
    if not re.fullmatch(r'[A-Za-z0-9]+', options.serial):
        raise RuntimeError('Use the physical USB serial, not a Wi-Fi address or mDNS transport.')
    adb.SERIAL = options.serial
    print('Using USB ' + options.serial + '. Connect the cable and allow USB debugging.', flush=True)
    try:
        shell('getprop ro.serialno', timeout=4)
    except (OSError, EOFError, RuntimeError) as error:
        raise RuntimeError('Quest USB connection is unavailable. Connect the cable and allow USB debugging.') from error
    if shell('getprop ro.serialno') != options.serial:
        raise RuntimeError('USB device identity did not match the selected serial.')
    if has_root():
        if not options.check_only:
            enable_wifi(options)
        print('Root is already active; the exploit was skipped. You can run tools/run_ir.ps1.', flush=True)
        return
    check_target()
    if options.check_only:
        print('Checks passed. Wi-Fi and headset state were left unchanged; no exploit ran.', flush=True)
        return
    assert_no_running_exploit()
    log_folder = Path(tempfile.gettempdir()) / 'quest3-ir-root'
    log_folder.mkdir(exist_ok=True)
    print('Turning Wi-Fi off for IonStack. The payload may hang or reboot the headset.', flush=True)
    shell('svc wifi disable')
    for number in range(1, options.attempts + 1):
        boot = wait_for_boot()
        if has_root():
            break
        check_target()
        assert_no_running_exploit()
        shell('svc wifi disable')
        deadline = time.monotonic() + 10
        while 'Wifi is disabled' not in shell('cmd wifi status'):
            if time.monotonic() > deadline:
                raise RuntimeError('Wi-Fi did not turn off; exploit was not started.')
            time.sleep(0.5)
        log = log_folder / ('ionstack-' + datetime.now().strftime('%Y%m%d-%H%M%S') + f'-{number}.log')
        print(f'Attempt {number}/{options.attempts}; host log: {log}', flush=True)
        outcome = attempt(log)
        if has_root():
            break
        if outcome == 'timeout':
            raise RuntimeError('Attempt timed out; it may still be running. Wi-Fi remains off. '
                               'Hold Power to reboot if hung, then run this script again. No overlapping retry was started.')
        next_boot = wait_for_boot()
        if has_root():
            break
        if next_boot == boot:
            raise RuntimeError('No root and no reboot detected. Wi-Fi remains off. Reboot the headset, '
                               'then rerun; another attempt in this boot was not started.')
        print('Headset reboot detected; checking the new boot before retrying.', flush=True)
    else:
        raise RuntimeError('No root after the bounded attempts. Wi-Fi remains off. Logs: ' + str(log_folder))
    enable_wifi(options)
    print('Done. Root is temporary and clears on reboot. Run tools/run_ir.ps1 to start IR.', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial', default='2G0YC5ZF9Q01J0', help='Physical USB serial; wireless is not used for exploitation.')
    parser.add_argument('--attempts', type=int, choices=range(1, 9), default=5,
                        help='Maximum attempts; retries require a detected reboot. Default 5.')
    parser.add_argument('--keep-wifi-off', action='store_true')
    parser.add_argument('--check-only', action='store_true', help='Validate existing root or saved payload/config without exploiting or changing Wi-Fi.')
    options = parser.parse_args()
    try:
        run(options)
        return 0
    except KeyboardInterrupt:
        print('Stopped. A running payload may outlive this command; reboot before retrying.')
        return 130
    except (OSError, EOFError, RuntimeError) as error:
        print('Error: ' + str(error), flush=True)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
