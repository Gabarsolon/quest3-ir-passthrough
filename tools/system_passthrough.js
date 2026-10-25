'use strict';
// OPTIONS and C_SOURCE are supplied by system_passthrough.py.
// Camera buffers are read only. The compositor samples our own YUV images.
const gl = Process.getModuleByName('libGLESv3.so');
const egl = Process.getModuleByName('libEGL.so');
const nw = Process.getModuleByName('libnativewindow.so');
const libc = Process.getModuleByName('libc.so');
const tracking = Process.getModuleByName('libtrackingserviceclients.so');
const native = (m, name, ret, args) => new NativeFunction(m.getExportByName(name), ret, args);
const g = (name, ret, args) => native(gl, name, ret, args);
const getProc = native(egl, 'eglGetProcAddress', 'pointer', ['pointer']);
const extension = (name, ret, args) => {
    const address = getProc(Memory.allocUtf8String(name));
    if (address.isNull()) throw Error('Missing EGL extension ' + name);
    return new NativeFunction(address, ret, args);
};

const G = {
    string: g('glGetString', 'pointer', ['uint']),
    genTex: g('glGenTextures', 'void', ['int', 'pointer']),
    delTex: g('glDeleteTextures', 'void', ['int', 'pointer']),
    bindTex: g('glBindTexture', 'void', ['uint', 'uint']),
    param: g('glTexParameteri', 'void', ['uint', 'uint', 'int']),
    texImage: g('glTexImage2D', 'void', ['uint', 'int', 'int', 'int', 'int', 'int', 'uint', 'uint', 'pointer']),
    imageTarget: g('glEGLImageTargetTexture2DOES', 'void', ['uint', 'pointer']),
    genFbo: g('glGenFramebuffers', 'void', ['int', 'pointer']),
    delFbo: g('glDeleteFramebuffers', 'void', ['int', 'pointer']),
    shader: g('glCreateShader', 'uint', ['uint']),
    source: g('glShaderSource', 'void', ['uint', 'int', 'pointer', 'pointer']),
    compile: g('glCompileShader', 'void', ['uint']),
    shaderiv: g('glGetShaderiv', 'void', ['uint', 'uint', 'pointer']),
    shaderlog: g('glGetShaderInfoLog', 'void', ['uint', 'int', 'pointer', 'pointer']),
    delShader: g('glDeleteShader', 'void', ['uint']),
    program: g('glCreateProgram', 'uint', []),
    attachShader: g('glAttachShader', 'void', ['uint', 'uint']),
    link: g('glLinkProgram', 'void', ['uint']),
    programiv: g('glGetProgramiv', 'void', ['uint', 'uint', 'pointer']),
    programlog: g('glGetProgramInfoLog', 'void', ['uint', 'int', 'pointer', 'pointer']),
    delProgram: g('glDeleteProgram', 'void', ['uint']),
    location: g('glGetUniformLocation', 'int', ['uint', 'pointer'])
};
const allocate = native(nw, 'AHardwareBuffer_allocate', 'int', ['pointer', 'pointer']);
const release = native(nw, 'AHardwareBuffer_release', 'void', ['pointer']);
const getDisplay = native(egl, 'eglGetCurrentDisplay', 'pointer', []);
const nativeBuffer = extension('eglGetNativeClientBufferANDROID', 'pointer', ['pointer']);
const createImage = extension('eglCreateImageKHR', 'pointer', ['pointer', 'pointer', 'uint', 'pointer', 'pointer']);
const destroyImage = extension('eglDestroyImageKHR', 'uint', ['pointer', 'pointer']);
const scratch = Memory.alloc(32), descriptor = Memory.alloc(40), attrs = Memory.alloc(12);
const storage = Memory.alloc(16384);
storage.writeByteArray(new Uint8Array(16384));
const poseRecords = Memory.alloc(1024 * 144);
poseRecords.writeByteArray(new Uint8Array(1024 * 144));
const sources = ['left', 'right'].map(eye => ({
    eye, path: Memory.allocUtf8String(OPTIONS.directory + '/' + eye + '.raw'),
    sequence: Memory.allocUtf8String(OPTIONS.directory + '/' + eye + '.raw.rgba.seq'),
    stamp: Memory.allocUtf8String(OPTIONS.directory + '/' + eye + '.raw.stamp'),
    pixels: Memory.alloc(640 * 512 * 17), upload: 0, yuv: 0, map: 0, mapping: null,
    ahb: ptr(0), image: ptr(0)
}));
let program = 0, framebuffer = 0, display = ptr(0), api;
let irDelayMs = OPTIONS.irDelayMs;
let poseTracker = ptr(0);
const createPoseTracker = native(tracking, 'createHeadTrackingFbs', 'pointer', ['ulong']);
const destroyPoseTracker = native(tracking, 'destroyHeadTrackingFbs', 'void', ['ulong', 'pointer']);

