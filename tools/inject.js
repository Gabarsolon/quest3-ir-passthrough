// IR layer injector for Virtual Desktop (OpenGL ES path) - STEREO (per-eye quads)
// Hooks xrEndFrame, builds two GL swapchains + quad layers (LEFT eye = left camera,
// RIGHT eye = right camera), inserting them after the PASSTHROUGH_FB layer.

function NF(a, ret, args) { return new NativeFunction(a, ret, args); }
function xrn(n) { return Process.getModuleByName('libopenxr_loader.so').findExportByName(n); }
var GL = Process.getModuleByName('libGLESv2.so');
function gln(n) { return GL.findExportByName(n); }
var LIBC = Process.getModuleByName('libc.so');
function lcn(n) { return LIBC.findExportByName(n); }
var c_open  = NF(lcn('open'),  'int',  ['pointer','int','int']);
var c_read  = NF(lcn('read'),  'long', ['int','pointer','ulong']);
var c_close = NF(lcn('close'), 'int',  ['int']);

var xrCreateSwapchain          = NF(xrn('xrCreateSwapchain'), 'int', ['pointer','pointer','pointer']);
var xrEnumerateSwapchainFormats= NF(xrn('xrEnumerateSwapchainFormats'), 'int', ['pointer','uint','pointer','pointer']);
var xrEnumerateSwapchainImages = NF(xrn('xrEnumerateSwapchainImages'), 'int', ['pointer','uint','pointer','pointer']);
var xrAcquireSwapchainImage    = NF(xrn('xrAcquireSwapchainImage'), 'int', ['pointer','pointer','pointer']);
var xrWaitSwapchainImage       = NF(xrn('xrWaitSwapchainImage'), 'int', ['pointer','pointer']);
var xrReleaseSwapchainImage    = NF(xrn('xrReleaseSwapchainImage'), 'int', ['pointer','pointer']);
var xrCreateReferenceSpace     = NF(xrn('xrCreateReferenceSpace'), 'int', ['pointer','pointer','pointer']);

var glBindTexture   = NF(gln('glBindTexture'), 'void', ['uint','uint']);
var glTexSubImage2D = NF(gln('glTexSubImage2D'), 'void', ['uint','int','int','int','int','int','uint','uint','pointer']);
var glPixelStorei   = NF(gln('glPixelStorei'), 'void', ['uint','int']);

var IR_W = 640, IR_H = 512;        // mono source frame (sensor mounted rotated)
var W = IR_H, H = IR_W;            // rotated texture: 512 wide x 640 tall
var session = null, ready = false, setupTried = false, space = null, layerArr = null;
var frameNo = 0, chosFmt = 0;
var EYE_LEFT = 1, EYE_RIGHT = 2;

function makeRes(path, eye) {
    return {
        path: path, eye: eye, swapchain: null, texIds: [], imgCount: 0, quad: null,
        pixbuf: Memory.alloc(W * H * 4),
        cpath: Memory.allocUtf8String(path),
        cseq: Memory.allocUtf8String(path + '.seq'),
        seqBuf: Memory.alloc(8),
        seq: -1
    };
}
var RES = [ makeRes('/data/local/tmp/ir.raw.rgba', EYE_LEFT),
            makeRes('/data/local/tmp/ir_r.raw.rgba', EYE_RIGHT) ];

function z(p, off, n) { for (var i = 0; i < n; i++) p.add(off + i).writeU8(0); }

function readFileInto(pathPtr, dst, len) {
    var fd = c_open(pathPtr, 0, 0);
    if (fd < 0) return false;
    var off = 0;
    while (off < len) {
        var r = c_read(fd, dst.add(off), len - off);
        if (r <= 0) break;
        off += r;
    }
    c_close(fd);
    return off === len;
}

function loadIR(res) {
    var s = -1;
    var sfd = c_open(res.cseq, 0, 0);
    if (sfd >= 0) {
        if (c_read(sfd, res.seqBuf, 8) === 8) s = res.seqBuf.readU32();
        c_close(sfd);
    }
    if (s >= 0) {
        if (s === res.seq) return false;   // no new camera frame -> keep last image
        res.seq = s;
    }
    if (!readFileInto(res.cpath, res.pixbuf, W * H * 4)) {
        if (frameNo % 120 === 0) send('ir read err ' + res.path);
        return false;
    }
    return true;
}

