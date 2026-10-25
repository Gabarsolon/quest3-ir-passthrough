"""Offline native-report date/truncation checks. No headset access."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from inspect_tombstones import summarize_dropbox


def varint(value):
    result = bytearray()
    while value >= 128:
        result.append((value & 127) | 128)
        value >>= 7
    return bytes(result) + bytes([value])


def field(number, value):
    if isinstance(value, int):
        return varint(number << 3) + varint(value)
    if isinstance(value, str):
        value = value.encode()
    return varint((number << 3) | 2) + varint(len(value)) + value


def fixture(truncate=False):
    frame = field(4, 'crashing_function') + field(1, 1234) + field(6, '/example.so')
    thread = field(1, 42) + field(4, frame)
    tomb = (field(4, '2026-10-12 00:27:26+0300') + field(5, 42) + field(6, 42)
            + field(9, '/example') + field(10, field(2, 'SIGABRT'))
            + field(14, 'an old crash') + field(16, field(1, 42) + field(2, thread)))
    if truncate:
        # Android's DropBox dump caps a payload inside the native wrapper.
        blob = field(1, tomb + field(17, b'memory' * 100))[:len(tomb) + 10]
    else:
        blob = field(1, tomb)
    return field(1, field(1, 1792956488356) + field(2, blob))


class TombstoneTests(unittest.TestCase):
    def test_preserves_import_and_internal_dates(self):
        report, = summarize_dropbox(fixture())
        self.assertTrue(report['dropboxTimeUtc'].startswith('2026-10-25'))
        self.assertTrue(report['tombstoneTime'].startswith('2026-10-12'))
        self.assertEqual(report['process'], ['/example'])
        self.assertEqual(report['backtrace'][0]['function'], 'crashing_function')
        self.assertFalse(report['truncated'])

    def test_recovers_header_without_hiding_truncation(self):
        report, = summarize_dropbox(fixture(truncate=True))
        self.assertTrue(report['truncated'])
        self.assertEqual(report['abort'], 'an old crash')
        self.assertEqual(report['backtrace'][0]['relativePc'], 1234)

    def test_rejects_corrupt_outer_message(self):
        for data in [b'\x0a\xff', b'\x0a\x01', b'\x00', b'\x08\x01',
                     b'\x08' + b'\xff' * 10]:
            with self.subTest(data=data), self.assertRaises(ValueError):
                summarize_dropbox(data)

    def test_empty_inventory(self):
        self.assertEqual(summarize_dropbox(b''), [])


if __name__ == '__main__':
    unittest.main()
