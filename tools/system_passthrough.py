"""Replace the Quest 3 compositor's RGB camera textures with IR, in RAM.

Temporary root must already be active. Ctrl+C, --stop, a lost host heartbeat,
or reboot restores the camera's normal RGB output. No firmware is installed.
"""

import argparse
import base64
from contextlib import contextmanager, ExitStack
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import socket
import socketserver
import subprocess
import sys
import tempfile
import threading
import time

import adb_transport as adb

HERE = Path(__file__).resolve().parent
DIRECTORY = "/dev/quest3-ir-session/system"
CONTROL_PORT = 27343
SUPPORTED_BUILD = "51154110174900520"


@contextmanager
def temporary_display_refresh(rate):
    """Keep the display override optional and restore its exact prior value."""
    if rate is None:
        yield None
        return
    if rate not in (72, 80, 90, 120):
        raise ValueError("Unsupported Quest 3 display rate")
    property_name = "debug.oculus.refreshRate"
    previous = adb.shell("getprop " + property_name).decode().strip()
    try:
        adb.shell(f"setprop {property_name} {rate}")
        actual = adb.shell("getprop " + property_name).decode().strip()
        if actual != str(rate):
            raise RuntimeError("Display refresh override was not accepted: " + repr(actual))
        yield {"requestedHz": rate, "previousOverride": previous}
    finally:
        adb.shell("setprop " + property_name + " " + shlex.quote(previous))
        restored = adb.shell("getprop " + property_name).decode().strip()
        if restored != previous:
            raise RuntimeError("Display refresh override was not restored")


def root(command, timeout=20):
    marker = "__IR_EXIT__"
    script = command + '\nresult=$?\nprintf "\\n' + marker + '%s\\n" "$result"'
    result = adb.shell("/data/local/tmp/su -c " + shlex.quote(script), timeout).decode(
        errors="replace"
    )
    output, found, status = result.rpartition(marker)
    if not found or status.strip() != "0":
        raise RuntimeError("Root command failed: " + result.strip()[-2500:])
    return output.strip()


def build_reader():
    source = HERE / "irfeed.c"
    digest = hashlib.sha256(source.read_bytes() + b"IRFEED_RAW_ONLY").hexdigest()[:16]
    cache = Path(tempfile.gettempdir()) / "quest3-ir-system"
    cache.mkdir(exist_ok=True)
    binary = cache / ("irfeed-" + digest)
    if not binary.exists():
        clang = shutil.which("clang")
        if not clang:
            candidate = Path(r"C:\Program Files\LLVM\bin\clang.exe")
            if candidate.exists():
                clang = str(candidate)
        if not clang:
            raise RuntimeError("LLVM clang is needed to build the RAM-only reader.")
        subprocess.run(
            [clang, "-target", "aarch64-linux-gnu", "-nostdlib", "-static",
             "-fno-stack-protector", "-fno-builtin", "-DIRFEED_RAW_ONLY", "-O2", "-Wl,-e,_start",
             "-Wl,--build-id=none", "-o", str(binary), str(source)],
            check=True, capture_output=True, text=True,
        )
    data = binary.read_bytes()
    if data[:4] != b"\x7fELF" or data[18:20] != b"\xb7\x00":
        raise RuntimeError("Reader is not an AArch64 ELF executable.")
    return data


def upload_ram(data, path):
    if not path.startswith(DIRECTORY + "/"):
        raise ValueError("Refusing a device write outside the RAM session directory.")
    encoded = base64.b64encode(data).decode()
    if len(encoded) > 45000:
        raise RuntimeError("Reader is too large for this bounded upload method.")
    digest = hashlib.sha256(data).hexdigest()
    temporary = path + ".new"
    root("printf %s " + shlex.quote(encoded) + " | base64 -d > " + shlex.quote(temporary))
    actual = root("sha256sum " + shlex.quote(temporary)).split()[0]
    if actual != digest:
        raise RuntimeError("RAM executable checksum did not match.")
    root("chmod 0755 " + shlex.quote(temporary) + " && mv "
         + shlex.quote(temporary) + " " + shlex.quote(path))