// distinct test patterns per eye (left magenta/cyan, right yellow/blue) until files load
(function () {
    for (var k = 0; k < RES.length; k++) {
        var ab = new ArrayBuffer(W * H * 4), u8 = new Uint8Array(ab);
        for (var y = 0; y < H; y++) for (var x = 0; x < W; x++) {
            var o = (y * W + x) * 4;
            if (k === 0) { if (y < H / 2) { u8[o]=255; u8[o+2]=255; } else { u8[o+1]=255; u8[o+2]=255; } }
            else         { if (y < H / 2) { u8[o]=255; u8[o+1]=255; } else { u8[o+2]=255; } }
            u8[o + 3] = 255;
        }
        RES[k].pixbuf.writeByteArray(ab);
    }
})();

function createEye(res, ses) {
    var ci = Memory.alloc(64); z(ci, 0, 64);
    ci.writeU32(9);                 // XR_TYPE_SWAPCHAIN_CREATE_INFO
    ci.add(24).writeU64(0x21);      // usage: SAMPLED|COLOR_ATTACHMENT
    ci.add(32).writeS64(chosFmt);   // format
    ci.add(40).writeU32(1);         // sampleCount
    ci.add(44).writeU32(W);
    ci.add(48).writeU32(H);
    ci.add(52).writeU32(1);         // faceCount
    ci.add(56).writeU32(1);         // arraySize
    ci.add(60).writeU32(1);         // mipCount
    var scp = Memory.alloc(8);
    var r = xrCreateSwapchain(ses, ci, scp);
    if (r !== 0) { send('xrCreateSwapchain eye=' + res.eye + ' r=' + r); return false; }
    res.swapchain = scp.readPointer();

    var ic = Memory.alloc(4);
    xrEnumerateSwapchainImages(res.swapchain, 0, ic, ptr(0));
    res.imgCount = ic.readU32();
    var imgs = Memory.alloc(res.imgCount * 24);
    for (var i = 0; i < res.imgCount; i++) { z(imgs, i * 24, 24); imgs.add(i * 24).writeU32(1000024002); }
    xrEnumerateSwapchainImages(res.swapchain, res.imgCount, ic, imgs);
    res.texIds = [];
    for (var i = 0; i < res.imgCount; i++) res.texIds.push(imgs.add(i * 24).add(16).readU32());
    send('eye=' + res.eye + ' images=' + res.imgCount + ' texIds=' + res.texIds.join(','));

    var q = Memory.alloc(112); z(q, 0, 112);
    q.writeU32(36);                 // XR_TYPE_COMPOSITION_LAYER_QUAD
    q.add(16).writeU64(0);          // layerFlags
    q.add(24).writePointer(space);
    q.add(32).writeU32(res.eye);    // eyeVisibility: LEFT/RIGHT
    q.add(40).writePointer(res.swapchain);  // subImage.swapchain
    q.add(48).writeU32(0); q.add(52).writeU32(0);   // imageRect offset
    q.add(56).writeU32(W); q.add(60).writeU32(H);   // imageRect extent
    q.add(64).writeU32(0);          // imageArrayIndex
    q.add(72).writeFloat(0); q.add(76).writeFloat(0); q.add(80).writeFloat(0); q.add(84).writeFloat(1);
    q.add(88).writeFloat(0); q.add(92).writeFloat(0); q.add(96).writeFloat(-8.0);
    q.add(100).writeFloat(17.0); q.add(104).writeFloat(21.25); // size (aspect 512:640)
    res.quad = q;
    return true;
}

