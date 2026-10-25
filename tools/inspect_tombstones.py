"""Summarize an Android DropBox native-tombstone protobuf on the host.

No device access or third-party protobuf runtime. Omits registers, memory dumps,
and open files. A DropBox import date is not necessarily the crash date; both
dates are retained. Truncated payloads are labeled, never treated as complete.

Field numbers: AOSP debuggerd/proto/tombstone.proto and DropBox dump schema.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path


def _varint(data, offset):
    value = 0
    for index in range(10):
        if offset >= len(data):
            raise ValueError('Incomplete protobuf varint')
        byte = data[offset]
        offset += 1
        if index == 9 and byte > 1:
            raise ValueError('Protobuf varint exceeds 64 bits')
        value |= (byte & 127) << (7 * index)
        if not byte & 128:
            return value, offset
    raise ValueError('Invalid protobuf varint')


def _fields(data, allow_truncated=False):
    if not isinstance(data, bytes):
        raise ValueError('Expected protobuf message bytes')
    fields, offset, truncated = {}, 0, False
    while offset < len(data):
        try:
            key, offset = _varint(data, offset)
            number, wire = key >> 3, key & 7
            if not 0 < number < (1 << 29):
                raise ValueError('Invalid protobuf field number')
            if wire == 0:
                value, offset = _varint(data, offset)
            elif wire in (1, 2, 5):
                if wire == 2:
                    size, offset = _varint(data, offset)
                else:
                    size = 8 if wire == 1 else 4
                end = offset + size
                if end > len(data):
                    if not allow_truncated or wire != 2:
                        raise ValueError('Incomplete protobuf field')
                    truncated = True
                value = data[offset:min(end, len(data))]
                offset = end
            else:
                raise ValueError('Unsupported protobuf wire type')
        except ValueError as exc:
            if allow_truncated and str(exc).startswith('Incomplete'):
                return fields, True
            raise
        fields.setdefault(number, []).append(value)
    return fields, truncated


def _first(fields, number, default=None):
    return fields.get(number, [default])[0]


def _text(fields, number):
    value = _first(fields, number, b'')
    return _decode_text(value)


def _decode_text(value):
    if not isinstance(value, bytes):
        raise ValueError('Expected protobuf string')
    return value[:65536].decode('utf-8', errors='replace')


def _nested(value):
    if not isinstance(value, bytes):
        raise ValueError('Expected nested protobuf message')
    return _fields(value, allow_truncated=True)[0]


def summarize_dropbox(data):
    outer, _ = _fields(data)
    summaries = []
    for index, raw in enumerate(outer.get(1, [])):
        entry, _ = _fields(raw)
        blob = _first(entry, 2, b'')
        wrapper, truncated = _fields(blob, allow_truncated=True)
        payload = _first(wrapper, 1)
        if not isinstance(payload, bytes):
            raise ValueError('Missing native tombstone payload')
        tomb, inner_truncated = _fields(payload, allow_truncated=True)
        timestamp = _first(entry, 1)
        if not isinstance(timestamp, int):
            raise ValueError('Missing DropBox timestamp')
        try:
            imported = datetime.fromtimestamp(timestamp / 1000, timezone.utc).isoformat()
        except (OverflowError, OSError, ValueError) as exc:
            raise ValueError('Invalid DropBox timestamp') from exc
        signal = _nested(_first(tomb, 10, b''))
        tid = _first(tomb, 6)
        frames = []
        for raw_thread in tomb.get(16, []):
            thread_entry = _nested(raw_thread)
            if _first(thread_entry, 1) != tid:
                continue
            thread = _nested(_first(thread_entry, 2, b''))
            for raw_frame in thread.get(4, [])[:24]:
                frame = _nested(raw_frame)
                frames.append(dict(function=_text(frame, 4),
                    functionOffset=_first(frame, 5), file=_text(frame, 6),
                    relativePc=_first(frame, 1), buildId=_text(frame, 8)))
        summaries.append(dict(index=index, dropboxTimeUtc=imported,
            tombstoneTime=_text(tomb, 4), pid=_first(tomb, 5), tid=tid,
            process=[_decode_text(value) for value in tomb.get(9, [])],
            signal=_text(signal, 2), abort=_text(tomb, 14),
            uptimeSeconds=_first(tomb, 20), backtrace=frames,
            truncated=truncated or inner_truncated,
            sha256=hashlib.sha256(payload).hexdigest()))
    return summaries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    options = parser.parse_args()
    try:
        summaries = summarize_dropbox(options.input.read_bytes())
    except (OSError, ValueError) as exc:
        parser.exit(1, str(exc) + '\n')
    print(json.dumps(summaries, indent=2))


if __name__ == '__main__':
    main()