function compileShader(type, text) {
    const id = G.shader(type), source = Memory.allocUtf8String(text), array = Memory.alloc(8);
    array.writePointer(source);
    G.source(id, 1, array, ptr(0));
    G.compile(id);
    G.shaderiv(id, 0x8b81, scratch);
    if (scratch.readS32() !== 1) {
        const log = Memory.alloc(8192);
        G.shaderlog(id, 8192, ptr(0), log);
        G.delShader(id);
        throw Error(log.readUtf8String());
    }
    return id;
}

function textureParameters(target) {
    G.param(target, 0x2801, 0x2601);
    G.param(target, 0x2800, 0x2601);
    G.param(target, 0x2802, 0x812f);
    G.param(target, 0x2803, 0x812f);
}

function initialize() {
    if (OPTIONS.calibration && sources.some(s => s.mapping === null)) return;
    try {
        if (OPTIONS.calibration && !G.string(0x1f03).readUtf8String().includes('GL_OES_texture_float_linear'))
            throw Error('Float calibration maps require GL_OES_texture_float_linear');
        const vs = compileShader(0x8b31, '#version 300 es\nout highp vec2 uv;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);uv=p;gl_Position=vec4(p*2.0-1.0,0.0,1.0);}');
        // Camera texture coordinates invert the visible direction of rotation.
        const transforms = ['', 'p=vec2(p.y,1.0-p.x);', 'p=vec2(1.0-p.x,1.0-p.y);', 'p=vec2(1.0-p.y,p.x);'];
        const transform = transforms[OPTIONS.rotate / 90] + (OPTIONS.mirror ? 'p.x=1.0-p.x;' : '');
        const crop = OPTIONS.stretch ? [1, 1] : (OPTIONS.rotate % 180 === 90 ? [0.6006872852, 1] : [1, 0.9385738832]);
        const fit = 'p=(p-0.5)*vec2(' + crop.map(v => v.toFixed(8)).join(',') + ')+0.5;';
        const coordinates = OPTIONS.calibration
            ? 'p=texture(irMapping,p).rg;'
            : transform + fit;
        const fragment = '#version 300 es\n#extension GL_EXT_YUV_target : require\nprecision highp float;in highp vec2 uv;uniform highp sampler2D irImage;uniform highp sampler2D irMapping;uniform vec2 irRange;layout(yuv) out vec4 color;void main(){vec2 p=(uv-0.5)/' + Number(OPTIONS.zoom).toFixed(6) + '+0.5;' + coordinates + 'float y=clamp((texture(irImage,p).r-irRange.x)*irRange.y,0.0,1.0);if(any(lessThan(p,vec2(0.0)))||any(greaterThan(p,vec2(1.0))))y=0.0;color=vec4(y,0.5,0.5,1.0);}';
        let fs = 0;
        try {
            fs = compileShader(0x8b30, fragment);
            program = G.program();
            G.attachShader(program, vs); G.attachShader(program, fs); G.link(program);
            G.programiv(program, 0x8b82, scratch);
            if (scratch.readS32() !== 1) {
                const log = Memory.alloc(8192);
                G.programlog(program, 8192, ptr(0), log);
                throw Error(log.readUtf8String());
            }
        } finally {
            G.delShader(vs); if (fs) G.delShader(fs);
        }
        G.genFbo(1, scratch); framebuffer = scratch.readU32();
        display = getDisplay();
        descriptor.writeByteArray(new Uint8Array(40));
        descriptor.writeU32(OPTIONS.outputSize[0]); descriptor.add(4).writeU32(OPTIONS.outputSize[1]);
        descriptor.add(8).writeU32(1); descriptor.add(12).writeU32(0x23);
        descriptor.add(16).writeU64(0x300); // GPU sampled image + framebuffer
        attrs.writeU32(0x30d2); attrs.add(4).writeU32(1); attrs.add(8).writeU32(0x3038);
        for (const source of sources) {
            if (OPTIONS.calibration) {
                G.genTex(1, scratch); source.map = scratch.readU32();
                G.bindTex(0x0de1, source.map); textureParameters(0x0de1);
                G.texImage(0x0de1, 0, 0x8230, 640, 512, 0, 0x8227, 0x1406, source.mapping);
            }
            G.genTex(1, scratch); source.upload = scratch.readU32();
            G.bindTex(0x0de1, source.upload); textureParameters(0x0de1);
            G.texImage(0x0de1, 0, 0x8229, 640, 512, 0, 0x1903, 0x1401, ptr(0));
            const result = allocate(descriptor, scratch);
            if (result !== 0) throw Error('YUV AHardwareBuffer allocation failed: ' + result);
            source.ahb = scratch.readPointer();
            source.image = createImage(display, ptr(0), 0x3140, nativeBuffer(source.ahb), attrs);
            if (source.image.isNull()) throw Error('YUV EGLImage creation failed');
            G.genTex(1, scratch); source.yuv = scratch.readU32();
            G.bindTex(0x8d65, source.yuv); textureParameters(0x8d65);
            G.imageTarget(0x8d65, source.image);
        }
        const imageLocation = G.location(program, Memory.allocUtf8String('irImage'));
        const rangeLocation = G.location(program, Memory.allocUtf8String('irRange'));
        if (OPTIONS.calibration) api.maps(sources[0].map, sources[1].map,
            G.location(program, Memory.allocUtf8String('irMapping')));
        api.resources(program, framebuffer, sources[0].upload, sources[1].upload,
            sources[0].yuv, sources[1].yuv, imageLocation, rangeLocation,
            OPTIONS.outputSize[0], OPTIONS.outputSize[1]);
        send({kind: 'ready', strategy: 'owned-yuv-native', program, framebuffer});
    } catch (error) {
        api.fail();
        send({kind: 'error', error: String(error)});
    }
}

