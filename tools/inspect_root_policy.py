"""Query loaded SELinux policy for Fugu candidates; never change policy or run an exploit."""
import argparse
import json
from pathlib import Path
import shlex

import adb_transport as adb
from system_passthrough import root

DOMAINS = ['trackingserver', 'crash_dump', 'rild', 'set-usb-cdc-mac',
           'vendor_wcnss_service', 'vendor_wifidisplayhalservice',
           'hal_oculus_wifi_default', 'hal_oculus_sensors_default',
           'hal_graphics_composer_default', 'init-insmod-sh', 'init']


def definitions(kind, permissions):
    base = '/sys/fs/selinux/class/' + kind
    index = int(root('cat ' + base + '/index'))
    positions = {permission: int(root('cat ' + base + '/perms/' + permission))
                 for permission in permissions}
    return index, positions


def decode_decision(reply, positions):
    fields = reply.split()
    if len(fields) != 6:
        raise RuntimeError('Invalid kernel access-decision reply: ' + repr(reply))
    allowed = int(fields[0], 16)
    if any(not 1 <= position <= 32 for position in positions.values()):
        raise RuntimeError('Invalid SELinux permission bit position.')
    return {'permissions': {name: bool(allowed & (1 << (position - 1)))
                            for name, position in positions.items()},
            'policySequence': int(fields[4]), 'flags': int(fields[5], 16)}


def query(source, target, definition):
    index, positions = definition
    request = shlex.quote(f'{source} {target} {index}')
    # This transaction endpoint computes a decision for the supplied contexts.
    # Writing its query is not writing/loading policy or setting enforcement.
    reply = root('exec 3<> /sys/fs/selinux/access; printf %s ' + request + ' >&3; cat <&3')
    return decode_decision(reply, positions)


def inspect():
    file = definitions('file', ['open', 'read'])
    udp = definitions('udp_socket', ['create', 'write', 'connect', 'setopt'])
    report = {'build': adb.shell('getprop ro.build.version.incremental').decode().strip(),
              'kernel': adb.shell('uname -r').decode().strip(),
              'currentEnforcement': root('getenforce'),
              'method': 'Kernel selinuxfs access-decision queries for explicit source/target contexts. '
                        'No policy/enforcement change, file poisoning, module loading or service restart.',
              'limitation': 'Policy permission is not proof of a working exploit or a reachable code-execution trigger.',
              'domains': []}
    for domain in DOMAINS:
        source = f'u:r:{domain}:s0'
        report['domains'].append({'domain': domain,
                                  'moduleFile': query(source, 'u:object_r:vendor_file:s0', file),
                                  'ownUdpSocket': query(source, source, udp)})
    sequences = {item[key]['policySequence'] for item in report['domains']
                 for key in ['moduleFile', 'ownUdpSocket']}
    if len(sequences) != 1:
        raise RuntimeError('Loaded policy changed during inspection; results were not saved.')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial')
    parser.add_argument('--output', type=Path, help='Optional host JSON report.')
    options = parser.parse_args()
    adb.SERIAL = adb.resolve_serial(options.serial)
    report = inspect()
    encoded = json.dumps(report, indent=2) + '\n'
    if options.output:
        options.output.write_text(encoded, encoding='utf-8')
        print('Saved host policy-decision report: ' + str(options.output))
    else:
        print(encoded, end='')


if __name__ == '__main__':
    main()
