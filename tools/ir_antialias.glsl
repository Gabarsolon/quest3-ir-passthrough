// Spatial, luminance-only directional anti-aliasing for reconstructed IR.
// INPUT_SIZE is supplied by the launcher. Flat/low-contrast regions pass through.
uniform highp sampler2D irResolvedImage;
in highp vec2 uv;
layout(yuv) out vec4 color;

float resolveEdge(vec2 p) {
    const vec2 pixel = 1.0 / INPUT_SIZE;
    float center = texture(irResolvedImage, p).r;
    float nw = texture(irResolvedImage, p + vec2(-1.0, -1.0) * pixel).r;
    float ne = texture(irResolvedImage, p + vec2(1.0, -1.0) * pixel).r;
    float sw = texture(irResolvedImage, p + vec2(-1.0, 1.0) * pixel).r;
    float se = texture(irResolvedImage, p + vec2(1.0, 1.0) * pixel).r;
    float low = min(center, min(min(nw, ne), min(sw, se)));
    float high = max(center, max(max(nw, ne), max(sw, se)));
    if (high - low < max(1.0 / 32.0, high / 8.0)) return center;

    vec2 direction = vec2(sw + se - nw - ne, nw + sw - ne - se);
    float regularizer = max((nw + ne + sw + se) / 32.0, 1.0 / 128.0);
    direction /= min(abs(direction.x), abs(direction.y)) + regularizer;
    direction = clamp(direction, vec2(-4.0), vec2(4.0)) * pixel;
    float inner = 0.5 * (texture(irResolvedImage, p - direction / 6.0).r +
                          texture(irResolvedImage, p + direction / 6.0).r);
    float wider = inner * 0.5 + 0.25 *
        (texture(irResolvedImage, p - direction * 0.5).r +
         texture(irResolvedImage, p + direction * 0.5).r);
    return wider < low || wider > high ? inner : wider;
}

void main() {
    color = vec4(clamp(resolveEdge(uv), 0.0, 1.0), 0.5, 0.5, 1.0);
}
