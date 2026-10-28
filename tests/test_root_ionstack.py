"""Check that the root launcher refuses unsafe or overlapping attempts."""
import hashlib
import json
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import root_ionstack as root


class RootLauncherTests(unittest.TestCase):
    def options(self, **changes):
        values = dict(serial='2G0YC5ZF9Q01J0', attempts=5, keep_wifi_off=False, check_only=False)
        return SimpleNamespace(**(values | changes))

    def test_existing_root_check_only_never_runs_payload_or_changes_wifi(self):
        with patch.object(root, 'shell', return_value='2G0YC5ZF9Q01J0') as shell, \
                patch.object(root, 'has_root', return_value=True), \
                patch.object(root, 'attempt') as attempt, \
                patch.object(root, 'check_target') as check:
            root.run(self.options(check_only=True))
        attempt.assert_not_called()
        check.assert_not_called()
        self.assertTrue(all(call.args[0] == 'getprop ro.serialno' for call in shell.call_args_list))

    def test_existing_root_enables_wifi_unless_suppressed(self):
        for keep_off in [False, True]:
            with self.subTest(keep_wifi_off=keep_off), \
                    patch.object(root, 'shell', return_value='2G0YC5ZF9Q01J0') as shell, \
                    patch.object(root, 'has_root', return_value=True), \
                    patch.object(root, 'attempt') as attempt:
                root.run(self.options(keep_wifi_off=keep_off))
                attempt.assert_not_called()
                commands = [call.args[0] for call in shell.call_args_list]
                self.assertEqual('svc wifi enable' in commands, not keep_off)
                self.assertEqual('settings put global adb_wifi_enabled 1' in commands, not keep_off)

    def test_wireless_transport_rejected_before_access(self):
        with patch.object(root, 'shell') as shell:
            with self.assertRaisesRegex(RuntimeError, 'physical USB serial'):
                root.run(self.options(serial='192.168.0.50:5555'))
            shell.assert_not_called()

    def test_build_mismatch_and_config_tampering(self):
        with patch.object(root, 'shell', side_effect=['different-build', root.PROFILE['kernel']]):
            with self.assertRaisesRegex(RuntimeError, 'mismatch'):
                root.check_target()
        pairs = {f'key{i}': i for i in range(28)}
        profile = dict(root.PROFILE, configValuesSha256=hashlib.sha256(
            json.dumps(pairs, sort_keys=True, separators=(',', ':')).encode()).hexdigest())
        config = '\n'.join(f'{key} = {value:#x} # comment' for key, value in pairs.items())
        prefix = [profile['build'], profile['kernel'], root.PAYLOAD_SHA256 + ' /data/local/tmp/preload']
        with patch.object(root, 'PROFILE', profile), patch.object(root, 'shell', side_effect=prefix + [config]):
            root.check_target()
        with patch.object(root, 'PROFILE', profile), patch.object(root, 'shell', side_effect=prefix + [config.replace('key0 = 0x0', 'key0 = 0x1')]):
            with self.assertRaisesRegex(RuntimeError, '28 kernel values'):
                root.check_target()

    def test_timeout_does_not_retry_or_restore_wifi(self):
        commands = []

        def shell(command, **kwargs):
            commands.append(command)
            return {'getprop ro.serialno': '2G0YC5ZF9Q01J0',
                    'settings get global wifi_on': '1',
                    'cmd wifi status': 'Wifi is disabled'}.get(command, '')

        with patch.object(root, 'shell', side_effect=shell), \
                patch.object(root, 'check_target'), \
                patch.object(root, 'has_root', return_value=False), \
                patch.object(root, 'wait_for_boot', return_value='same-boot'), \
                patch.object(root, 'attempt', return_value='timeout') as attempt:
            with self.assertRaisesRegex(RuntimeError, 'timed out'):
                root.run(self.options())
            attempt.assert_called_once()
        self.assertNotIn('svc wifi enable', commands)

    def test_success_enables_wifi_even_when_it_was_off_without_restarting_adb(self):
        commands = []

        def shell(command, **kwargs):
            commands.append(command)
            return {'getprop ro.serialno': '2G0YC5ZF9Q01J0',
                    'settings get global wifi_on': '0',
                    'cmd wifi status': 'Wifi is disabled'}.get(command, '')

        with patch.object(root, 'shell', side_effect=shell), \
                patch.object(root, 'check_target'), \
                patch.object(root, 'has_root', side_effect=[False, False, True]), \
                patch.object(root, 'wait_for_boot', return_value='same-boot'), \
                patch.object(root, 'attempt', return_value='root') as attempt:
            root.run(self.options())
            attempt.assert_called_once()
        self.assertIn('svc wifi enable', commands)
        self.assertIn('settings put global adb_wifi_enabled 1', commands)
        self.assertFalse(any('tcpip' in cmd or 'adbd' in cmd for cmd in commands))


if __name__ == '__main__':
    unittest.main()
