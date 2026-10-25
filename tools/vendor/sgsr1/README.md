# SGSR 1 monochrome adaptation

Adapted from Qualcomm Innovation Center's SGSR 1 GLSL edge-direction shader:

https://github.com/SnapdragonGameStudios/snapdragon-gsr/blob/main/sgsr/v1/include/glsl/sgsr1_shader_mobile_edge_direction.frag

Retrieved October 25, 2026. Upstream source copyright is 2025; the upstream
repository's BSD-3-Clause license is reproduced alongside this file.

Changes: use the R8 IR texture's red channel as luminance, expose a function
for combination with the calibrated YUV remap, enable edge direction, use
high precision and protect division by zero. Edge threshold remains 8/255;
the upstream VR contrast clamp remains +/-23/255. Sharpening defaults to 1.5
(upstream's supported range is 1 to 2). This is spatial reconstruction;
it does not recover uncaptured detail or perform temporal denoising.
