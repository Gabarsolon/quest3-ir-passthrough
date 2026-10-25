"""Compile the real native context guards with isolated fake GL namespaces."""
import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    start = source.rfind('\n', 0, source.index(name)) + 1
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


def run_native(source):
    with tempfile.TemporaryDirectory(prefix='quest-ir-context-test-') as tmp:
        folder = Path(tmp)
        (folder / 'gum').mkdir()
        (folder / 'gum/guminterceptor.h').write_text(
            'typedef struct {unsigned char data[256];} GumInvocationContext;\n'
            '#define GUM_IC_GET_INVOCATION_DATA(ic,type) ((type *)(ic)->data)\n')
        (folder / 'test.c').write_text(source)
        library = folder / ('test.dll' if os.name == 'nt' else 'test.so')
        compiler = shutil.which('clang') or r'C:\Program Files\LLVM\bin\clang.exe'
        command = [compiler, '-shared', '-nostdlib', '-fno-builtin', '-O2', '-I', tmp,
                   str(folder / 'test.c'), '-o', str(library)]
        if os.name == 'nt':
            command[1:1] = ['-target', 'x86_64-pc-windows-msvc', '-fuse-ld=lld']
            command.append('-Wl,/noentry')
        else:
            command.append('-fPIC')
        subprocess.run(command, check=True)
        loaded = ctypes.CDLL(str(library))
        try:
            return loaded.run_tests()
        finally:
            if os.name == 'nt':
                ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(loaded._handle))


EXPORT = '''
#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
'''


