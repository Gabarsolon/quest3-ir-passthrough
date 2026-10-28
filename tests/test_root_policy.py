"""Decode raw policy masks rather than treating Permissive as an allow rule."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from inspect_root_policy import decode_decision


class PolicyDecisionTests(unittest.TestCase):
    def test_denial_stays_denied_with_a_permissive_subject_flag(self):
        result = decode_decision('0 ffffffff 0 ffffffff 7 1', {'open': 19, 'read': 2})
        self.assertEqual(result['permissions'], {'open': False, 'read': False})
        self.assertEqual(result['flags'], 1)

    def test_one_based_permission_numbers_map_to_correct_bits(self):
        result = decode_decision('40002 ffffffff 0 0 7 0', {'open': 19, 'read': 2, 'write': 3})
        self.assertEqual(result['permissions'], {'open': True, 'read': True, 'write': False})

    def test_incomplete_reply_rejected(self):
        with self.assertRaises(RuntimeError):
            decode_decision('ffff', {'read': 2})


if __name__ == '__main__':
    unittest.main()
