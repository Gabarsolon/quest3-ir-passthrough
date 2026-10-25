// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// Monochrome adaptation: see README.md and LICENSE in this directory.

float sgsrLanczos2(float x) {
    float a = x - 4.0;
    float b = x * a - a;
    return b * a * a;
}

vec2 sgsrWeight(float dx, float dy, float value, vec3 data) {
    float edgeDistance = dx * data.z + dy * data.y;
    float distance = dx * dx + dy * dy + edgeDistance * edgeDistance *
        (clamp(value * value * data.x, 0.0, 1.0) * 0.7 - 1.0);
    float weight = sgsrLanczos2(distance);
    return vec2(weight, weight * value);
}

vec2 sgsrEdgeDirection(vec4 left, vec4 right) {
    float a = right.x - left.z;
    float b = right.w - left.y;
    vec2 delta = vec2(a + b, a - b);
    return delta * inversesqrt(dot(delta, delta) + 3.075740e-05);
}

float sgsrLuma(vec2 uv) {
    const vec2 size = vec2(640.0, 512.0);
    const vec2 stepSize = 1.0 / size;
    float center = textureLod(irImage, uv, 0.0).r;
    vec2 image = uv * size + vec2(-0.5, 0.5);
    vec2 pixel = floor(image);
    vec2 coord = pixel * stepSize;
    vec2 fraction = image - pixel;
    vec4 left = textureGather(irImage, coord, 0);
    float edgeVote = abs(left.z-left.y) + abs(center-left.y) + abs(center-left.z);
    if (edgeVote <= 8.0 / 255.0) return center;

    coord.x += stepSize.x;
    vec4 right = textureGather(irImage, coord + vec2(stepSize.x, 0.0), 0);
    vec4 upDown;
    upDown.xy = textureGather(irImage, coord + vec2(0.0, -stepSize.y), 0).wz;
    upDown.zw = textureGather(irImage, coord + vec2(0.0, stepSize.y), 0).yx;
    float mean = (left.y + left.z + right.x + right.w) * 0.25;
    left -= mean;
    right -= mean;
    upDown -= mean;
    float relativeCenter = center - mean;
    float sum = dot(abs(left) + abs(right) + abs(upDown), vec4(1.0));
    float inverseSum = 10.14185 / max(sum, 0.0001);
    vec3 data = vec3(inverseSum * inverseSum, sgsrEdgeDirection(left, right));

    vec2 weights = sgsrWeight(fraction.x, fraction.y+1.0, upDown.x, data);
    weights += sgsrWeight(fraction.x-1.0, fraction.y+1.0, upDown.y, data);
    weights += sgsrWeight(fraction.x-1.0, fraction.y-2.0, upDown.z, data);
    weights += sgsrWeight(fraction.x, fraction.y-2.0, upDown.w, data);
    weights += sgsrWeight(fraction.x+1.0, fraction.y-1.0, left.x, data);
    weights += sgsrWeight(fraction.x, fraction.y-1.0, left.y, data);
    weights += sgsrWeight(fraction.x, fraction.y, left.z, data);
    weights += sgsrWeight(fraction.x+1.0, fraction.y, left.w, data);
    weights += sgsrWeight(fraction.x-1.0, fraction.y-1.0, right.x, data);
    weights += sgsrWeight(fraction.x-2.0, fraction.y-1.0, right.y, data);
    weights += sgsrWeight(fraction.x-2.0, fraction.y, right.z, data);
    weights += sgsrWeight(fraction.x-1.0, fraction.y, right.w, data);
    if (abs(weights.x) < 0.00001) return center;
    float finalValue = weights.y / weights.x;
    float low = min(min(left.y, left.z), min(right.x, right.w));
    float high = max(max(left.y, left.z), max(right.x, right.w));
    float delta = clamp(SGSR_SHARPNESS * finalValue, low, high) - relativeCenter;
    delta = clamp(delta, -23.0 / 255.0, 23.0 / 255.0);
    return clamp(center + delta, 0.0, 1.0);
}
