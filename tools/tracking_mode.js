'use strict';
// Offsets/ABI validated on build 51154110174900520 only; host checks the build.
const base = Process.mainModule.base;
const apply = new NativeFunction(base.add(0x3a780), 'uint64', ['pointer', 'uchar']);
const updateFrequency = new NativeFunction(base.add(0x4b640), 'void', ['pointer']);
let translator = null, modes = [], original = -1, active = false, timer = null;
let policy = null, restoring = false, overrides = 0;
let utility = null, originalFrequency = null, requestedFrequency = null, frequencyPolicy = null;
const utilityProbe = Interceptor.attach(base.add(0x4bae0), {onEnter(args) { utility = args[0]; }});
function string(p) {
    const tag = p.readU8();
    return (tag & 1) ? p.add(16).readPointer().readUtf8String() : p.add(1).readUtf8String(tag >> 1);
}
const probe = Interceptor.attach(base.add(0x3a8e0), {onEnter(args) {
    const self = args[0];
    if (string(self.add(8)) !== 'worldTracking') return;
    const begin = self.add(0x68).readPointer(), end = self.add(0x70).readPointer();
    const count = Number(end.sub(begin)) / 0xe8;
    if (count < 1 || count > 40 || count % 1) throw Error('Invalid tracking mode vector');
    modes = Array.from({length: count}, (_, i) => string(begin.add(i * 0xe8)));
    translator = self;
}});
function select(index) {
    const result = apply(translator, index);
    if (result.and(uint64('4294967295')).compare(uint64(0)) !== 0)
        throw Error('Tracking mode request failed: ' + result);
    return result.toString();
}
function restore() {
    if (timer !== null) { clearTimeout(timer); timer = null; }
    if (!active) return false;
    // Remain pinned if restoration fails, so a later explicit retry is possible.
    restoring = true;
    try {
        select(original);
        if (originalFrequency !== null) {
            utility.add(0x28).writeS32(originalFrequency);
            updateFrequency(utility);
        }
    } finally { restoring = false; }
    active = false;
    if (policy !== null) { policy.detach(); policy = null; }
    if (frequencyPolicy !== null) { frequencyPolicy.detach(); frequencyPolicy = null; }
    Script.unpin();
    send({kind: 'mode-restored', mode: modes[original]});
    return true;
}
function arm(ms) {
    if (timer !== null) clearTimeout(timer);
    timer = setTimeout(() => { try { restore(); } catch (e) { send({kind: 'error', error: String(e)}); } }, ms);
}
rpc.exports.activate = function (name, frequency) {
    if (!translator) throw Error('Tracking translator was not discovered');
    probe.detach();
    utilityProbe.detach();
    if (frequency !== null && frequency !== 50 && frequency !== 60)
        throw Error('Unsupported utility frequency');
    original = modes.indexOf(name);
    const target = modes.indexOf('lowLightHandsDouble');
    if (original < 0 || target < 0) throw Error('Expected tracking modes are unavailable');
    // Home can request dynamic-object tracking after its UI initializes. FMM
    // and dynamic-object tracking are mutually exclusive; prioritize the
    // explicitly requested IR cadence for these two hands-only configurations.
    const fastModes = ['lowLightHands', 'lowLightHandsObject']
        .map(n => modes.indexOf(n)).filter(i => i >= 0);
    if (!fastModes.includes(original))
        return {active: false, original: name, reason: 'Preserving the current input mode'};
    Script.pin(); active = true; arm(15000);
    // The service periodically reselects the two-slot mode. Substitute only
    // that exact mode at the same validated interface, leaving controller,
    // depth, object-tracking and disabled modes under the service's control.
    policy = Interceptor.attach(base.add(0x382a0), {onEnter(args) {
        if (active && !restoring && args[0].equals(translator)
                && fastModes.includes(args[1].toUInt32() & 255)) {
            args[1] = ptr(target);
            overrides++;
        }
    }});
    try {
        if (frequency !== null) {
            if (utility === null) throw Error('Utility frequency manager was not discovered');
            const cached = utility.add(0x28).readS32();
            const applied = utility.add(0x38).readS32();
            if (![-1, 50, 60].includes(cached) || ![50, 60].includes(applied))
                throw Error('Unexpected utility frequency state');
            originalFrequency = cached;
            requestedFrequency = frequency;
            // Only the manager's cached value changes; no preference is saved.
            frequencyPolicy = Interceptor.attach(base.add(0x4b640), {onEnter(args) {
                if (active && !restoring && args[0].equals(utility))
                    utility.add(0x28).writeS32(requestedFrequency);
            }});
            utility.add(0x28).writeS32(frequency);
            updateFrequency(utility);
            if (utility.add(0x38).readS32() !== frequency) throw Error('Utility frequency was not applied');
        }
        select(target);
    } catch (e) { restore(); throw e; }
    return {active: true, original: name, selected: modes[target], utilityFrequency: requestedFrequency,
        dynamicObjectTrackingPaused: name === 'lowLightHandsObject'};
};
rpc.exports.heartbeat = () => { if (active) arm(4000); };
rpc.exports.restore = restore;
rpc.exports.status = () => ({active, original: modes[original], overrides,
    utilityFrequency: requestedFrequency === null ? null : utility.add(0x38).readS32()});
