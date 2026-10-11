// IR layer injector for Virtual Desktop (OpenGL ES path)
// Hooks xrEndFrame, builds own GL swapchain + quad layer, replaces the
// XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB layer with our own quad.

function NF(a, ret, args) { return new NativeFunction(a, ret, args); }
function xrn(n) { return Process.getModuleByName('libopenxr_loader.so').findExportByName(n); }
var GL = Process.getModuleByName('libGLESv2.so');
function gln(n) { return GL.findExportByName(n); }

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

var W = 640, H = 512;
var session = null, ready = false, setupTried = false, swapchain = null, space = null, quad = null;
var texIds = [], imgCount = 0, frameNo = 0, layerArr = null;
var TARGET_TYPE = 1000017000; // CYLINDER_KHR (change to 1000118003 to test PASSTHROUGH_FB)
var pixbuf = Memory.alloc(W * H * 4);
var irAB = new ArrayBuffer(W * H * 4);
var irU8 = new Uint8Array(irAB);
var irU32 = new Uint32Array(irAB);
var irOK = false;

function loadIR() {
    try {
        var f = new File('/data/local/tmp/ir.raw', 'rb');
        var ab = f.readBytes(W * H);
        f.close();
        var src = new Uint8Array(ab);
        var d = irU32, n = W * H;
        for (var i = 0; i < n; i++) { var v = src[i]; d[i] = 0xff000000 | (v << 16) | (v << 8) | v; }
        pixbuf.writeByteArray(irAB);
        irOK = true;
    } catch (e) {
        if (frameNo % 60 === 0) send('ir read err ' + e);
    }
}

// build a test pattern once
(function () {
    var ab = new ArrayBuffer(W * H * 4);
    var u8 = new Uint8Array(ab);
    for (var y = 0; y < H; y++) for (var x = 0; x < W; x++) {
        var o = (y * W + x) * 4;
        if (y < H / 2) { u8[o] = 255; u8[o + 1] = 0;   u8[o + 2] = 255; } // top: magenta
        else           { u8[o] = 0;   u8[o + 1] = 255; u8[o + 2] = 255; } // bottom: cyan
        u8[o + 3] = 255;
    }
    pixbuf.writeByteArray(ab);
})();

function z(p, off, n) { for (var i = 0; i < n; i++) p.add(off + i).writeU8(0); }

function setup(ses) {
    session = ses;
    // choose a format
    var cnt = Memory.alloc(4);
    xrEnumerateSwapchainFormats(ses, 0, cnt, ptr(0));
    var n = cnt.readU32();
    var fmts = Memory.alloc(Math.max(1, n) * 8);
    xrEnumerateSwapchainFormats(ses, n, cnt, fmts);
    var chos = -1, list = [];
    for (var i = 0; i < n; i++) { var f = fmts.add(i * 8).readU32(); list.push(f); }
    var prefs = [0x8058, 0x8C43, 0x8D62, 0x93A1];
    for (var p = 0; p < prefs.length && chos < 0; p++) if (list.indexOf(prefs[p]) >= 0) chos = prefs[p];
    if (chos < 0) chos = list[0];
    send('formats=' + list.map(function (x) { return x + '(0x' + (x >>> 0).toString(16) + ')'; }).join(',') + ' -> chosen=0x' + (chos >>> 0).toString(16));

    var ci = Memory.alloc(64); z(ci, 0, 64);
    ci.writeU32(9);                 // XR_TYPE_SWAPCHAIN_CREATE_INFO
    ci.add(24).writeU64(0x21);      // usage: SAMPLED|COLOR_ATTACHMENT
    ci.add(32).writeS64(chos);      // format
    ci.add(40).writeU32(1);         // sampleCount
    ci.add(44).writeU32(W);
    ci.add(48).writeU32(H);
    ci.add(52).writeU32(1);         // faceCount
    ci.add(56).writeU32(1);         // arraySize
    ci.add(60).writeU32(1);         // mipCount
    var scp = Memory.alloc(8);
    var r = xrCreateSwapchain(ses, ci, scp);
    send('xrCreateSwapchain r=' + r);
    if (r !== 0) return;
    swapchain = scp.readPointer();

    var ic = Memory.alloc(4);
    xrEnumerateSwapchainImages(swapchain, 0, ic, ptr(0));
    imgCount = ic.readU32();
    var imgs = Memory.alloc(imgCount * 24);
    for (var i = 0; i < imgCount; i++) { z(imgs, i * 24, 24); imgs.add(i * 24).writeU32(1000024002); }
    var r2 = xrEnumerateSwapchainImages(swapchain, imgCount, ic, imgs);
    for (var i = 0; i < imgCount; i++) texIds.push(imgs.add(i * 24).add(16).readU32());
    send('images=' + imgCount + ' texIds=' + texIds.join(',') + ' r2=' + r2);

    // reference space (VIEW)
    var rci = Memory.alloc(56); z(rci, 0, 56);
    rci.writeU32(37);               // XR_TYPE_REFERENCE_SPACE_CREATE_INFO
    rci.add(16).writeU32(1);        // XR_REFERENCE_SPACE_TYPE_VIEW
    rci.add(20).writeFloat(0); rci.add(24).writeFloat(0); rci.add(28).writeFloat(0); rci.add(32).writeFloat(1); // quat identity
    rci.add(36).writeFloat(0); rci.add(40).writeFloat(0); rci.add(44).writeFloat(0); // position 0
    var spp = Memory.alloc(8);
    var r3 = xrCreateReferenceSpace(ses, rci, spp);
    space = spp.readPointer();
    send('xrCreateReferenceSpace r=' + r3 + ' space=' + space);

    // quad layer
    quad = Memory.alloc(112); z(quad, 0, 112);
    quad.writeU32(36);              // XR_TYPE_COMPOSITION_LAYER_QUAD
    quad.add(16).writeU64(0);       // layerFlags
    quad.add(24).writePointer(space);
    quad.add(32).writeU32(0);       // eyeVisibility BOTH
    quad.add(40).writePointer(swapchain);  // subImage.swapchain
    quad.add(48).writeU32(0); quad.add(52).writeU32(0);       // imageRect offset
    quad.add(56).writeU32(W); quad.add(60).writeU32(H);       // imageRect extent
    quad.add(64).writeU32(0);       // imageArrayIndex
    // pose: identity orientation, position (0,0,-2)
    quad.add(72).writeFloat(0); quad.add(76).writeFloat(0); quad.add(80).writeFloat(0); quad.add(84).writeFloat(1);
    quad.add(88).writeFloat(0); quad.add(92).writeFloat(0); quad.add(96).writeFloat(-8.0);
    quad.add(100).writeFloat(40.0); quad.add(104).writeFloat(40.0); // size
    ready = true;
    layerArr = Memory.alloc(16 * 8);
    send('SETUP COMPLETE ready');
}

