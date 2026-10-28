"""Deploy/start the native headset controller. ADB is not needed after startup."""
import argparse
import hashlib
from pathlib import Path
import secrets
import shlex
import sys
import time

import adb_transport as adb
from system_passthrough import root

BASE = '/dev/quest3-ir-session/headset'
OUTPUT = Path(__file__).resolve().parents[1] / 'build' / 'headset'


def upload(path):
    if path.name not in {'ir-headset', 'irfeed'}:
        raise ValueError('Unexpected native binary')
    data = path.read_bytes()
    if data[:5] != b'\x7fELF\x02' or data[18:20] != b'\xb7\x00':
        raise ValueError('Expected an Android ARM64 executable')
    digest = hashlib.sha256(data).hexdigest()
    target = BASE + '/' + path.name
    existing = root(f'if [ -f {target} ]; then sha256sum {target}; fi')
    if existing.split()[:1] == [digest]:
        print('Already staged: ' + target)
        return
    nonce = secrets.token_hex(12)
    incoming = BASE + '/upload-' + nonce
    greeting = ('IR_HEADSET_' + nonce + '\n').encode()
    receive = ('printf %s ' + shlex.quote(greeting.decode())
               + f'; head -c {len(data)} > {incoming} && printf IR_UPLOAD_OK')
    pid = int(root('setsid /system/bin/toybox nc -s 127.0.0.1 -p 27345 -l -w 120 sh -c '
                   + shlex.quote(receive) + f' </dev/null >{incoming}.log 2>&1 &\necho $!').splitlines()[-1])
    try:
        stream = None
        for _ in range(30):
            try:
                stream = adb.connect('tcp:27345', timeout=120)
                break
            except RuntimeError:
                time.sleep(.1)
        if stream is None:
            raise RuntimeError('RAM upload receiver did not start')
        with stream:
            if adb.readn(stream, len(greeting)) != greeting:
                raise RuntimeError('Unexpected RAM upload receiver')
            print(f'Staging {path.name}: {len(data):,} bytes', flush=True)
            stream.sendall(data)
            result = bytearray()
            while block := stream.recv(4096):
                result.extend(block)
            if result.strip() != b'IR_UPLOAD_OK':
                raise RuntimeError('RAM upload was incomplete')
        if root('sha256sum ' + incoming).split()[0] != digest:
            raise RuntimeError('RAM upload checksum mismatch')
        root(f'chmod 0700 {incoming}\nmv {incoming} {target}')
    finally:
        # Only our unique receiver's command line may be terminated.
        root(f'if [ -r /proc/{pid}/cmdline ]; then\n'
             f'case "$(cat /proc/{pid}/cmdline | tr "\\000" " ")" in '
             f'*{nonce}*) kill {pid};; esac; fi\nrm -f {incoming} {incoming}.log')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial')
    parser.add_argument('--deploy', action='store_true')
    actions = parser.add_mutually_exclusive_group()
    for action in ['start', 'stop', 'status', 'check-only']:
        actions.add_argument('--' + action, action='store_true')
    parser.add_argument('options', nargs=argparse.REMAINDER, help='Native arguments after --, for example -- --brightness 1.2')
    args = parser.parse_args()
    adb.SERIAL = adb.resolve_serial(args.serial)
    if args.deploy:
        if 'uid=0(root)' not in root('id'):
            raise RuntimeError('Temporary root is not active')
        mounts = root('cat /proc/mounts')
        if not any(line.split()[1:3] == ['/dev', 'tmpfs'] for line in mounts.splitlines()):
            raise RuntimeError('/dev is not tmpfs')
        if adb.shell('getprop ro.build.version.incremental').decode().strip() != '51154110174900520':
            raise RuntimeError('Unsupported headset build')
        current = root(f'if [ -x {BASE}/ir-headset ]; then {BASE}/ir-headset --status; fi')
        if current and '"status":"stopped"' not in current:
            raise RuntimeError('Stop the native headset controller before replacing its binaries')
        root(f'mkdir -p {BASE}\nchmod 0700 {BASE}')
        for name in ['irfeed', 'ir-headset']:
            upload(OUTPUT / name)
    if args.start:
        options = args.options[1:] if args.options[:1] == ['--'] else args.options
        command = f'TMPDIR={BASE} setsid {BASE}/ir-headset ' + shlex.join(options)
        pid = root(command + f' </dev/null >{BASE}/controller.log 2>&1 &\necho $!')
        print('Started headset-native controller PID ' + pid + '. No host heartbeat is used.')
        time.sleep(1)
        print(root(BASE + '/ir-headset --status'))
    elif args.stop or args.status or args.check_only:
        action = '--stop' if args.stop else '--check-only' if args.check_only else '--status'
        print(root(BASE + '/ir-headset ' + action))
    elif not args.deploy:
        parser.error('Choose --deploy, --start, --stop, --status, or --check-only')


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, ValueError, EOFError) as exc:
        print('Error: ' + str(exc), file=sys.stderr)
        sys.exit(1)
