"""Live IR preview through the laptop's existing ADB server."""
import collections
import json
import pathlib
import threading
import time
import traceback
import tempfile
import zlib

import cv2
import numpy as np

import adb_transport as qadb

HERE = pathlib.Path(tempfile.gettempdir()) / "quest3-ir-viewer"
HERE.mkdir(exist_ok=True)
REMOTE = "/dev/quest3-ir-session/"
WINDOW = "Quest 3 IR feed"
lock = threading.Lock()
stop = threading.Event()
state = {"eye": "left", "frame": None, "error": "Connecting", "seq": 0,
         "received": 0, "received_times": collections.deque(maxlen=512)}


def reader():
    while not stop.is_set():
        with lock:
            eye = state["eye"]
        path = REMOTE + eye + ".raw"
        try:
            # Atomic rename in irfeed gives each cat a complete immutable file.
            # Keep one transport open; opening two sync sessions for every image
            # costs more than a frame interval on this connection.
            command = "exec:while true; do cat " + path + " || break; done 2>/dev/null"
            last_checksum = None
            with qadb.connect(command, timeout=3) as stream:
                while not stop.is_set():
                    raw = qadb.readn(stream, 640 * 512)
                    with lock:
                        if state["eye"] != eye:
                            break
                    checksum = zlib.crc32(raw)
                    if checksum == last_checksum:
                        continue
                    last_checksum = checksum
                    frame = np.frombuffer(raw, np.uint8).reshape(512, 640)
                    now = time.perf_counter()
                    with lock:
                        state["received"] += 1
                        state["received_times"].append(now)
                        state.update(frame=frame, error=None, seq=state["received"],
                                     timestamp=now, frame_eye=eye, checksum=checksum)
        except Exception as exc:
            with lock:
                state["error"] = str(exc)
            stop.wait(.5)


def main():
    threading.Thread(target=reader, daemon=True).start()
    cv2.namedWindow(WINDOW, cv2.WINDOW_NORMAL)
    cv2.resizeWindow(WINDOW, 640, 800)
    cv2.moveWindow(WINDOW, 80, 60)
    rotations = (cv2.ROTATE_90_CLOCKWISE, cv2.ROTATE_180,
                 cv2.ROTATE_90_COUNTERCLOCKWISE, None)
    rotation, mirror = 0, False
    reduce_flicker = True
    gain = None
    last_id = None
    arrivals = collections.deque()
    last_report = 0
    processing_times = collections.deque(maxlen=180)
    received_fps = 0
    vis = np.zeros((640, 512), dtype=np.uint8)
    status = "Waiting for frames"
    try:
        while True:
            with lock:
                current = state.copy()
                received_times = list(state["received_times"])
            now = time.perf_counter()
            identity = (current.get("frame_eye"), current.get("seq"), current.get("checksum"))
            if current["frame"] is not None and identity != last_id:
                if last_id is None or identity[0] != last_id[0]:
                    gain = None
                    arrivals.clear()
                frame = current["frame"]
                processing_start = time.perf_counter()
                # Estimate the scene's exposure without letting a saturated
                # illuminator set the white point. Recompute for each frame:
                # smoothing this gain preserves alternating exposure pulses.
                percentiles = [5, 90] if reduce_flicker else [1, 99]
                lo, hi = np.percentile(frame[::4, ::4], percentiles)
                hi = max(lo + 1, hi)
                gain = (lo, hi) if reduce_flicker or gain is None else (
                    .85 * gain[0] + .15 * lo, .85 * gain[1] + .15 * hi)
                vis = np.clip((frame.astype(np.float32) - gain[0]) * 255 /
                              max(1, gain[1] - gain[0]), 0, 255).astype(np.uint8)
                if last_id is None or identity[2] != last_id[2]:
                    arrivals.append(now)
                last_id = identity
                processing_times.append(time.perf_counter() - processing_start)
            while arrivals and arrivals[0] < now - 3:
                arrivals.popleft()
            fps = ((len(arrivals) - 1) / (arrivals[-1] - arrivals[0])
                   if len(arrivals) > 1 else 0)
            received_times = [t for t in received_times if t >= now - 3]
            received_fps = ((len(received_times) - 1) /
                            (received_times[-1] - received_times[0])
                            if len(received_times) > 1 else 0)
            if now - last_report >= 1:
                report = {"version": "stream-2", "eye": current["eye"],
                          "received_fps": round(received_fps, 2),
                          "displayed_fps": round(fps, 2),
                          "received_images": current["received"],
                          "processing_ms": round(1000 * np.mean(processing_times), 3)
                          if processing_times else 0,
                          "error": current["error"]}
                (HERE / "viewer-stats.json").write_text(json.dumps(report))
                last_report = now
            image = vis
            code = rotations[rotation]
            if code is not None:
                # vis is kept in the sensor's original orientation.
                image = cv2.rotate(image, code)
            if mirror:
                image = cv2.flip(image, 1)
            image = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
            age = now - current.get("timestamp", now)
            status = f"{current['eye'].title()} | shown {fps:.1f} fps | received {received_fps:.1f}"
            if current["error"]:
                status = current["error"][:70]
            elif age > 1:
                status += f" | paused {age:.1f}s"
            cv2.rectangle(image, (0, 0), (image.shape[1], 78), (0, 0, 0), -1)
            cv2.putText(image, status, (9, 22), cv2.FONT_HERSHEY_SIMPLEX,
                        .5, (255, 255, 255), 1, cv2.LINE_AA)
            cv2.putText(image, "1/2: camera   R: rotate   F: mirror",
                        (9, 46), cv2.FONT_HERSHEY_SIMPLEX, .43,
                        (210, 210, 210), 1, cv2.LINE_AA)
            cv2.putText(image, "S: flicker reduction " + ("ON" if reduce_flicker else "OFF") + "   Esc: close",
                        (9, 66), cv2.FONT_HERSHEY_SIMPLEX, .43,
                        (210, 210, 210), 1, cv2.LINE_AA)
            cv2.imshow(WINDOW, image)
            key = cv2.waitKey(1) & 0xff
            if key == 27 or cv2.getWindowProperty(WINDOW, cv2.WND_PROP_VISIBLE) < 1:
                break
            if key in (ord("1"), ord("2")):
                with lock:
                    state["eye"] = "left" if key == ord("1") else "right"
            elif key in (ord("r"), ord("R")):
                rotation = (rotation + 1) % 4
            elif key in (ord("f"), ord("F")):
                mirror = not mirror
            elif key in (ord("s"), ord("S")):
                reduce_flicker = not reduce_flicker
                gain = None
                last_id = None
    finally:
        stop.set()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    try:
        main()
    except Exception:
        (HERE / "viewer-error.log").write_text(traceback.format_exc())
        raise
