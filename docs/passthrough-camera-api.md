# How the Passthrough Camera API works

Scope: public API observations from the tested Horizon OS v79 build
51154110174900520, rather than a claim about every later SDK/OS version.
The rooted [system IR prototype](system-passthrough.md) runs below this API.

Meta's **Passthrough Camera API** (PCA) gives apps the camera frames behind passthrough. It shipped as an experimental feature in Horizon OS **v74**. Since Meta XR Core SDK **v76** it can be used in Store apps. It works on the Quest 3 and Quest 3S.

## It is Android Camera2 underneath

The PCA has no camera stack of its own. Apps use the standard Android `CameraManager` / `CameraDevice` API, either Java/Kotlin Camera2 or the NDK `ACameraManager`, and Meta adds:

1. **Two permissions**, both runtime permissions:
   - `android.permission.CAMERA`
   - `horizonos.permission.HEADSET_CAMERA`
2. **Two vendor tags** that tell the passthrough cameras apart from the other virtual cameras:
   - `camera_source`: `0` means a passthrough camera
   - `camera_position`: `0` = left, `1` = right

Apps shouldn't hard-code camera ids. Meta's samples go through `getCameraIdList()`, read the `camera_source` and `camera_position` characteristics, and open the matching id. On the test headset those turned out to be `50` (left) and `51` (right). See [camera-inventory.md](camera-inventory.md).

Unity, Unreal and Spatial SDK wrap the same Camera2 calls. Unity's `WebCamTexture` route ends up on the same two cameras.

## What you get

- YUV_420_888 frames at 1280×960, up to 30 fps. Other sizes are available depending on the SDK sample.
- Lens intrinsics and the camera pose relative to the head, so frames can be reprojected.
- Only the RGB passthrough cameras.

## Why it can't give IR night vision

- `camera_source` has a single documented value, `0`, for the RGB passthrough cameras. No value selects the tracking cameras.
- The tracking cameras are not registered with Android's camera service, so no Camera2 call can open them.
- The RGB passthrough cameras don't respond usefully to IR. Meta forum reports say only Quest 1, Quest 2 and Rift S give an IR-capable view, and that Quest 3, 3S and Pro show colour with no IR. In a dark room under an IR lamp, the frames come out black.

Image processing on top of the PCA can't fix this. Gain and denoising can stretch dim *visible* light, but the cameras record no IR signal to amplify.

## Other capture routes, same limit

| Route | What it returns | Sees IR? |
|---|---|---|
| Passthrough Camera API (Camera2) | Raw RGB passthrough frames | No |
| Android media projection (screen capture) | Composited view, about 1024×1024, central field of view, includes virtual content | No |
| Meta's casting / recording | Composited view | No |

## References

- [Passthrough Camera API overview](https://developers.meta.com/vr/documentation/spatial-sdk/spatial-sdk-pca-overview/)
- [Android Camera2 API (Kotlin)](https://developers.meta.com/vr/documentation/native/android/pca-kotlin-documentation/)
- [Android Native Camera2 API](https://developers.meta.com/horizon/documentation/native/android/pca-native-documentation/)
- [Passthrough Camera API samples](https://developers.meta.com/horizon/documentation/android-apps/passthrough-camera-samples/)
- [Getting Started with the Quest Camera API (uralstech)](https://uralstech.github.io/2025/03/15/The-Meta-Quest-Passthrough-Camera-API.html)
