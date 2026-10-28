"""Build the standalone Android ARM64 IR controller with a pinned Frida devkit."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
CACHE = Path(os.environ.get('LOCALAPPDATA', Path.home() / '.cache')) / 'quest3-ir' / 'build'
DEVKIT_SHA256 = '0c688824a24949e7be77109fd2c7306bba60cb47f608789e141bb24cd23b1005'
NDK_SHA1 = '9bf167a1985fa7d4a036186b78f702eab9179408'


def download(url, path, algorithm, expected):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists():
        incoming = path.with_suffix(path.suffix + '.part')
        print('Downloading ' + url, flush=True)
        with urllib.request.urlopen(url, timeout=60) as source, incoming.open('wb') as out:
            shutil.copyfileobj(source, out)
        incoming.replace(path)
    with path.open('rb') as stream:
        actual = hashlib.file_digest(stream, algorithm).hexdigest()
    if actual != expected:
        raise RuntimeError(f'Checksum mismatch: {path}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ndk', type=Path)
    parser.add_argument('--devkit', type=Path)
    parser.add_argument('--download', action='store_true', help='Fetch pinned official build dependencies if missing.')
    options = parser.parse_args()
    ndk = options.ndk or CACHE / 'android-ndk-r30'
    devkit = options.devkit or CACHE / 'frida-core-17.24.3'
    if options.download:
        archive = devkit / 'frida-core-devkit-17.24.3-android-arm64.tar.xz'
        download('https://github.com/frida/frida/releases/download/17.24.3/' + archive.name,
                 archive, 'sha256', DEVKIT_SHA256)
        if not (devkit / 'frida-core.h').exists():
            with tarfile.open(archive) as source:
                source.extractall(devkit, filter='data')
        if not (ndk / 'source.properties').exists():
            archive = CACHE / 'android-ndk-r30-windows.zip'
            download('https://dl.google.com/android/repository/' + archive.name, archive, 'sha1', NDK_SHA1)
            with zipfile.ZipFile(archive) as source:
                source.extractall(CACHE)
    host = 'windows-x86_64' if os.name == 'nt' else 'linux-x86_64'
    compiler = ndk / 'toolchains' / 'llvm' / 'prebuilt' / host / 'bin' / ('clang.exe' if os.name == 'nt' else 'clang')
    if not compiler.exists() or not (devkit / 'libfrida-core.a').exists():
        raise RuntimeError('NDK / Frida devkit missing. Use --download on Windows or supply --ndk and --devkit.')
    output = ROOT / 'build' / 'headset'
    output.mkdir(parents=True, exist_ok=True)
    assets = dict(c='system_passthrough.c', contexts='system_contexts.c', js='system_passthrough.js',
                  sgsr='vendor/sgsr1/mono.glsl', aa='ir_antialias.glsl', warp='ir_pose_warp.glsl', mode='tracking_mode.js')
    header = ''.join('static const char asset_' + name + '[] = ' + json.dumps(
        (ROOT / 'tools' / path).read_text(encoding='utf-8'), ensure_ascii=True) + ';\n' for name, path in assets.items())
    (output / 'assets.h').write_text(header, encoding='utf-8')
    subprocess.run([str(compiler), '--target=aarch64-linux-android24', '-DANDROID', '-O2',
                    '-ffunction-sections', '-fdata-sections', '-Wall', '-Wextra',
                    '-I' + str(devkit), '-I' + str(output),
                    str(ROOT / 'headset/controller.c'), str(ROOT / 'headset/calibration.c'),
                    '-o', str(output / 'ir-headset'), '-L' + str(devkit), '-lfrida-core',
                    '-llog', '-ldl', '-lm', '-latomic', '-pthread',
                    '-Wl,--export-dynamic', '-Wl,--gc-sections', '-Wl,-z,max-page-size=16384', '-Wl,-s'], check=True)
    from system_passthrough import build_reader
    (output / 'irfeed').write_bytes(build_reader())
    for name in ['ir-headset', 'irfeed']:
        path = output / name
        print(f'{path}: {path.stat().st_size:,} bytes, SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()}')


if __name__ == '__main__':
    main()
