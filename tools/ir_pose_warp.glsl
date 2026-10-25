// Only our IR image moves. The compositor's camera/eye/menu poses stay intact.
// Factory IR_* constants are supplied per eye by the launcher.
uniform highp vec4 irPoseTop;
uniform highp vec4 irPoseBottom;
uniform int irEye;

vec3 rotateRay(vec4 q, vec3 ray) {
    vec3 t = 2.0 * cross(q.xyz, ray);
    return ray + q.w * t + cross(q.xyz, t);
}

vec2 projectIR(vec3 ray, mat3 rotation, vec3 projection,
               vec4 radialA, vec2 radialB, vec2 tangential, vec2 size) {
    ray = rotation * ray;
    if (ray.z <= 0.0) return vec2(-1.0);
    float radius = length(ray.xy);
    float theta = atan(radius, ray.z);
    float theta2 = theta * theta;
    float polynomial = 1.0 + theta2 * (radialA.x + theta2 * (radialA.y +
        theta2 * (radialA.z + theta2 * (radialA.w +
        theta2 * (radialB.x + theta2 * radialB.y)))));
    vec2 p = ray.xy * (theta * polynomial / max(radius, 1e-12));
    float radius2 = dot(p, p);
    vec2 d = p + tangential * (2.0 * p * p + radius2) +
        2.0 * tangential.yx * p.x * p.y;
    return (projection.x * d + projection.yz) / size;
}

vec2 warpCoordinates(vec2 p) {
    vec2 slope = texture(irMapping, p).rg;
    vec4 delta = normalize(mix(irPoseTop, irPoseBottom, clamp(p.y, 0.0, 1.0)));
    vec3 ray = rotateRay(delta, vec3(slope, 1.0));
    if (irEye == 0)
        return projectIR(ray, IR_ROTATION_0, IR_PROJECTION_0, IR_RADIAL_A_0,
                         IR_RADIAL_B_0, IR_TANGENTIAL_0, IR_SIZE_0);
    return projectIR(ray, IR_ROTATION_1, IR_PROJECTION_1, IR_RADIAL_A_1,
                     IR_RADIAL_B_1, IR_TANGENTIAL_1, IR_SIZE_1);
}
