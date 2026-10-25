# Headset experiments

- Keep root and runtime patches temporary. Do not flash firmware, unlock the
  bootloader, install persistent root, or write boot/system/vendor/odm partitions.
- Put new experiment binaries, frame files, and device logs in
  `/dev/quest3-ir-session` (tmpfs). Existing `/data/local/tmp` helper files survive
  reboot; temporary root and in-memory hooks do not.
- Recheck the device build, process IDs, and camera buffer mapping for each boot.
  Saved fd lists are historical snapshots. All 13 buffers per eye were active in
  the October 25 session; excluding buffers reduced the delivered rate.
- Prefer external readers and observation of the existing pipeline. Previous
  sensor-service restarts and unvalidated frame-rate function calls disrupted RGB
  passthrough. Validate the interface and recovery procedure before attempting
  such changes again.
- Keep publisher counters, changed image arrivals, viewer updates, and native
  camera completions distinct. A counter increase or a sensor datasheet maximum
  does not establish the headset's delivered frame rate. See
  `docs/session-2026-10-25.md` for the current measurements.
- The command tool runs on an isolated Windows desktop. Launching a GUI there
  does not make it visible on the user's desktop. The user launches
  `python tools/live_viewer.py`; its measurements are written once per second to
  `%TEMP%/quest3-ir-viewer/viewer-stats.json`.
- Frida CModule data is read-only. Allocate writable native globals in JavaScript
  with `Memory.alloc`, declare them `extern` in C, and pass their addresses as
  imports. Mutable static C arrays caused a diagnostic compositor crash.
