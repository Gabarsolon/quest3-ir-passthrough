"""Package the native IR controller into a small Android Start/Stop app."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

from build_headset import CACHE, ROOT, download


def run(*args):
    subprocess.run([str(x) for x in args], check=True)


def main():
    sdk = CACHE / 'apk-tools'
    sdk.mkdir(parents=True, exist_ok=True)
    packages = [
        ('platform-34-ext7_r03.zip', '1f2e9478d6a7601425ceaa553311dc43191f103d', {'android.jar': 'android.jar'}),
        ('build-tools_r35_windows.zip', 'af059bb67cf7786f45ee0db85e2d24985df1b4b6',
         {'aapt2.exe': 'aapt2.exe', 'lib/d8.jar': 'd8.jar', 'lib/apksigner.jar': 'apksigner.jar', 'zipalign.exe': 'zipalign.exe'}),
    ]
    for filename, digest, wanted in packages:
        if all((sdk / name).exists() for name in wanted.values()):
            continue
        archive = CACHE / filename
        download('https://dl.google.com/android/repository/' + filename, archive, 'sha1', digest)
        with zipfile.ZipFile(archive) as source:
            for suffix, name in wanted.items():
                matches = [x for x in source.namelist() if x.endswith('/' + suffix)]
                if len(matches) != 1:
                    raise RuntimeError('Unexpected SDK archive: ' + suffix)
                (sdk / name).write_bytes(source.read(matches[0]))
    java_bin = Path(shutil.which('java')).parent
    output = ROOT / 'build' / 'headset'
    classes = output / 'classes'; dex = output / 'dex'
    classes.mkdir(exist_ok=True); dex.mkdir(exist_ok=True)
    hashes = {name: hashlib.sha256((output / name).read_bytes()).hexdigest() for name in ['ir-headset', 'irfeed']}
    info = output / 'BuildInfo.java'
    info.write_text('package com.gabarsolon.questir; final class BuildInfo {\n'
                    + f' static final String CONTROLLER_SHA256="{hashes["ir-headset"]}";\n'
                    + f' static final String READER_SHA256="{hashes["irfeed"]}";\n' + '}\n', encoding='utf-8')
    run(java_bin / 'javac.exe', '-source', '8', '-target', '8', '-classpath', sdk / 'android.jar',
        '-d', classes, ROOT / 'headset/app/src/com/gabarsolon/questir/MainActivity.java',
        ROOT / 'headset/app/src/com/gabarsolon/questir/SmokeTest.java', info)
    jar = output / 'app-classes.jar'
    run(java_bin / 'jar.exe', 'cf', jar, '-C', classes, '.')
    run(java_bin / 'java.exe', '-cp', sdk / 'd8.jar', 'com.android.tools.r8.D8',
        '--lib', sdk / 'android.jar', '--min-api', '26', '--output', dex, jar)
    unsigned = output / 'quest-ir-unsigned.apk'
    run(sdk / 'aapt2.exe', 'link', '-I', sdk / 'android.jar', '--manifest',
        ROOT / 'headset/app/AndroidManifest.xml', '-o', unsigned)
    with zipfile.ZipFile(unsigned, 'a', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as target:
        target.write(dex / 'classes.dex', 'classes.dex')
        for name in hashes:
            target.write(output / name, 'assets/' + name)
        target.write(ROOT / 'headset/FRIDA-LICENSE.txt', 'assets/licenses/frida-core.txt')
        target.write(ROOT / 'LICENSE', 'assets/licenses/quest-ir.txt')
    aligned = output / 'quest-ir-aligned.apk'
    run(sdk / 'zipalign.exe', '-f', '4', unsigned, aligned)
    keystore = CACHE / 'quest-ir-development.p12'
    if not keystore.exists():
        run(java_bin / 'keytool.exe', '-genkeypair', '-keystore', keystore, '-storetype', 'PKCS12',
            '-storepass', 'android', '-alias', 'quest-ir-dev', '-keyalg', 'RSA', '-keysize', '2048',
            '-validity', '3650', '-dname', 'CN=Quest IR Development')
    target = ROOT / 'dist' / 'quest-ir-headset-dev.apk'
    target.parent.mkdir(exist_ok=True)
    run(java_bin / 'java.exe', '-jar', sdk / 'apksigner.jar', 'sign', '--ks', keystore,
        '--ks-pass', 'pass:android', '--ks-key-alias', 'quest-ir-dev', '--out', target, aligned)
    run(java_bin / 'java.exe', '-jar', sdk / 'apksigner.jar', 'verify', '--verbose', target)
    print(f'{target}: {target.stat().st_size:,} bytes')


if __name__ == '__main__':
    main()
