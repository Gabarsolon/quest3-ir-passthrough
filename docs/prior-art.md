# Prior art: night or IR passthrough on Quest headsets

## Stock Quest 3 / 3S / Pro passthrough background

- **Meta Community Forums, "IR Night vision mode in Quest 3".** Users confirm that Quest 3 passthrough doesn't show IR. One poster with a camera diagram says the headset's IR-capable cameras are monochrome tracking cameras and the colour passthrough cameras can't see IR. Another says only Quest 1, Quest 2 and Rift S give an IR-capable view, and Quest 3, 3S and Pro show colour. Several report that an IR illuminator noticeably improves *tracking* in the dark while the view stays dark. A claim in the thread that IR view was "removed for legal reasons" is unsourced.
  <https://communityforums.atmeta.com/discussions/OtherTroubleshooting/ir-night-vision-mode-in-quest-3/1332645>

- **Julian Triveri, media-projection capture.** Android's screen-capture API works on Quest headsets. It returns about 1024×1024 frames of the central field of view, *composited*: passthrough plus virtual content. That is RGB passthrough again, not tracking-camera data.
  <https://www.uploadvr.com/quest-3-raw-camera-access-better-workaround/>

- **Passthrough Camera API launch coverage (UploadVR).** The API covers the RGB passthrough cameras only.
  <https://www.uploadvr.com/quest-passthrough-camera-api-experimental-out-now/>
  <https://www.uploadvr.com/quest-passthrough-camera-api-now-shippable-on-store/>

- **IR illuminator accessories** sold for Quest are marketed for hand and body *tracking* in dim rooms, not for seeing in the dark.

## Headsets where IR passthrough does work

- **Quest 1 / Quest 2 / Rift S.** Passthrough came from the monochrome tracking cameras, so an IR lamp lit up the passthrough view.
- **PSVR2.** Passthrough comes from the monochrome tracking cameras, so it works with an IR illuminator. On PC, PSVR2Toolkit's *Room View* (`v1.0.0-experimental`) exposes that feed to SteamVR camera clients.

## Below-Android access

This project now reads the tracking-camera DMA buffers with temporary root on
one v79 Quest 3 and performs a RAM-only substitution in the shared compositor.
See [system passthrough](system-passthrough.md). This is a firmware-specific
prototype, distinct from supported app-level camera access. These background
references do not establish the current state of every external project.
