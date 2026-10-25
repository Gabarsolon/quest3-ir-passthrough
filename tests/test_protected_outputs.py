"""Exercise the actual JS output allocator against EGL/GL protection rules."""
from pathlib import Path
import shutil
import subprocess
import unittest

from test_system_contexts import function

ROOT = Path(__file__).resolve().parents[1]


class ProtectedOutputTests(unittest.TestCase):
    def test_context_protection_and_all_render_targets(self):
        source = (ROOT / 'tools/system_passthrough.js').read_text()
        functions = '\n'.join(function(source, name) for name in (
            'function textureParameters(', 'function configureOutput(',
            'function allocateAntialias('))
        script = r'''
const assert = require('node:assert/strict');
class Memory {
    constructor(size, buffer, offset=0) {this.buffer=buffer||Buffer.alloc(size); this.offset=offset;}
    add(offset) {return new Memory(0,this.buffer,this.offset+offset);}
    writeByteArray(bytes) {Buffer.from(bytes).copy(this.buffer,this.offset);}
    writeU32(value) {this.buffer.writeUInt32LE(value,this.offset);}
    writeU64(value) {this.buffer.writeBigUInt64LE(BigInt(value),this.offset);}
    readU32() {return this.buffer.readUInt32LE(this.offset);}
    readS32() {return this.buffer.readInt32LE(this.offset);}
}
const scratch=new Memory(32),descriptor=new Memory(40),attrs=new Memory(20);
const context={}, display={}, OPTIONS={outputSize:[1280,960],antialiasing:'edge'};
let protectedContent=false,eglProtected=0,flags=0,queryOk=true,eglExtension=true;
let operations=[];
const ptr=value=>value;
const queryString=(actual,key)=>{
    assert.equal(actual,display); assert.equal(key,0x3055);
    return {isNull:()=>false,readUtf8String:()=>eglExtension?'EGL_EXT_protected_content':''};
};
const queryContext=(actual,owner,key,out)=>{
    assert.equal(actual,display);assert.equal(owner,context);assert.equal(key,0x32c0);
    out.writeU32(eglProtected); return queryOk;
};
const G={
    integer:(key,out)=>{assert.equal(key,0x821e);out.writeU32(flags);},
    genTex:(count,out)=>{assert.equal(count,1);out.writeU32(77);},
    bindTex:(target,id)=>{assert.equal(target,0x0de1);assert.equal(id,77);},
    param:(target,key,value)=>{assert.equal(target,0x0de1);operations.push([key,value]);},
    texStorage:(target,levels,format,width,height)=>{
        assert.deepEqual([target,levels,format,width,height],[0x0de1,1,0x8229,1280,960]);
        assert.deepEqual(operations.at(-1),[0x8bfa,1]);operations.push('protected-storage');
    },
    texImage:()=>{assert.equal(protectedContent,false);operations.push('ordinary-storage');}
};
FUNCTIONS
function checkLayout(usage,expectedAttrs) {
    assert.deepEqual([...new Uint32Array(descriptor.buffer.buffer,
        descriptor.buffer.byteOffset,4)],[1280,960,1,0x23]);
    assert.equal(Number(descriptor.buffer.readBigUInt64LE(16)),usage);
    const actual=[]; for(let i=0;i<5;i++) {const v=attrs.add(i*4).readU32();actual.push(v);if(v===0x3038)break;}
    assert.deepEqual(actual,expectedAttrs);
}
const ext=['GL_EXT_protected_textures'];
configureOutput(display,ext);
assert.equal(protectedContent,false);checkLayout(0x300,[0x30d2,1,0x3038]);
allocateAntialias({});assert.equal(operations.at(-1),'ordinary-storage');
eglProtected=1;flags=16;operations=[];
configureOutput(display,ext);
assert.equal(protectedContent,true);checkLayout(0x4300,[0x30d2,1,0x32c0,1,0x3038]);
allocateAntialias({});assert.equal(operations.at(-1),'protected-storage');
assert.throws(()=>configureOutput(display,[]),/Protected antialiasing requires/);
queryOk=false;assert.throws(()=>configureOutput(display,ext),/Cannot query EGL context/);
queryOk=true;flags=0;assert.throws(()=>configureOutput(display,ext),/mismatch/);
eglExtension=false;flags=16;assert.throws(()=>configureOutput(display,ext),/mismatch/);
flags=0;configureOutput(display,[]);checkLayout(0x300,[0x30d2,1,0x3038]);
'''.replace('FUNCTIONS', functions)
        node = shutil.which('node')
        self.assertIsNotNone(node, 'Node.js is needed for the JS allocator test')
        subprocess.run([node, '-e', script], check=True, capture_output=True, text=True)


if __name__ == '__main__':
    unittest.main()
