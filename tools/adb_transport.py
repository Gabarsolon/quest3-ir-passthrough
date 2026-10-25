"""Use the existing local ADB server without launching another adb client."""
import argparse
import os
import pathlib
import socket
import struct
import sys
import time

TARGET_FILE = pathlib.Path(os.environ.get("LOCALAPPDATA", pathlib.Path.home() / ".config")) / "quest3-ir" / "adb-target.txt"


def saved_serial():
    try:
        return TARGET_FILE.read_text(encoding="utf-8").strip()
    except FileNotFoundError:
        return ""


# Host-only preference; no headset property is changed. Explicit launch --serial
# still overrides this default, and the environment can select a temporary target.
SERIAL = os.environ.get("QUEST3_ADB_SERIAL") or saved_serial() or "2G0YC5ZF9Q01J0"


def readn(sock, count):
    result = bytearray()
    while len(result) < count:
        chunk = sock.recv(count - len(result))
        if not chunk:
            raise EOFError("ADB connection closed")
        result.extend(chunk)
    return bytes(result)


def request(sock, command):
    data = command.encode()
    sock.sendall(f"{len(data):04x}".encode() + data)
    status = readn(sock, 4)
    if status != b"OKAY":
        raise RuntimeError(readn(sock, int(readn(sock, 4), 16)).decode(errors="replace"))


def connect(service, timeout=15, transport=True):
    sock = socket.create_connection(("127.0.0.1", 5037), timeout=5)
    sock.settimeout(timeout)
    if transport:
        request(sock, "host:transport:" + SERIAL)
    request(sock, service)
    return sock


def shell(command, timeout=15):
    with connect("shell:" + command, timeout) as sock:
        chunks = []
        while True:
            chunk = sock.recv(65536)
            if not chunk:
                return b"".join(chunks)
            chunks.append(chunk)


def pull(remote, local=None):
    with connect("sync:") as sock:
        path = remote.encode()
        sock.sendall(b"RECV" + struct.pack("<I", len(path)) + path)
        chunks = []
        while True:
            kind, length = struct.unpack("<4sI", readn(sock, 8))
            if kind == b"DONE":
                break
            data = readn(sock, length)
            if kind == b"FAIL":
                raise RuntimeError(data.decode(errors="replace"))
            if kind != b"DATA":
                raise RuntimeError("Unexpected sync response: " + repr(kind))
            chunks.append(data)
    result = b"".join(chunks)
    if local is not None:
        pathlib.Path(local).write_bytes(result)
    return result


def push(data, remote, mode=0o644):
    if isinstance(data, (str, pathlib.Path)):
        data = pathlib.Path(data).read_bytes()
    with connect("sync:") as sock:
        path = f"{remote},{0o100000 | mode}".encode()
        sock.sendall(b"SEND" + struct.pack("<I", len(path)) + path)
        for offset in range(0, len(data), 65536):
            chunk = data[offset:offset + 65536]
            sock.sendall(b"DATA" + struct.pack("<I", len(chunk)) + chunk)
        sock.sendall(b"DONE" + struct.pack("<I", int(time.time())))
        status, length = struct.unpack("<4sI", readn(sock, 8))
        if status != b"OKAY":
            raise RuntimeError(readn(sock, length).decode(errors="replace"))


def forward(local, remote):
    with socket.create_connection(("127.0.0.1", 5037), timeout=5) as sock:
        request(sock, f"host-serial:{SERIAL}:forward:{local};{remote}")
        if readn(sock, 4) != b"OKAY":
            raise RuntimeError("ADB forwarding failed")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--serial", default=SERIAL)
    parser.add_argument("--timeout", type=float, default=15)
    parser.add_argument("command", choices=["shell", "pull", "push", "forward"])
    parser.add_argument("args", nargs="+")
    opts = parser.parse_args()
    SERIAL = opts.serial
    if opts.command == "shell":
        with connect("shell:" + " ".join(opts.args), opts.timeout) as sock:
            while True:
                data = sock.recv(65536)
                if not data:
                    break
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()
    elif opts.command == "pull":
        data = pull(*opts.args)
        print(f"Pulled {len(data)} bytes")
    elif opts.command == "push":
        push(*opts.args)
    else:
        forward(*opts.args)