function setup(ses) {
    session = ses;
    var cnt = Memory.alloc(4);
    xrEnumerateSwapchainFormats(ses, 0, cnt, ptr(0));
    var n = cnt.readU32();
    var fmts = Memory.alloc(Math.max(1, n) * 8);
    xrEnumerateSwapchainFormats(ses, n, cnt, fmts);
    var list = [];
    for (var i = 0; i < n; i++) list.push(fmts.add(i * 8).readU32());
    var prefs = [0x8058, 0x8C43, 0x8D62, 0x93A1];
    chosFmt = -1;
    for (var p = 0; p < prefs.length && chosFmt < 0; p++) if (list.indexOf(prefs[p]) >= 0) chosFmt = prefs[p];
    if (chosFmt < 0) chosFmt = list[0];
    send('formats=' + list.map(function (x) { return x + '(0x' + (x >>> 0).toString(16) + ')'; }).join(',') + ' -> chosen=0x' + (chosFmt >>> 0).toString(16));

    var rci = Memory.alloc(56); z(rci, 0, 56);
    rci.writeU32(37);               // XR_TYPE_REFERENCE_SPACE_CREATE_INFO
    rci.add(16).writeU32(1);        // XR_REFERENCE_SPACE_TYPE_VIEW
    rci.add(20).writeFloat(0); rci.add(24).writeFloat(0); rci.add(28).writeFloat(0); rci.add(32).writeFloat(1);
    rci.add(36).writeFloat(0); rci.add(40).writeFloat(0); rci.add(44).writeFloat(0);
    var spp = Memory.alloc(8);
    var r3 = xrCreateReferenceSpace(ses, rci, spp);
    space = spp.readPointer();
    send('xrCreateReferenceSpace r=' + r3 + ' space=' + space);

    for (var k = 0; k < RES.length; k++) if (!createEye(RES[k], ses)) return;

    ready = true;
    layerArr = Memory.alloc(64 * 8);
    send('SETUP COMPLETE ready (stereo)');
}

function uploadOne(res) {
    if (!res.swapchain) return;
    if (!loadIR(res)) return;   // no new frame -> reuse last released image
    var wi = Memory.alloc(32); z(wi, 0, 32);
    wi.writeU32(56); wi.add(16).writeU64(5000000000);
    var ai = Memory.alloc(16); z(ai, 0, 16); ai.writeU32(55);
    var idxp = Memory.alloc(4);
    var ra = xrAcquireSwapchainImage(res.swapchain, ai, idxp);
    if (ra !== 0) { if (frameNo % 90 === 0) send('acquire fail eye=' + res.eye + ' r=' + ra); return; }
    var rw = xrWaitSwapchainImage(res.swapchain, wi);
    if (rw !== 0) { if (frameNo % 90 === 0) send('wait fail eye=' + res.eye + ' r=' + rw); return; }
    var idx = idxp.readU32();
    var tex = res.texIds[idx];
    glBindTexture(0x0DE1, tex);
    glPixelStorei(0x0CF5, 1);
    glTexSubImage2D(0x0DE1, 0, 0, 0, W, H, 0x1908, 0x1401, res.pixbuf);
    var ri = Memory.alloc(16); z(ri, 0, 16); ri.writeU32(57);
    xrReleaseSwapchainImage(res.swapchain, ri);
}

var statCount = 0, appended = 0;
var ef = xrn('xrEndFrame');
Interceptor.attach(ef, {
    onEnter: function (args) {
        this.info = args[1];
        try {
            if (!ready && !setupTried) { setupTried = true; try { setup(args[0]); } catch (e) { send('setup ERR ' + e); } }
            if (!ready) return;
            uploadOne(RES[0]);
            uploadOne(RES[1]);
            statCount++; frameNo++;
            var info = this.info;
            if (info.isNull()) return;
            var n = info.add(28).readU32();
            var lp = info.add(32).readPointer();
            if (n > 0 && lp.isNull()) return;
            if (n > 60) n = 60;
            var na = layerArr, j = 0, types = [], inserted = false;
            for (var i = 0; i < n; i++) {
                var L = lp.add(i * 8).readPointer();
                if (L.isNull()) continue;
                var t = L.readU32(); types.push(t);
                na.add(j * 8).writePointer(L); j++;
                if (t === 1000118003 && !inserted) {
                    for (var a = 0; a < RES.length && j < 62; a++) { na.add(j * 8).writePointer(RES[a].quad); j++; }
                    inserted = true;
                }
            }
            if (!inserted) { for (var b = 0; b < RES.length && j < 62; b++) { na.add(j * 8).writePointer(RES[b].quad); j++; } }
            info.add(28).writeU32(j);
            info.add(32).writePointer(na);
            if (statCount % 300 === 0) send('frames=' + statCount + ' n=' + n + '->' + j + ' types=' + types.join(','));
        } catch (e) { send('ERR ' + e); }
    },
    onLeave: function (retval) {
        var r = (typeof retval === 'object' && retval !== null) ? retval.toInt32() : retval;
        if (r !== 0 || statCount % 90 === 0) send('xrEndFrame ret=' + r);
    }
});
send('injector armed');
rpc.exports.stats = function () { return { frames: statCount, appended: appended, ready: ready, imgCount: RES[0].imgCount }; };
