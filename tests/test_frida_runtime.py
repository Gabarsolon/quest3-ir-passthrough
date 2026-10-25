"""Check RAM server staging and rejection of the old ARM64 hook runtime."""
import hashlib
import io
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import system_passthrough as launcher


class FridaRuntimeTests(unittest.TestCase):
    def test_old_runtime_rejected_before_device_access(self):
        with patch.dict(sys.modules, {'frida': SimpleNamespace(__version__='17.17.0')}), \
                patch.object(launcher.adb, 'shell') as shell, \
                patch.object(launcher, 'root') as root:
            with self.assertRaisesRegex(RuntimeError, '17.23.0 or newer'):
                launcher.run(SimpleNamespace())
            shell.assert_not_called()
            root.assert_not_called()

    def test_matching_ram_server_is_reused_without_userdata_changes(self):
        with patch.object(launcher, 'root', return_value='17.24.3') as root, \
                patch.object(launcher.adb, 'push') as push:
            self.assertEqual(launcher.prepare_frida_server(None, '17.24.3'),
                             launcher.DIRECTORY + '/frida-server')
            self.assertEqual(root.call_count, 1)
            push.assert_not_called()

    def test_upload_checks_hash_and_only_stops_owned_ram_daemon(self):
        binary = bytearray(32)
        binary[:5] = b'\x7fELF\x02'
        binary[18:20] = b'\xb7\x00'
        digest = hashlib.sha256(binary).hexdigest()
        commands, upload_services = [], []
        target = launcher.DIRECTORY + '/frida-server'

        def root(command):
            commands.append(command)
            if command.startswith('if [ -x '):
                return '17.17.0'
            if command.startswith('if [ -f '):
                return 'oldhash ' + target
            if command.startswith('pidof '):
                return '7878 9999'
            if command.startswith('setsid /system/bin/toybox nc '):
                self.assertIn('-s 127.0.0.1 -p 27344 -l -w 10', command)
                return '7070'
            if command == 'readlink /proc/7878/exe':
                return target
            if command == 'readlink /proc/9999/exe':
                return '/data/local/tmp/frida-server'
            if command.startswith('sha256sum '):
                return digest + ' ' + command.split()[1]
            if command == target + ' --version':
                return '17.24.3'
            return ''

        class Stream:
            def __enter__(inner):
                inner.responses = io.BytesIO(b'IR_UPLOAD_checktoken\nIR_UPLOAD_OK')
                return inner

            def __exit__(inner, *args):
                return False

            def sendall(inner, data):
                self.assertEqual(data, binary)

            def recv(inner, count):
                return inner.responses.read(count)

        def connect(service, timeout):
            self.assertEqual(service, 'tcp:27344')
            self.assertEqual(timeout, 30)
            upload_services.append(service)
            return Stream()

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'server'
            path.write_bytes(binary)
            with patch.object(launcher, 'root', side_effect=root), \
                    patch.object(launcher.adb, 'connect', side_effect=connect), \
                    patch.object(launcher.secrets, 'token_hex', return_value='checktoken'), \
                    patch.object(launcher, 'stop_owned_process') as stop:
                self.assertEqual(launcher.prepare_frida_server(path, '17.24.3'), target)
                stop.assert_called_once_with(7878, target)
        self.assertEqual(len(upload_services), 1)
        self.assertTrue(any(command.startswith('rm -f ' + launcher.DIRECTORY + '/upload-') for command in commands))
        self.assertFalse(any('adbd' in command or 'tcpip' in command for command in commands))
        self.assertFalse(any(command.startswith('cp ') for command in commands))


if __name__ == '__main__':
    unittest.main()
