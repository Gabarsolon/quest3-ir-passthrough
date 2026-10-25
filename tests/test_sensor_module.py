"""Check mode discovery and rejection of inconsistent binary references."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from inspect_sensor_module import inspect_module


def container(dimensions):
    info = struct.pack('<IIII', 0, 0, len(dimensions), 2)
    modes = bytearray(260 * len(dimensions))
    records = [(1, 'resolutionInfo', info), (2, 'resolutionData', modes)]
    for i, (width, height) in enumerate(dimensions):
        struct.pack_into('<II', modes, i*260+20, 1, 3+i)
        stream = bytearray(116)
        struct.pack_into('<I', stream, 16, 43)
        struct.pack_into('<II', stream, 36, width, height)
        struct.pack_into('<I', stream, 52, 10)
        records.append((3+i, 'streamConfiguration', stream))
    table, symbols = 168, 192
    payload = symbols + 56*len(records)
    result = bytearray(payload)
    magic = b'QTI Chromatix Header\0'
    result[:len(magic)] = magic
    version = b'Parameter Parser V3.4.0 (2106301010)\0'
    result[40:40+len(version)] = version
    struct.pack_into('<II', result, 160, table, 2)
    struct.pack_into('<III', result, table, 0, symbols, payload-symbols)
    struct.pack_into('<III', result, table+12, 1, payload, sum(len(r[2]) for r in records))
    for i, (key, name, content) in enumerate(records):
        struct.pack_into('<I32sIIiII', result, symbols+i*56, key, name.encode(),
                         1, 0, -1, len(result)-payload, len(content))
        result.extend(content)
    struct.pack_into('<I', result, 28, len(result))
    return result


class SensorModuleTests(unittest.TestCase):
    def test_reports_actual_multiple_modes(self):
        result = inspect_module(container([(640, 513), (1280, 1024)]))
        self.assertEqual(result['declaredResolutionCount'], 2)
        self.assertEqual([(m['streams'][0]['width'], m['streams'][0]['height'])
                          for m in result['modes']], [(640, 513), (1280, 1024)])

    def test_decoy_dimensions_do_not_create_a_mode(self):
        data = container([(640, 513)])
        # An unrelated payload value resembles a larger image's dimensions.
        payload = struct.unpack_from('<I', data, 184)[0]
        struct.pack_into('<II', data, payload+16+100, 1280, 1024)
        result = inspect_module(data)
        self.assertEqual(result['declaredResolutionCount'], 1)
        self.assertEqual(result['modes'][0]['streams'][0]['width'], 640)

    def test_rejects_truncation_and_bad_references(self):
        original = container([(640, 513)])
        with self.assertRaises(ValueError):
            inspect_module(original[:-1])
        bad = original.copy()
        struct.pack_into('<I', bad, 192+48, len(bad)*2)
        with self.assertRaises(ValueError):
            inspect_module(bad)
        bad = original.copy()
        payload = struct.unpack_from('<I', bad, 184)[0]
        struct.pack_into('<I', bad, payload+12, 999)
        with self.assertRaises(ValueError):
            inspect_module(bad)

    def test_rejects_unknown_resolution_layout(self):
        data = container([(640, 513)])
        payload = struct.unpack_from('<I', data, 184)[0]
        struct.pack_into('<I', data, payload+8, 2)
        with self.assertRaises(ValueError):
            inspect_module(data)


if __name__ == '__main__':
    unittest.main()
