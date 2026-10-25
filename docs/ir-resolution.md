# IR capture resolution on the tested Quest 3

**Result: no existing 1280×1024 IR stream was found on build 51154110174900520.**
The working source remains 640×512 per eye. Both the live native HAL and the
shipped sensor configuration describe a 640×513 RAW mode for cameras 0/1.
This is a limit of the configuration inspected here, not proof that the sensor
hardware cannot operate at full resolution.

## Different sizes in the pipeline

| Stage | Front IR cameras 0/1 | What it establishes |
|---|---|---|
| Sensor specification | 1280×1024, up to 120 FPS | Bare OG01A1B capability, not a Quest driver mode |
| Factory calibration `ImageSize` | 1280×1024 | Coordinates used by the factory camera model |
| Live HAL RAW capabilities and configured stream | 640×513 | Size currently advertised/selected by the native camera stack |
| Exported mono8 DMA image read by this project | 640×512, 327680 bytes | Actual image pixels available to the reader |
| Our owned remap texture | 1280×960 by default | Calibrated/enlarged output, not additional captured detail |

The other monochrome pair, cameras 2/3 (OV7251), uses 640×481 at the HAL/module
level and exports 640×480 images. Those are different cameras from the pair used
for the current passthrough view.

The extra HAL row is observed in both pairs. Its exact purpose was not
established in this audit; do not interpret it as an extra visible image row.
Likewise, this work has not established whether the half-size image comes from
sensor binning, subsampling, or another stage. The factory calibration size and
sensor datasheet alone cannot answer that question.

[OMNIVISION's OG01A1B specification](https://www.ovt.com/products/og01a1b/)
lists 1280×1024/120 FPS and 640×480/240 FPS. Neither establishes an available
640×512 Quest mode at those rates.

## Live HAL inspection

On October 25, 2026, with the accepted IR controller still running:

1. Rechecked root, build, boot ID, and the sensors/tracking process IDs.
2. Enumerated distinct DMA-buffer inodes held by those processes. Sensors held
   26 buffers of 327680 bytes and 26 of 307200 bytes, among larger RGB/processing
   allocations. A large buffer by itself does not identify a full-resolution IR
   image: dimensions, format, stride, camera identity and changing contents must
   also match.
3. Checked the current `libxrcamerahal.so` SHA-256 against the locally analyzed
   library before attaching a bounded read-only probe to the sensors process.
4. Observed the existing `XrCameraConfiguration` objects during ordinary buffer
   accounting. Read their sensor metadata and configured stream vectors; no
   configure/start/stop function was invoked.
5. Detached the probe and verified that the existing IR controller remained
   active with zero reported renderer errors.

All six camera configuration objects were captured. For the two front IR
cameras, the RAW format entries 32, 36, 37, 38, and vendor format 291 were all
640×513. No entry for those cameras advertised 1280×1024, including in their
non-RAW list. Cameras 2/3 listed RAW at 640×481. The RGB pair separately listed
RAW at 2328×1748; their larger buffers are not a high-resolution IR source.
Standard RAW format numbers are defined in
[AOSP's pixel-format declarations](https://android.googlesource.com/platform/hardware/interfaces/+/refs/heads/main/graphics/common/1.0/types.hal).

Relevant native layout evidence came from these functions in the exact library:

- `getMaxStreamResolution` at `+0x32820` walks the sensor's advertised format,
  width and height records.
- `getMatchingStreamResolution` at `+0x32700` requires an exact advertised
  format/width/height match; a missing combination reaches a fatal assertion.
- `addStream` at `+0x32220` uses that lookup for explicit dimensions or selects
  the largest advertised entry when dimensions are omitted.

These offsets and object layouts are specific to the checked binary. The
resolution evidence is stronger than merely finding a literal number in memory.

## Shipped sensor-module inspection

Copied the four `/vendor/etc/camera/com.qti.sensormodule.*` monochrome module
files to the host without changing the originals. Each file uses the QTI
Parameter Parser V3.4.0 container. Its symbol table references a `resolutionInfo`
record, a counted `resolutionData` array, and stream configuration records.

The offline [inspector](../tools/inspect_sensor_module.py) follows those
references and validates bounds and the tested record sizes. It reports:

| Modules | Declared resolution entries | Stream dimensions | Declared bit depth |
|---|---:|---|---:|
| OG01A1B left / right | 1 each | 640×513 | 10 |
| OV7251 left / right | 1 each | 640×481 | 10 |

No second full-resolution entry was present. Unrelated values that happen to
encode 1280 or 1024 are not treated as image dimensions. Tests include a
synthetic two-mode module, decoy dimensions, invalid references, truncation and
inconsistent layouts. To inspect a host copy:

```powershell
python .\tools\inspect_sensor_module.py .\path\to\sensor-module.bin
python -B .\tests\test_sensor_module.py
```

The module binaries are not committed. The
[evidence JSON](../evidence/ir-resolution-2026-10-25.json) records their hashes,
decoded mode dimensions, native HAL capabilities and active input/output sizes.

## What a full-resolution implementation would require

There is no existing larger IR stream in the inspected configuration to select
with `--output-size` or by reading more bytes from the current DMA buffers.
Advertising 1280×1024 in metadata alone would not create the corresponding
sensor/CSI/ISP output. A working experiment needs a validated full-resolution
sensor mode, matching capture/buffer configuration, and a recovery procedure,
followed by measurements of actual new image detail and delivered cadence.

The reader, upload dimensions, image history, optional filters and source-size
assumptions would also need to accept the larger input. Calibration would need
validation for that mode. The current capture path is intentionally still the
accepted 640×512 / roughly 60 images/sec configuration, with a 120 Hz display
request. No sensor restart, resolution change, service restart, firmware update
or partition write was performed during this investigation.
