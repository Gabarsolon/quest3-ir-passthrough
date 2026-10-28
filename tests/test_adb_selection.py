"""Validate saved-wireless fallback without selecting an unrelated device."""
import io
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import adb_transport as adb


class Reply(io.BytesIO):
    def recv(self, count):
        return self.read(count)


class TargetSelectionTests(unittest.TestCase):
    def select(self, listing, explicit=None):
        encoded = listing.encode()
        reply = Reply(f'{len(encoded):04x}'.encode() + encoded)
        with patch.object(adb, 'SERIAL', 'paired-quest'), patch.object(adb, 'connect', return_value=reply):
            return adb.resolve_serial(explicit)

    def test_saved_wireless_preferred_and_offline_usb_ignored(self):
        self.assertEqual(self.select('paired-quest device\n2G0YC5ZF9Q01J0 device\n'), 'paired-quest')
        with self.assertRaises(RuntimeError):
            self.select('paired-quest offline\n2G0YC5ZF9Q01J0 unauthorized\n')

    def test_missing_wireless_falls_back_to_same_quest(self):
        self.assertEqual(self.select('2G0YC5ZF9Q01J0 device product:eureka\n'), '2G0YC5ZF9Q01J0')
        with self.assertRaises(RuntimeError):
            self.select('other-headset device\n')

    def test_explicit_target_never_silently_substituted(self):
        with self.assertRaises(RuntimeError):
            self.select('2G0YC5ZF9Q01J0 device\n', 'explicit-wifi')
        self.assertEqual(self.select('explicit-wifi device\n', 'explicit-wifi'), 'explicit-wifi')


if __name__ == '__main__':
    unittest.main()