class ContextTests(unittest.TestCase):
    def test_first_remap_isolates_and_restores_native_clip_planes(self):
        source = (ROOT / 'tools/system_passthrough.c').read_text()
        code = source[:source.index('static Q now_ms')]
        code += r'''
Control control; PoseRecord pose_records[1]; void *render_context;
static U clip_bits,clip_queries,clip_support=8;
static int sample_mask=1,neutral,green,active=0x84c3,program=44,framebuffer=99;
static int viewport[4]={0,0,4128,2208};
static unsigned char states[9]={1,1,1,1,1,1,1,1,1};
static int cap_index(U key){
 const U caps[]={0x0be2,0x0b71,0x0b44,0x0b90,0x0c11,0x8c89,0x809e,0x80a0,0x8e51};
 int i;for(i=0;i<9;i++)if(caps[i]==key)return i;return -1;
}
void glGetIntegerv(U key,int *out){
 int i;
 if(key==0x0ba2){for(i=0;i<4;i++)out[i]=viewport[i];return;}
 *out=key==0x84e0?active:(key==0x8b8d?program:(key==0x8ca6?framebuffer:0));
}
static int integer(U key){int value;glGetIntegerv(key,&value);return value;}
void glGetBooleanv(U key,unsigned char *out){int i;for(i=0;i<4;i++)out[i]=1;}
unsigned char glIsEnabled(U key){
 if(key>=0x3000&&key<0x3008){clip_queries++;return !!(clip_bits&(1U<<(key-0x3000)));}
 int i=cap_index(key);return i>=0?states[i]:0;
}
void glEnable(U key){
 if(key>=0x3000&&key<0x3008){clip_bits|=1U<<(key-0x3000);return;}
 int i=cap_index(key);if(i>=0)states[i]=1;
}
void glDisable(U key){
 if(key>=0x3000&&key<0x3008){clip_bits&=~(1U<<(key-0x3000));return;}
 int i=cap_index(key);if(i>=0)states[i]=0;
}
void glActiveTexture(U value){active=value;}
void glBindTexture(U target,U value){}
void glBindSampler(U unit,U value){}
void glBindBuffer(U target,U value){}
void glBindFramebuffer(U target,U value){framebuffer=value;}
void glUseProgram(U value){program=value;}
void glViewport(int x,int y,int w,int h){viewport[0]=x;viewport[1]=y;viewport[2]=w;viewport[3]=h;}
void glColorMask(unsigned char r,unsigned char g,unsigned char b,unsigned char a){}
void glPixelStorei(U key,int value){}
void glUniform1i(int location,int value){}
void initialize_resources(void){
 control.clip_count=clip_support;control.ready=1;
 control.program=777;control.framebuffer=888;control.width=1280;control.height=960;
}
static void refresh_source(Source *source,int poll){
 // Model the symptom: discard leaves untouched YUV chroma, rather than a
 // neutral-chroma image. This checks the actual production pass boundaries.
 if(clip_bits||states[8]||program!=777||framebuffer!=888)green++;
 else neutral++;
}
'''
        code += function(source, 'refresh(')
        code += EXPORT + r'''
int run_tests(void){
 int i;control.enabled=1;control.ready=0;control.clip_count=0;clip_bits=7;
 refresh(-1,1);
 if(neutral!=2||green||clip_bits!=7||!clip_queries)return 30;
 if(active!=0x84c3||program!=44||framebuffer!=99||viewport[2]!=4128||viewport[3]!=2208)return 31;
 for(i=0;i<9;i++)if(!states[i])return 32;
 // Enabled planes can change between native layers; restore the current set.
 clip_bits=0xaa;neutral=green=0;refresh(-1,1);
 if(neutral!=2||green||clip_bits!=0xaa)return 33;
 // Extension-absent contexts must not query or toggle unsupported clip enums.
 control.ready=0;control.clip_count=0;clip_support=0;clip_bits=0;clip_queries=0;
 neutral=green=0;refresh(-1,1);
 if(neutral!=2||green||clip_queries)return 34;
 return 0;
}
'''
        self.assertEqual(run_native(code), 0)

    def test_foreign_namespaces_and_cleanup(self):
        source = (ROOT / 'tools/system_passthrough.c').read_text()
        code = source[:source.index('static Q now_ms')]
        code += source[source.index('typedef struct {\n  int active, texture'):source.index('static int validate_pose_layout')]
        code += r'''
Control control; PoseRecord pose_records[1]; void *render_context=(void *)1;
static void *current=(void *)2;
static int queries,binds,releases,active=0x84c7,matched=1;
static int textures[16];
void *eglGetCurrentContext(void){return current;}
static Q now_ms(void){return 10000;}
void glGetIntegerv(U key,int *out){
 queries++;
 if(key==0x8b8d)*out=55;
 else if(key==0x84e0)*out=active;
 else if(key==0x8d67)*out=textures[active-0x84c0];
 else if(key==0x0ba2){out[0]=out[1]=0;out[2]=4128;out[3]=2208;}
 else if(key==0x0c10){out[0]=out[1]=0;out[2]=2064;out[3]=1104;}
}
static int integer(U key){int value;glGetIntegerv(key,&value);return value;}
int glGetUniformLocation(U p,const char *s){queries++;return matched?3:-1;}
void glGetUniformiv(U p,int location,int *out){*out=3;}
unsigned char glIsEnabled(U key){return 1;}
void glActiveTexture(U value){active=value;}
void glBindTexture(U target,U value){binds++;textures[active-0x84c0]=value;}
void glBindBuffer(U target,U value){}
void glBindBufferRange(U t,U i,U b,long s,long z){}
void glDeleteSync(void *s){}
void *glFenceSync(U a,U b){return (void*)7;}
void glDeleteBuffers(int n,const U *p){}
void release_resources(void){releases++;}
static void restore_direct_poses(void){}
static int validate_pose_layout(U program){return 1;}
static int begin_image_frame(Source *s,const int *v,const int *r,Q n){return 1;}
static int read_native_pose(Source *s){return 1;}
static int write_ir_pose(Source *s,Invocation *c){return 1;}
static int bind_ir_pose(Source *s,Invocation *c){return 1;}
static void refresh(int eye,int poll){control.sources[0].valid=control.sources[1].valid=1;}
static void reset(void){
 unsigned long i;unsigned char *p=(unsigned char*)&control;
 for(i=0;i<sizeof(control);i++)p[i]=0;
 for(i=0;i<16;i++)textures[i]=80;
 queries=binds=releases=0;active=0x84c7;matched=1;
 control.enabled=1;control.heartbeat=10000;control.last_refresh=10000;
 control.sources[0].valid=control.sources[1].valid=1;
 control.sources[0].yuv_texture=101;control.sources[1].yuv_texture=102;
 control.pose_method=POSE_IMAGE;
}
'''
        for name in ['fail(', 'invalidate_program_cache(', 'release_owned_resources(',
                     'cleanup_current(', 'on_enter(', 'on_leave(']:
            code += function(source, name)
        code += EXPORT + r'''
int run_tests(void){
 GumInvocationContext ic;
 // Equal program/texture numbers in another context must trigger no GL work.
 reset();current=(void*)2;on_enter(&ic);on_leave(&ic);
 if(queries||binds||releases||control.cache_count||control.owner_calls)return 10;
 // The owning namespace substitutes and restores the original texture/unit.
 current=(void*)1;on_enter(&ic);
 if(textures[3]!=101||active!=0x84c7||control.owner_calls!=1)return 11;
 if(cleanup_current()||releases)return 12;
 on_leave(&ic);
 if(textures[3]!=80||active!=0x84c7||control.owner_calls||control.draws!=1)return 13;
 // Host stop and explicit cleanup both leave the foreign namespace alone.
 control.enabled=0;queries=binds=0;current=(void*)2;
 on_enter(&ic);on_leave(&ic);
 if(queries||binds||releases||cleanup_current())return 14;
 current=(void*)1;
 if(!cleanup_current()||releases!=1||!control.cleaned)return 15;
 if(!cleanup_current()||releases!=1)return 16;
 // A relink/delete invalidation rejects a recycled program name that no longer
 // has the passthrough sampler, despite an earlier positive cache entry.
 reset();current=(void*)1;on_enter(&ic);on_leave(&ic);
 matched=0;invalidate_program_cache();binds=0;
 on_enter(&ic);on_leave(&ic);
 if(binds||control.cache[0].location!=-1||control.owner_calls)return 17;
 // A context changed inside an intercepted draw must never receive restoration
 // calls belonging to the first context.
 reset();current=(void*)1;on_enter(&ic);binds=queries=0;current=(void*)2;
 on_leave(&ic);
 if(binds||queries||control.enabled||control.errors!=1||control.owner_calls)return 18;
 return 0;
}
'''
        self.assertEqual(run_native(code), 0)

    def test_registry_reuse_limits_and_disabled_observation(self):
        code = (ROOT / 'tools/system_contexts.c').read_text()
        code += r'''
unsigned char registry_lock[64];
typeof(registry) registry;
static void *current;
static int locked,discovered,queries;
void *eglGetCurrentContext(void){return current;}
int pthread_mutex_trylock(void *p){if(locked)return 1;locked=1;return 0;}
int pthread_mutex_lock(void *p){locked=1;return 0;}
int pthread_mutex_unlock(void *p){locked=0;return 0;}
void glGetIntegerv(U key,int *out){queries++;*out=55;}
int glGetUniformLocation(U p,const char *s){queries++;return 3;}
int context_discovered(void *p){if(locked)return -1;discovered++;return 1;}
'''
        code += EXPORT + r'''
int run_tests(void){
 GumInvocationContext ic;int i;
 current=(void*)1;observe(&ic);
 if(discovered||queries)return 20;
 registry.enabled=1;observe(&ic);observe(&ic);
 if(discovered!=1||queries!=2||locked)return 21;
 // A second context with exactly the same numeric program gets its own slot.
 current=(void*)2;observe(&ic);
 if(discovered!=2)return 22;
 forget((void*)1);current=(void*)1;observe(&ic);
 if(discovered!=3)return 23;
 locked=1;current=(void*)3;observe(&ic);locked=0;
 if(discovered!=3)return 24;
 disable();observe(&ic);
 if(discovered!=3||locked)return 25;
 return 0;
}
'''
        self.assertEqual(run_native(code), 0)


if __name__ == '__main__':
    unittest.main()
