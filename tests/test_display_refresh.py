"""Verify display restoration on normal stop, failed startup, and exceptions."""
from pathlib import Path
import shlex
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from system_passthrough import temporary_display_refresh


class DisplayRefreshTests(unittest.TestCase):
    def test_no_option_does_not_touch_headset(self):
        with patch('system_passthrough.adb.shell') as shell:
            with temporary_display_refresh(None) as value:
                self.assertIsNone(value)
            shell.assert_not_called()

    def exercise(self, previous, *, exception=False, reject=False):
        value = previous
        writes = []

        def shell(command):
            nonlocal value
            parts = shlex.split(command)
            self.assertEqual(parts[1], 'debug.oculus.refreshRate')
            if parts[0] == 'setprop':
                writes.append(parts[2])
                if not (reject and len(writes) == 1):
                    value = parts[2]
                return b''
            self.assertEqual(parts[0], 'getprop')
            return (value + '\n').encode()

        with patch('system_passthrough.adb.shell', side_effect=shell):
            if exception or reject:
                with self.assertRaises(RuntimeError):
                    with temporary_display_refresh(120):
                        raise RuntimeError('startup/render failure')
            else:
                with temporary_display_refresh(120) as state:
                    self.assertEqual(value, '120')
                    self.assertEqual(state['previousOverride'], previous)
            self.assertEqual(value, previous)
            self.assertEqual(writes, ['120', previous])

    def test_normal_stop_restores_empty_and_explicit_values(self):
        for previous in ['', '90', '120']:
            with self.subTest(previous=previous):
                self.exercise(previous)

    def test_exception_restores_original(self):
        self.exercise('', exception=True)

    def test_rejected_override_restores_original(self):
        self.exercise('90', reject=True)


if __name__ == '__main__':
    unittest.main()
