"""Prevent a second controller from patching a compositor already owned by IR."""
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import system_passthrough as launcher


class ControllerOwnershipTests(unittest.TestCase):
    def test_absent_or_stopped_native_controller_allows_laptop(self):
        for state in ['', '{"status":"stopped"}']:
            with self.subTest(state=state), patch.object(launcher, 'root', return_value=state):
                launcher.check_native_controller()

    def test_startup_active_and_cleanup_all_block_a_second_controller(self):
        for state in ['starting', 'IR active', 'restoring RGB']:
            with self.subTest(state=state), patch.object(launcher, 'root', return_value=json.dumps({'status': state})):
                with self.assertRaisesRegex(RuntimeError, 'Headset-native IR is running'):
                    launcher.check_native_controller()

    def test_bad_status_is_not_treated_as_safe_to_inject(self):
        with patch.object(launcher, 'root', return_value='broken response'):
            with self.assertRaises(ValueError):
                launcher.check_native_controller()


if __name__ == '__main__':
    unittest.main()