def cleanup_orphan_readers():
    # The exclusive host control port is held before this runs. A crashed host
    # may leave its RAM readers behind; only terminate this launcher's exact
    # executable and output paths, after verifying the process's executable.
    rows = root("ps -A -o PID,ARGS")
    pattern = (r"\s*(\d+) \S*irfeed \d+ " + re.escape(DIRECTORY)
               + r"/(?:left|right)\.raw(?: \d+)+\s*")
    for line in rows.splitlines():
        match = re.fullmatch(pattern, line)
        if match:
            pid = int(match[1])
            executable = root(f"readlink /proc/{pid}/exe")
            if executable.removesuffix(" (deleted)") == DIRECTORY + "/irfeed":
                stop_owned_process(pid, DIRECTORY + "/irfeed")


def discover_buffers(pid):
    listing = root(f"ls -l /proc/{pid}/fd")
    fds = []
    for line in listing.splitlines():
        if "dmabuf" not in line.lower():
            continue
        match = re.search(r"\s(\d+)\s+->", line)
        if match:
            fds.append(int(match[1]))
    if not fds:
        raise RuntimeError("No exported camera DMA buffers found.")
    paths = " ".join(f"/proc/{pid}/fd/{fd}" for fd in fds)
    rows = root("stat -L -c '%n %s %i' " + paths)
    buffers = []
    for line in rows.splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[1] == "327680":
            buffers.append((int(fields[2]), int(fields[0].rsplit("/", 1)[1])))
    buffers.sort()
    if len(buffers) != 26 or len({inode for inode, _ in buffers}) != 26:
        raise RuntimeError(f"Expected 26 distinct 640x512 buffers; found {len(buffers)}.")
    groups = [buffers[:13], buffers[13:]]
    # The two 13-buffer pools were physically identified on this firmware.
    # Validate their contiguous allocation pattern again instead of reusing fds.
    for group in groups:
        if any(b[0] - a[0] != 2 for a, b in zip(group, group[1:])):
            raise RuntimeError("Camera allocation pattern changed; refusing guessed eye mapping.")
    return [[fd for _, fd in group] for group in groups]


def start_reader(pid, eye, fds):
    out = f"{DIRECTORY}/{eye}.raw"
    command = (f"setsid {DIRECTORY}/irfeed {pid} {out} "
               + " ".join(map(str, fds))
               + f" </dev/null >{DIRECTORY}/{eye}.log 2>&1 &\necho $!")
    return int(root(command).splitlines()[-1])


def stop_owned_process(pid, executable):
    if not pid:
        return
    try:
        root(f"if [ -r /proc/{pid}/cmdline ]; then\n"
             f"case \"$(cat /proc/{pid}/cmdline | tr '\\000' ' ')\" in\n"
             + "*" + shlex.quote(executable) + f"*) kill {pid};;\nesac\nfi")
    except Exception as exc:
        print("Process cleanup:", exc, file=sys.stderr)


def control(command, **arguments):
    with socket.create_connection(("127.0.0.1", CONTROL_PORT), timeout=3) as sock:
        sock.sendall((json.dumps({"command": command, **arguments}) + "\n").encode())
        with sock.makefile("rb") as stream:
            return json.loads(stream.readline(100000))


class ControlHandler(socketserver.StreamRequestHandler):
    def handle(self):
        self.connection.settimeout(3)
        try:
            request = json.loads(self.rfile.readline(4096))
            if request.get("command") == "stop":
                self.server.state["status"] = "stopping"
                self.server.stop_event.set()
            elif request.get("command") == "delay":
                value = request.get("delay_ms")
                if type(value) is not int or not 0 <= value <= 200:
                    raise ValueError("Delay must be an integer from 0 to 200 ms")
                self.server.state["requested_delay_ms"] = value
            elif request.get("command") == "pose_age":
                value = request.get("age_ms")
                if type(value) is not int or not 0 <= value <= 100:
                    raise ValueError("Pose age must be an integer from 0 to 100 ms")
                self.server.state["requested_pose_age_ms"] = value
            elif request.get("command") == "filter":
                value = request.get("filter")
                if value not in ["linear", "cubic", "sgsr"]:
                    raise ValueError("Filter must be linear, cubic or sgsr")
                self.server.state["requested_filter"] = value
            self.wfile.write((json.dumps(self.server.state) + "\n").encode())
        except (ValueError, OSError):
            return