function cleanup() {
    if (!poseTracker.isNull()) { destroyPoseTracker(9, poseTracker); poseTracker = ptr(0); }
    for (const source of sources) {
        for (const texture of [source.upload, source.yuv, source.map]) {
            if (texture) { scratch.writeU32(texture); G.delTex(1, scratch); }
        }
        source.upload = 0; source.yuv = 0; source.map = 0;
        if (!source.image.isNull()) { destroyImage(display, source.image); source.image = ptr(0); }
        if (!source.ahb.isNull()) { release(source.ahb); source.ahb = ptr(0); }
    }
    if (framebuffer) { scratch.writeU32(framebuffer); G.delFbo(1, scratch); framebuffer = 0; }
    if (program) { G.delProgram(program); program = 0; }
    send({kind: 'cleaned'});
}

const initializeCallback = new NativeCallback(initialize, 'void', []);
const cleanupCallback = new NativeCallback(cleanup, 'void', []);
// Exact-build historical tracking query, validated against its v9 interface.
const imports = {control: storage, pose_records: poseRecords, initialize_resources: initializeCallback, release_resources: cleanupCallback,
    historical_pose: tracking.base.add(0x66954),
    glBufferStorageEXT: getProc(Memory.allocUtf8String('glBufferStorageEXT'))};
for (const name of ['clock_gettime', 'open', 'read', 'close']) imports[name] = libc.getExportByName(name);
for (const name of ['glGetIntegerv', 'glGetBooleanv', 'glIsEnabled', 'glEnable', 'glDisable',
    'glGetUniformLocation', 'glGetUniformiv', 'glActiveTexture', 'glBindTexture',
    'glBindFramebuffer', 'glFramebufferTexture2D', 'glCheckFramebufferStatus', 'glGetError',
    'glBindBuffer', 'glPixelStorei', 'glTexSubImage2D', 'glViewport', 'glColorMask',
    'glUseProgram', 'glUniform1i', 'glUniform2f', 'glDrawArrays', 'glGetIntegeri_v', 'glGetInteger64i_v',
    'glGetBufferPointerv', 'glGetBufferParameteriv', 'glIsBuffer', 'glGenBuffers', 'glDeleteBuffers', 'glBufferData', 'glBufferSubData',
    'glBindBufferRange', 'glGetUniformIndices', 'glGetActiveUniformsiv', 'glGetActiveUniformBlockiv',
    'glMapBufferRange', 'glFenceSync', 'glClientWaitSync', 'glDeleteSync']) imports[name] = gl.getExportByName(name);