function upload() {
    var wi = Memory.alloc(32); z(wi, 0, 32);
    wi.writeU32(56); wi.add(16).writeU64(5000000000);
    var ai = Memory.alloc(16); z(ai, 0, 16); ai.writeU32(55);
    var idxp = Memory.alloc(4);
    var ra = xrAcquireSwapchainImage(swapchain, ai, idxp);
    if (ra !== 0) { if (frameNo % 90 === 0) send('acquire fail r=' + ra); return; }
    var rw = xrWaitSwapchainImage(swapchain, wi);
    if (rw !== 0) { if (frameNo % 90 === 0) send('wait fail r=' + rw); return; }
    var idx = idxp.readU32();
    var tex = texIds[idx];
    loadIR();
    glBindTexture(0x0DE1, tex);
    glPixelStorei(0x0CF5, 1);
    glTexSubImage2D(0x0DE1, 0, 0, 0, W, H, 0x1908, 0x1401, pixbuf);
    var ri = Memory.alloc(16); z(ri, 0, 16); ri.writeU32(57);
    xrReleaseSwapchainImage(swapchain, ri);
    frameNo++;
}

var statCount = 0, swapped = 0, appended = 0;
var ef = xrn('xrEndFrame');
Interceptor.attach(ef, {
    onEnter: function (args) {
        this.info = args[1];
        try {
            if (!ready && !setupTried) { setupTried = true; try { setup(args[0]); } catch (e) { send('setup ERR ' + e); } }
            if (!ready) return;
            upload();
            statCount++;
            var info = this.info;
            var n = info.add(28).readU32();
            var lp = info.add(32).readPointer();
            var na = layerArr, j = 0, types = [], inserted = false;
            for (var i = 0; i < n; i++) {
                var L = lp.add(i * 8).readPointer();
                var t = L.readU32(); types.push(t);
                na.add(j * 8).writePointer(L); j++;
                if (t === 1000118003 && !inserted) { na.add(j * 8).writePointer(quad); j++; inserted = true; }
            }
            if (!inserted) { na.add(j * 8).writePointer(quad); j++; }
            info.add(28).writeU32(j);
            info.add(32).writePointer(na);
            if (statCount % 90 === 0) send('frames=' + statCount + ' n=' + n + '->' + j + ' types=' + types.join(','));
        } catch (e) { send('ERR ' + e); }
    },
    onLeave: function (retval) {
        var r = (typeof retval === 'object' && retval !== null) ? retval.toInt32() : retval;
        if (r !== 0 || statCount % 90 === 0) send('xrEndFrame ret=' + r);
    }
});
send('injector armed');
rpc.exports.stats = function () { return { frames: statCount, appended: appended, ready: ready, imgCount: imgCount }; };