def run(options):
    import frida

    stop = threading.Event()
    state = {"status": "starting", "host_pid": os.getpid(), "serial": adb.SERIAL}
    try:
        server = socketserver.ThreadingTCPServer(("127.0.0.1", CONTROL_PORT), ControlHandler)
    except OSError as exc:
        raise RuntimeError("Another controller may be running; use --status or --stop.") from exc
    server.daemon_threads = True
    server.stop_event, server.state = stop, state
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    readers, frida_pid, session, script = [], None, None, None
    mode_session, mode_script = None, None
    keepalive_stop = threading.Event()
    keepalive_scripts = []
    log_dir = Path(tempfile.gettempdir()) / "quest3-ir-system"
    log_dir.mkdir(exist_ok=True)
    log = (log_dir / "system-passthrough.jsonl").open("a", encoding="utf-8")

    def message(item, data):
        log.write(json.dumps({"time": time.time(), **item}) + "\n")
        log.flush()
        payload = item.get("payload", {})
        if item.get("type") == "error":
            state.update(status="error", error=item.get("description", str(item)))
            print("Hook error:", item.get("description", item), flush=True)
            stop.set()
        elif payload.get("kind") == "error":
            state.update(status="error", error=payload.get("error"))
            print("Hook error:", payload.get("error"), flush=True)
            stop.set()
        elif payload.get("kind") == "ready":
            print("Compositor hook ready; waiting for IR to be drawn.", flush=True)

    def keepalive():
        # Discovery and calibration transfer can exceed the watchdog interval
        # over Wi-Fi. Keep both scripts alive throughout startup and normal use.
        while not keepalive_stop.wait(0.5):
            try:
                for active in tuple(keepalive_scripts):
                    active.exports_sync.heartbeat()
            except Exception as exc:
                if not keepalive_stop.is_set():
                    state.update(status="error", error="Heartbeat failed: " + str(exc))
                    print("Heartbeat failed:", exc, flush=True)
                    stop.set()
                return

    keeper = threading.Thread(target=keepalive, daemon=True)
    keeper.start()
    display_settings = ExitStack()
    try:
        build = adb.shell("getprop ro.build.version.incremental").decode().strip()
        if build != SUPPORTED_BUILD:
            raise RuntimeError(f"This hook was validated on {SUPPORTED_BUILD}; connected build is {build}.")
        if "uid=0(root)" not in root("id"):
            raise RuntimeError("Temporary root is not active.")
        state["display_refresh"] = display_settings.enter_context(
            temporary_display_refresh(options.display_refresh))
        if state["display_refresh"]:
            print(f"Temporary display request: {options.display_refresh} Hz; restored on stop.", flush=True)
        adb.shell("am broadcast -a com.oculus.vrpowermanager.prox_close\ninput keyevent KEYCODE_WAKEUP")
        mounts = root("cat /proc/mounts")
        if not any(line.split()[1:3] == ["/dev", "tmpfs"] for line in mounts.splitlines()):
            raise RuntimeError("/dev is not tmpfs; refusing to create session files.")
        root("mkdir -p " + DIRECTORY + "\nchmod 0755 " + DIRECTORY)
        cleanup_orphan_readers()
        upload_ram(build_reader(), DIRECTORY + "/irfeed")
        calibration_maps = None
        camera_poses = None
        warp_parameters = None
        if not options.no_calibration:
            from ir_calibration import make_maps, camera_pose_parameters, image_warp_parameters
            calibration = json.loads(root("cat /persist/calibration/camera_calibration.json"))
            image_warp = options.pose_correction and options.pose_method == "image"
            calibration_maps = make_maps(calibration, rays_only=image_warp)
            camera_poses = camera_pose_parameters(calibration)
            if image_warp:
                warp_parameters = image_warp_parameters(calibration)
            print("Factory IR/RGB ray maps calculated for both eyes.", flush=True)
        adb.forward("tcp:27042", "tcp:27042")
        manager = frida.get_device_manager()
        device = manager.add_remote_device("127.0.0.1:27042")
        compositor = int(adb.shell("pidof com.oculus.vrruntimeservice").decode().strip())
        try:
            session = device.attach(compositor)
        except (frida.ServerNotRunningError, frida.TransportError):
            target = DIRECTORY + "/frida-server"
            root("cp /data/local/tmp/frida-server " + target + "\nchmod 0755 " + target)
            version = root(target + " --version")
            if version != frida.__version__:
                raise RuntimeError(f"Frida versions differ: host {frida.__version__}, device {version}.")
            frida_pid = int(root(f"TMPDIR={DIRECTORY} setsid {target} -l 127.0.0.1:27042 "
                                f"</dev/null >{DIRECTORY}/frida.log 2>&1 &\necho $!").splitlines()[-1])
            manager.remove_remote_device("127.0.0.1:27042")
            device = manager.add_remote_device("127.0.0.1:27042")
            for attempt in range(20):
                try:
                    session = device.attach(compositor)
                    break
                except (frida.ServerNotRunningError, frida.TransportError):
                    time.sleep(0.2)
            if session is None:
                raise RuntimeError("RAM Frida server did not start.")

        if not options.no_fast_mode:
            fidelity_pid = int(adb.shell("pidof trackingfidelityservice").decode().strip())
            mode_session = device.attach(fidelity_pid)
            mode_script = mode_session.create_script((HERE / "tracking_mode.js").read_text())
            mode_script.on("message", message)
            mode_script.load()
            dump = adb.shell("dumpsys oculus.internal.ITrackingFidelityService/default").decode()
            match = re.search(r"worldTracking Translator:.*?MuxMode = (\w+)", dump, re.S)
            if not match:
                raise RuntimeError("Could not read the original tracking mode.")
            frequency = None if options.frequency == "auto" else int(options.frequency)
            state["tracking_mode"] = mode_script.exports_sync.activate(match[1], frequency)
            keepalive_scripts.append(mode_script)
            print("Tracking mode:", state["tracking_mode"], flush=True)
            time.sleep(0.4)

        sensors = root("pidof vendor.oculus.hardware.sensors@1.0-service").split()
        if len(sensors) != 1:
            raise RuntimeError("Could not identify one sensors service.")
        sensor_pid = int(sensors[0])
        groups = discover_buffers(sensor_pid)
        print(f"Fresh buffer mapping: sensors PID {sensor_pid}, 13 buffers per eye.", flush=True)
        for eye, fds in zip(["left", "right"], groups):
            readers.append(start_reader(sensor_pid, eye, fds))
        deadline = time.monotonic() + 30
        previous = {eye: None for eye in ["left", "right"]}
        advances = dict.fromkeys(previous, 0)
        print("Waiting for both IR publishers to advance before replacing RGB.", flush=True)
        while min(advances.values()) < 2:
            if stop.is_set():
                raise KeyboardInterrupt
            if mode_script:
                mode_script.exports_sync.heartbeat()
            for eye in previous:
                try:
                    sequence = adb.pull(f"{DIRECTORY}/{eye}.raw.rgba.seq")
                    if len(sequence) == 8:
                        if previous[eye] is not None and sequence != previous[eye]:
                            advances[eye] += 1
                        previous[eye] = sequence
                except RuntimeError:
                    pass
            if time.monotonic() > deadline:
                raise RuntimeError("Both IR publishers did not start advancing; RGB remains available.")
            time.sleep(0.1)
        for eye in previous:
            if len(adb.pull(f"{DIRECTORY}/{eye}.raw")) != 327680:
                raise RuntimeError(f"The {eye} IR reader did not produce a complete frame.")

        settings = {"directory": DIRECTORY, "rotate": options.rotate,
                    "mirror": options.mirror, "swapEyes": options.swap_eyes,
                    "normalize": not options.no_normalize, "zoom": options.zoom,
                    "stretch": options.stretch, "calibration": calibration_maps is not None,
                    "outputSize": list(map(int, options.output_size.split("x"))),
                    "irDelayMs": options.ir_delay_ms,
                    "poseCorrection": options.pose_correction, "poseAgeMs": options.pose_age_ms,
                    "poseMethod": options.pose_method,
                    "filter": options.filter,
                    "sgsrSharpness": options.sgsr_sharpness,
                    "antialiasing": options.aa,
                    "cameraPoses": camera_poses, "warpParameters": warp_parameters}
        source = ("const OPTIONS=" + json.dumps(settings) + ";\nconst C_SOURCE="
                  + json.dumps((HERE / "system_passthrough.c").read_text()) + ";\n"
                  + "const SGSR_SOURCE=" + json.dumps((HERE / "vendor" / "sgsr1" / "mono.glsl").read_text()) + ";\n"
                  + "const AA_SOURCE=" + json.dumps((HERE / "ir_antialias.glsl").read_text()) + ";\n"
                  + "const WARP_SOURCE=" + json.dumps((HERE / "ir_pose_warp.glsl").read_text()) + ";\n"
                  + (HERE / "system_passthrough.js").read_text())
        script = session.create_script(source)
        script.on("message", message)
        script.load()
        keepalive_scripts.append(script)
        if calibration_maps:
            for eye, mapping in enumerate(calibration_maps):
                for offset in range(0, len(mapping), 128 * 1024):
                    if stop.is_set():
                        raise RuntimeError(state.get("error", "Startup was stopped."))
                    script.exports_sync.setmapchunk(eye, offset, mapping[offset:offset + 128 * 1024])
        print("Keep normal Quest passthrough enabled. Ctrl+C or --stop restores RGB.", flush=True)
        started = time.monotonic()
        announced = False
        prior_sequences = None
        stale_checks = 0
        last_reader_check = time.monotonic()
        while not stop.wait(0.5):
            requested_delay = state.get("requested_delay_ms", options.ir_delay_ms)
            if requested_delay != options.ir_delay_ms:
                options.ir_delay_ms = requested_delay
                script.exports_sync.setdelay(options.ir_delay_ms)
            requested_age = state.get("requested_pose_age_ms", options.pose_age_ms)
            if requested_age != options.pose_age_ms:
                options.pose_age_ms = requested_age
                script.exports_sync.setposeage(options.pose_age_ms)
            requested_filter = state.get("requested_filter", options.filter)
            if requested_filter != options.filter:
                options.filter = requested_filter
                script.exports_sync.setfilter(options.filter)
            if mode_script:
                state["tracking_mode"].update(mode_script.exports_sync.status())
            stats = script.exports_sync.stats()
            state.update(stats)
            if not stats["enabled"] or stats["errors"]:
                raise RuntimeError("The hook stopped; RGB capture is taking over.")
            if stats["substitutedDraws"] and not announced:
                announced = True
                state["status"] = "IR active"
                print("IR ACTIVE in the shared system passthrough textures.", flush=True)
            if not announced:
                state["status"] = "waiting for normal passthrough"
            if time.monotonic() - last_reader_check > 3:
                sequences = tuple(adb.pull(f"{DIRECTORY}/{eye}.raw.rgba.seq") for eye in ["left", "right"])
                if (any(len(seq) != 8 for seq in sequences)
                        or (prior_sequences is not None
                            and any(a == b for a, b in zip(sequences, prior_sequences)))):
                    stale_checks += 1
                    if stale_checks >= 2:
                        raise RuntimeError("IR readers stopped updating; restoring RGB.")
                else:
                    stale_checks = 0
                prior_sequences = sequences
                last_reader_check = time.monotonic()
            if options.seconds and time.monotonic() - started >= options.seconds:
                break
    except KeyboardInterrupt:
        pass
    finally:
        state["status"] = "restoring RGB"
        keepalive_stop.set()
        keeper.join(timeout=2)
        if script:
            try:
                script.exports_sync.stop()
                for _ in range(20):
                    if script.exports_sync.stats()["cleaned"]:
                        break
                    time.sleep(0.05)
            except Exception as exc:
                print("Hook cleanup:", exc, file=sys.stderr)
            try:
                script.unload()
            except Exception:
                pass
        if session:
            try:
                session.detach()
            except Exception:
                pass
        if mode_script:
            try:
                mode_script.exports_sync.restore()
                mode_script.unload()
            except Exception as exc:
                print("Tracking mode cleanup:", exc, file=sys.stderr)
        if mode_session:
            try:
                mode_session.detach()
            except Exception:
                pass
        for pid in readers:
            stop_owned_process(pid, DIRECTORY + "/irfeed")
        stop_owned_process(frida_pid, DIRECTORY + "/frida-server")
        try:
            display_settings.close()
        except Exception as exc:
            print("Display refresh cleanup:", exc, file=sys.stderr)
        log.close()
        state["status"] = "stopped"
        server.shutdown()
        server.server_close()
        print("IR hook removed. Normal RGB capture is restored.", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_mutually_exclusive_group()
    commands.add_argument("--stop", action="store_true")
    commands.add_argument("--status", action="store_true")
    commands.add_argument("--set-delay", type=int, metavar="MS", help="Change the running controller's experimental frame delay.")
    commands.add_argument("--set-pose-age", type=int, metavar="MS", help="Change the running IR pose timestamp estimate.")
    commands.add_argument("--set-filter", choices=["linear", "cubic", "sgsr"], help="Change the running controller's IR reconstruction filter.")
    parser.add_argument("--serial", default=adb.SERIAL)
    parser.add_argument("--ir-delay-ms", type=int, default=0, help="Experimental frame delay, 0–200 ms; default 0.")
    parser.add_argument("--seconds", type=float, default=0, help="Stop automatically after this duration; default runs until Ctrl+C.")
    parser.add_argument("--rotate", type=int, choices=[0, 90, 180, 270], default=90,
                        help="Clockwise image rotation, default 90 degrees.")
    parser.add_argument("--mirror", action="store_true")
    parser.add_argument("--swap-eyes", action="store_true")
    parser.add_argument("--no-fast-mode", action="store_true", help="Keep the original tracking camera schedule.")
    poses = parser.add_mutually_exclusive_group()
    poses.add_argument("--pose-correction", dest="pose_correction", action="store_true",
                       help="Use historical IR capture poses (default with calibration).")
    poses.add_argument("--no-pose-correction", dest="pose_correction", action="store_false",
                       help="Disable historical pose correction for diagnostics.")
    parser.set_defaults(pose_correction=None)
    parser.add_argument("--pose-method", choices=["image", "shadow", "direct"], default="image",
                        help="Default image corrects only our IR pixels. Legacy shadow/direct modes caused rendering artifacts.")
    parser.add_argument("--pose-age-ms", type=int, default=8, help="Estimated exposure age before the last observed DMA change; default 8 ms.")
    parser.add_argument("--frequency", choices=["auto", "50", "60"], default="60",
                        help="Temporary utility frequency for fast hands mode; restored on stop. Default 60 Hz.")
    parser.add_argument("--display-refresh", type=int, choices=[72, 80, 90, 120], default=None,
                        help="Optional temporary display rate, restored on stop. 120 gives even pacing for 60 Hz IR; camera rate is unchanged.")
    parser.add_argument("--output-size", choices=["640x512", "1280x960", "2328x1748"], default="1280x960",
                        help="GPU remap texture size; source IR remains 640x512. Default 1280x960.")
    parser.add_argument("--filter", choices=["linear", "cubic", "sgsr"], default="linear",
                        help="IR reconstruction filter; default linear. Cubic requires GL_IMG_texture_filter_cubic.")
    parser.add_argument("--sgsr-sharpness", type=float, default=1.5,
                        help="SGSR 1 edge sharpness, 1 to 2; default 1.5.")
    parser.add_argument("--aa", choices=["off", "edge"], default="off",
                        help="Optional directional edge anti-aliasing after IR reconstruction.")
    parser.add_argument("--no-calibration", action="store_true", help="Use the earlier rotated/cropped view instead of factory ray maps.")
    parser.add_argument("--no-normalize", action="store_true")
    parser.add_argument("--stretch", action="store_true", help="Use the whole sensor image instead of preserving its aspect ratio.")
    parser.add_argument("--zoom", type=float, default=1.0)
    options = parser.parse_args()
    if options.pose_correction is None:
        options.pose_correction = not options.no_calibration
    if options.seconds < 0 or not 0.25 <= options.zoom <= 5:
        parser.error("seconds must be nonnegative; zoom must be between 0.25 and 5")
    if not 1 <= options.sgsr_sharpness <= 2:
        parser.error("SGSR sharpness must be between 1 and 2")
    if not 0 <= options.ir_delay_ms <= 200 or (options.set_delay is not None and not 0 <= options.set_delay <= 200):
        parser.error("delay must be between 0 and 200 ms")
    if not 0 <= options.pose_age_ms <= 100 or (options.set_pose_age is not None and not 0 <= options.set_pose_age <= 100):
        parser.error("pose age must be between 0 and 100 ms")
    if options.pose_correction and options.no_calibration:
        parser.error("pose correction requires factory calibration")
    adb.SERIAL = options.serial
    try:
        if options.set_filter is not None:
            result = control("filter", filter=options.set_filter)
            print("Requested IR filter:", result["requested_filter"])
        elif options.set_pose_age is not None:
            result = control("pose_age", age_ms=options.set_pose_age)
            print("Requested IR pose age:", result["requested_pose_age_ms"], "ms")
        elif options.set_delay is not None:
            result = control("delay", delay_ms=options.set_delay)
            print("Requested IR frame delay:", result["requested_delay_ms"], "ms")
        elif options.stop or options.status:
            result = control("stop" if options.stop else "status")
            if options.stop:
                for _ in range(40):
                    time.sleep(0.1)
                    try:
                        result = control("status")
                    except OSError:
                        print("Controller stopped; normal RGB capture is restored.")
                        break
                else:
                    print(json.dumps(result, indent=2))
            else:
                print(json.dumps(result, indent=2))
        else:
            run(options)
        return 0
    except (RuntimeError, OSError, ValueError, subprocess.CalledProcessError) as exc:
        print("Error:", exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