const module = new CModule(C_SOURCE, imports);
api = {
    configure: new NativeFunction(module.configure, 'void', ['pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'int', 'int']),
    resources: new NativeFunction(module.set_resources, 'void', ['uint', 'uint', 'uint', 'uint', 'uint', 'uint', 'int', 'int', 'uint', 'uint']),
    heartbeat: new NativeFunction(module.heartbeat, 'void', []),
    delay: new NativeFunction(module.set_delay, 'void', ['uint']),
    pose: new NativeFunction(module.configure_pose, 'void', ['pointer', 'pointer', 'uint', 'pointer', 'pointer']),
    poseAge: new NativeFunction(module.set_pose_age, 'void', ['uint']),
    poseMethod: new NativeFunction(module.set_pose_method, 'void', ['uint']),
    maps: new NativeFunction(module.set_maps, 'void', ['uint', 'uint', 'int']),
    stop: new NativeFunction(module.stop, 'void', []),
    fail: new NativeFunction(module.fail, 'void', []),
    stats: new NativeFunction(module.stats, 'void', ['pointer'])
};
api.configure(sources[0].path, sources[1].path, sources[0].sequence, sources[1].sequence,
    sources[0].pixels, sources[1].pixels, OPTIONS.normalize ? 1 : 0, OPTIONS.swapEyes ? 1 : 0);
api.delay(irDelayMs);
api.poseMethod(OPTIONS.poseMethod === 'direct' ? 1 : 0);
if (OPTIONS.poseCorrection) {
    const extrinsics = Memory.alloc(14 * 4);
    extrinsics.writeByteArray(new Float32Array(OPTIONS.cameraPoses.flat()).buffer);
    poseTracker = createPoseTracker(9);
    if (poseTracker.isNull()) throw Error('Historical tracking interface unavailable');
    if (!poseTracker.readPointer().readPointer().equals(tracking.base.add(0x66954))) {
        destroyPoseTracker(9, poseTracker); poseTracker = ptr(0);
        throw Error('Historical tracking interface changed');
    }
    api.pose(poseTracker, extrinsics, OPTIONS.poseAgeMs, sources[0].stamp, sources[1].stamp);
}
const statsBuffer = Memory.alloc(19 * 8);
function stats() {
    api.stats(statsBuffer);
    const values = Array.from({length: 19}, (_, i) => Number(statsBuffer.add(i * 8).readU64()));
    return {strategy: 'owned-yuv-native', sourceSize: [640, 512], outputSize: OPTIONS.outputSize, irDelayMs,
        enabled: !!values[0], ready: !!values[1],
        cleaned: !!values[2], errors: values[3], substitutedDraws: values[4],
        poseCorrection: !!OPTIONS.poseCorrection, poseMethod: OPTIONS.poseMethod, poseCorrectedDraws: values[15],
        poseSamples: values[16], poseFailures: values[17], poseAgeMs: values[18],
        refreshes: values[5], lastSubstitution: values[6], sources: sources.map((s, i) =>
            ({eye: s.eye, uploads: values[7 + i], sequence: values[9 + i],
                changedSamples: values[13 + i], valid: !!values[11 + i]}))};
}
Interceptor.attach(gl.getExportByName('glDrawElements'), {onEnter: module.on_enter, onLeave: module.on_leave});
rpc.exports.stats = stats;
rpc.exports.heartbeat = () => api.heartbeat();
rpc.exports.stop = () => api.stop();
rpc.exports.setdelay = function (value) {
    if (!Number.isInteger(value) || value < 0 || value > 200) throw Error('Invalid delay');
    api.delay(value); irDelayMs = value;
};
rpc.exports.setposeage = function (value) {
    if (!Number.isInteger(value) || value < 0 || value > 100) throw Error('Invalid pose age');
    api.poseAge(value);
};
rpc.exports.setmap = function (eye, data) {
    if (eye !== 0 && eye !== 1) throw Error('Invalid eye');
    if (data.byteLength !== 640 * 512 * 8) throw Error('Invalid calibration map size');
    const mapping = Memory.alloc(data.byteLength);
    mapping.writeByteArray(data);
    sources[eye].mapping = mapping;
};
send({kind: 'armed', strategy: 'owned-yuv-native', watchdogSeconds: 4});
