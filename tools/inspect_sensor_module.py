"""Inspect the tested v79 QTI sensor-module binary without loading a driver.

Offline only: python tools/inspect_sensor_module.py path/to/sensormodule.bin
Supports the Parameter Parser V3.4.0 resolution records observed on this Quest.
Rejects unknown record layouts instead of guessing dimensions from byte patterns.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def inspect_module(data):
    def bounds(offset, size):
        if offset < 0 or size < 0 or offset + size > len(data):
            raise ValueError("Section or record exceeds the file")

    bounds(0, 168)
    if not data.startswith(b"QTI Chromatix Header\0"):
        raise ValueError("Not a QTI Chromatix container")
    if struct.unpack_from('<I', data, 28)[0] != len(data):
        raise ValueError("Declared file size does not match")
    parser = data[40:88].split(b'\0', 1)[0].decode('ascii')
    if not parser.startswith('Parameter Parser V3.4.0 '):
        raise ValueError("Untested parameter-parser version: " + parser)
    table, count = struct.unpack_from('<II', data, 160)
    if not 1 <= count <= 32:
        raise ValueError("Invalid section count")
    bounds(table, count * 12)
    sections = {}
    for index in range(count):
        kind, offset, size = struct.unpack_from('<III', data, table + index * 12)
        bounds(offset, size)
        if kind in sections:
            raise ValueError("Duplicate section")
        sections[kind] = (offset, size)
    if 0 not in sections or 1 not in sections:
        raise ValueError("Missing symbols or data section")
    symbols, symbol_size = sections[0]
    payload, payload_size = sections[1]
    if symbol_size % 56:
        raise ValueError("Unknown symbol record size")
    records = {}
    for offset in range(symbols, symbols + symbol_size, 56):
        key, name, major, minor, parent, start, size = struct.unpack_from('<I32sIIiII', data, offset)
        if start + size > payload_size or key in records:
            raise ValueError("Invalid symbol bounds or duplicate symbol")
        if (major, minor) != (1, 0):
            raise ValueError("Untested symbol version")
        records[key] = (name.split(b'\0', 1)[0].decode('ascii'),
                        data[payload + start:payload + start + size])

    def record(key, name):
        if key not in records or records[key][0] != name:
            raise ValueError("Missing or mismatched " + name + " reference")
        return records[key][1]

    infos = [value for name, value in records.values() if name == 'resolutionInfo']
    if len(infos) != 1 or len(infos[0]) != 16:
        raise ValueError("Unknown resolution-info layout")
    _, _, mode_count, mode_ref = struct.unpack('<IIII', infos[0])
    if not 1 <= mode_count <= 128:
        raise ValueError("Invalid resolution count")
    modes = record(mode_ref, 'resolutionData')
    if len(modes) != mode_count * 260:
        raise ValueError("Unknown resolution-data layout")
    result = []
    for index in range(mode_count):
        stream_count, stream_ref = struct.unpack_from('<II', modes, index * 260 + 20)
        if not 1 <= stream_count <= 32:
            raise ValueError("Invalid stream count")
        streams = record(stream_ref, 'streamConfiguration')
        if len(streams) != stream_count * 116:
            raise ValueError("Unknown stream-configuration layout")
        dimensions = []
        for stream in range(stream_count):
            start = stream * 116
            width, height = struct.unpack_from('<II', streams, start + 36)
            if not 1 <= width <= 20000 or not 1 <= height <= 20000:
                raise ValueError("Invalid image dimensions")
            dimensions.append(dict(width=width, height=height,
                csiDataType=struct.unpack_from('<I', streams, start + 16)[0],
                bitsPerPixel=struct.unpack_from('<I', streams, start + 52)[0]))
        result.append(dict(modeIndex=index, streams=dimensions))
    return dict(sha256=hashlib.sha256(data).hexdigest(), parser=parser,
                declaredResolutionCount=mode_count, modes=result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    options = parser.parse_args()
    try:
        result = inspect_module(options.binary.read_bytes())
    except (ValueError, OSError, UnicodeError, struct.error) as exc:
        parser.exit(1, str(exc) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
