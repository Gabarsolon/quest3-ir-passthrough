/* On-device supervisor. No ADB, Python, network service, or host heartbeat. */
#define _GNU_SOURCE
#include "frida-core.h"
#include "calibration.h"
#include "assets.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#define BASE "/dev/quest3-ir-session/headset"
#define FRAMES "/dev/quest3-ir-session/headset-frames"
#define BUILD "51154110174900520"
#define MAP_BYTES (640*512*8)
static volatile sig_atomic_t interrupted;
static volatile sig_atomic_t stop_sender, stop_signal;
static _Atomic int failed, cleaning, reply_done;
static int sequence, pending_id, reply_ok;
static GMutex rpc_lock;
static JsonNode *reply;
static FridaScript *hook, *mode;
static gint64 last_heartbeat;
static const char *status = "starting";

static void fail(const char *message) { g_printerr("ERROR: %s\n", message); failed=1; }
static void on_signal(int sig,siginfo_t *info,void *context) {
    (void)context;stop_signal=sig;stop_sender=info?info->si_pid:0;interrupted=1;
}
static char *json_text(JsonNode *node) {
    JsonGenerator *gen=json_generator_new(); json_generator_set_root(gen,node);
    char *text=json_generator_to_data(gen,NULL); g_object_unref(gen); return text;
}
static char *quote(const char *text) {
    JsonNode *node=json_node_new(JSON_NODE_VALUE); json_node_set_string(node,text);
    char *out=json_text(node); json_node_free(node); return out;
}
static void save_status(JsonNode *stats) {
    JsonObject *obj=json_object_new();
    json_object_set_string_member(obj,"status",status);
    json_object_set_int_member(obj,"pid",getpid());
    json_object_set_string_member(obj,"controller","headset-native");
    json_object_set_boolean_member(obj,"laptopRequired",FALSE);
    if (stats) json_object_set_member(obj,"rendering",json_node_copy(stats));
    JsonNode *node=json_node_new(JSON_NODE_OBJECT); json_node_take_object(node,obj);
    char *text=json_text(node);
    /* Avoid GLib's atomic-write helpers: they can attempt writes beside /dev. */
    int fd=open(BASE "/status.new",O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0600);
    if (fd>=0) { write(fd,text,strlen(text)); close(fd); rename(BASE "/status.new",BASE "/status.json"); }
    g_free(text); json_node_free(node);
}
static char *command(const char *text) {
    char *out=NULL, *err=NULL; int code=0; GError *error=NULL;
    char *argv[]={"/system/bin/sh","-c",(char*)text,NULL};
    if (!g_spawn_sync(NULL,argv,NULL,G_SPAWN_DEFAULT,NULL,NULL,&out,&err,&code,&error) || code) {
        fail(error?error->message:(err&&*err?err:"Device command failed"));
    }
    g_clear_error(&error); g_free(err); return out?g_strstrip(out):g_strdup("");
}
static void post_heartbeat(FridaScript *script) {
    if (script) frida_script_post(script,"[\"frida:rpc\",0,\"call\",\"heartbeat\",[]]",NULL);
}
static void pump(void) {
    for (int i=0; i<64 && g_main_context_iteration(NULL,FALSE); i++) {}
    gint64 now=g_get_monotonic_time();
    if (!cleaning && now-last_heartbeat>=500000) {
        post_heartbeat(hook); post_heartbeat(mode); last_heartbeat=now;
    }
}
static void pause_ms(int ms) {
    gint64 end=g_get_monotonic_time()+(gint64)ms*1000;
    while (g_get_monotonic_time()<end && !interrupted && !failed) {pump(); g_usleep(2000);}
}
static void on_message(FridaScript *script,const char *message,GBytes *data,gpointer unused) {
    (void)script; (void)data; (void)unused;
    JsonParser *p=json_parser_new();
    if (!json_parser_load_from_data(p,message,-1,NULL)) {g_object_unref(p); fail("Invalid agent message"); return;}
    JsonObject *obj=json_node_get_object(json_parser_get_root(p));
    const char *type=json_object_get_string_member(obj,"type");
    JsonNode *payload=json_object_get_member(obj,"payload");
    if (!strcmp(type,"send") && payload && JSON_NODE_HOLDS_ARRAY(payload)) {
        JsonArray *a=json_node_get_array(payload);
        if (json_array_get_length(a)>=4 && !strcmp(json_array_get_string_element(a,0),"frida:rpc")) {
            g_mutex_lock(&rpc_lock);
            if (pending_id && json_array_get_int_element(a,1)==pending_id && !reply_done) {
                reply_ok=!strcmp(json_array_get_string_element(a,2),"ok");
                reply=json_node_copy(json_array_get_element(a,3));
                /* Frida may emit on its worker thread. Publish completion last,
                   after the entire result has been copied, with release ordering. */
                reply_done=1;
            }
            g_mutex_unlock(&rpc_lock);
            g_object_unref(p); return;
        }
    }
    g_print("AGENT %s\n",message);
    if (!strcmp(type,"error")) fail("Injected script error");
    if (payload && JSON_NODE_HOLDS_OBJECT(payload)) {
        JsonObject *event=json_node_get_object(payload);
        if (json_object_has_member(event,"kind") && !strcmp(json_object_get_string_member(event,"kind"),"error")) fail("IR agent stopped");
    }
    g_object_unref(p);
}
static void on_detached(FridaSession *s,FridaSessionDetachReason reason,FridaCrash *crash,gpointer unused) {
    (void)s; (void)crash; (void)unused;
    if (!cleaning) {g_printerr("Session detached: %d\n",reason); fail("Target exited or the local Frida session was lost");}
}
static JsonNode *rpc(FridaScript *script,const char *name,const char *args,GBytes *binary) {
    if (!script) return NULL;
    g_mutex_lock(&rpc_lock);
    pending_id=++sequence; reply_done=reply_ok=0; reply=NULL;
    int id=pending_id;
    g_mutex_unlock(&rpc_lock);
    char *method=quote(name);
    char *message=g_strdup_printf("[\"frida:rpc\",%d,\"call\",%s,%s]",id,method,args);
    frida_script_post(script,message,binary); g_free(message); g_free(method);
    gint64 deadline=g_get_monotonic_time()+10000000;
    while (!reply_done && g_get_monotonic_time()<deadline && (!interrupted||cleaning) && (!failed||cleaning)) {pump(); g_usleep(1000);}
    g_mutex_lock(&rpc_lock);
    pending_id=0;JsonNode *result=reply;reply=NULL;int done=reply_done,ok=reply_ok;
    g_mutex_unlock(&rpc_lock);
    if (!done || !ok || !result) {
        char *detail=result?json_text(result):g_strdup("timed out, stopped, or no result");
        g_printerr("RPC %s: %s\n",name,detail); g_free(detail); failed=1;
        if (result) json_node_free(result);result=NULL;
    }
    return result;
}
static int call(FridaScript *script,const char *name,const char *args) {
    JsonNode *r=rpc(script,name,args,NULL); if (!r) return 0; json_node_free(r); return 1;
}
static FridaScript *load_script(FridaDevice *device,int pid,const char *source,FridaSession **session) {
    GError *error=NULL;
    *session=frida_device_attach_sync(device,pid,NULL,NULL,&error);
    if (!*session) goto error;
    g_signal_connect(*session,"detached",G_CALLBACK(on_detached),NULL);
    FridaScriptOptions *options=frida_script_options_new();
    frida_script_options_set_runtime(options,FRIDA_SCRIPT_RUNTIME_QJS);
    frida_script_options_set_name(options,"quest3-ir-headset");
    FridaScript *script=frida_session_create_script_sync(*session,source,options,NULL,&error);
    g_object_unref(options);
    if (!script) goto error;
    g_signal_connect(script,"message",G_CALLBACK(on_message),NULL);
    frida_script_load_sync(script,NULL,&error);
    if (error) {g_object_unref(script); goto error;}
    return script;
error:
    fail(error?error->message:"Could not load local Frida script"); g_clear_error(&error); return NULL;
}
static int process(const char *name) {
    char *cmd=g_strdup_printf("pidof %s",name), *text=command(cmd); g_free(cmd);
    char *end; long pid=strtol(text,&end,10);
    if (pid<=1 || (*end && *end!='\n')) {fail("Expected one matching system process"); pid=0;}
    g_free(text); return (int)pid;
}
typedef struct {unsigned long long inode; int fd;} Buffer;
static int compare_buffer(const void *a,const void *b) {
    const Buffer *x=a,*y=b; return (x->inode>y->inode)-(x->inode<y->inode);
}
static int buffers(int pid,Buffer out[26]) {
    char path[160]; snprintf(path,sizeof(path),"/proc/%d/fd",pid);
    DIR *dir=opendir(path); if (!dir) {fail("Cannot inspect sensor buffer descriptors"); return 0;}
    int count=0; struct dirent *entry;
    while ((entry=readdir(dir))) {
        char *end; long fd=strtol(entry->d_name,&end,10); if (*end || fd<0 || fd>1000000) continue;
        snprintf(path,sizeof(path),"/proc/%d/fd/%ld",pid,fd);
        char target[256]; ssize_t n=readlink(path,target,sizeof(target)-1);
        if (n<0) continue; target[n]=0;
        if (!strstr(target,"dmabuf")) continue;
        struct stat st; if (stat(path,&st) || st.st_size!=327680) continue;
        if (count<26) out[count]=(Buffer){st.st_ino,(int)fd}; count++;
    }
    closedir(dir);
    if (count!=26) {fail("Expected 26 distinct 640x512 sensor buffers"); return 0;}
    qsort(out,26,sizeof(Buffer),compare_buffer);
    for (int i=1;i<26;i++) if (out[i].inode==out[i-1].inode || (i!=13 && out[i].inode-out[i-1].inode!=2)) {
        fail("Sensor allocation changed; refusing guessed eye mapping"); return 0;
    }
    return 1;
}
static int start_reader(int sensors,const char *eye,Buffer *fds) {
    pid_t parent=getpid(), pid=fork();
    if (pid!=0) {if(pid<0) fail("Reader fork failed"); return pid;}
    prctl(PR_SET_PDEATHSIG,SIGTERM); if (getppid()!=parent) _exit(1);
    char path[180], sensor[32], fdargs[13][16]; char *argv[17];
    snprintf(path,sizeof(path),BASE "/%s.log",eye);
    int log=open(path,O_WRONLY|O_CREAT|O_TRUNC,0600); if(log<0) _exit(1);
    dup2(log,1); dup2(log,2); close(log);
    int null=open("/dev/null",O_RDONLY); if(null>=0) {dup2(null,0);close(null);}
    /* The compositor has a different uid. Match the maintained reader protocol:
       only image/sequence files are readable, executables and logs stay private. */
    umask(0022);
    const char *suffix[]={".raw",".raw.tmp",".raw.rgba.seq",".raw.stamp",".raw.stamp.tmp"};
    for(int i=0;i<5;i++){snprintf(path,sizeof(path),FRAMES "/%s%s",eye,suffix[i]);chmod(path,0644);}
    snprintf(sensor,sizeof(sensor),"%d",sensors); snprintf(path,sizeof(path),FRAMES "/%s.raw",eye);
    argv[0]=BASE "/irfeed"; argv[1]=sensor; argv[2]=path;
    for(int i=0;i<13;i++){snprintf(fdargs[i],sizeof(fdargs[i]),"%d",fds[i].fd);argv[i+3]=fdargs[i];}
    argv[16]=NULL; execv(argv[0],argv); _exit(127);
}
static int read_sequence(const char *eye,unsigned long long *out) {
    char path[180]; snprintf(path,sizeof(path),FRAMES "/%s.raw.rgba.seq",eye);
    int fd=open(path,O_RDONLY|O_CLOEXEC); if(fd<0) return 0;
    ssize_t n=read(fd,out,8); close(fd); return n==8;
}
static int readers_advancing(void) {
    unsigned long long previous[2]={0,0}, value; int advances[2]={0,0}, seen[2]={0,0};
    gint64 deadline=g_get_monotonic_time()+30000000;
    while(!interrupted && !failed && g_get_monotonic_time()<deadline) {
        for(int eye=0;eye<2;eye++) if(read_sequence(eye?"right":"left",&value)) {
            if(seen[eye] && value!=previous[eye]) advances[eye]++;
            previous[eye]=value; seen[eye]=1;
        }
        if(advances[0]>=2 && advances[1]>=2) return 1;
        pause_ms(100);
    }
    fail("Both IR readers did not advance; RGB remains available"); return 0;
}
static int array_values(JsonObject *obj,const char *key,double *out,int count) {
    if(!json_object_has_member(obj,key)) return 0;
    JsonNode *node=json_object_get_member(obj,key); if(!JSON_NODE_HOLDS_ARRAY(node)) return 0;
    JsonArray *a=json_node_get_array(node); if(json_array_get_length(a)!=(guint)count) return 0;
    for(int i=0;i<count;i++){out[i]=json_array_get_double_element(a,i);if(!isfinite(out[i]))return 0;}
    return 1;
}
static JsonArray *numbers(const double *values,int count) {
    JsonArray *a=json_array_new(); for(int i=0;i<count;i++) json_array_add_double_element(a,values[i]); return a;
}
static int calibration(IrCamera camera[4]) {
    JsonParser *p=json_parser_new(); GError *error=NULL;
    if(!json_parser_load_from_file(p,"/persist/calibration/camera_calibration.json",&error)) {
        fail(error?error->message:"Cannot read factory calibration");g_clear_error(&error);g_object_unref(p);return 0;
    }
    JsonObject *root=json_node_get_object(json_parser_get_root(p));
    JsonArray *a=json_object_get_array_member(root,"CameraCalibration"); int found=0;
    if(a) for(guint i=0;i<json_array_get_length(a);i++) {
        JsonObject *c=json_array_get_object_element(a,i); JsonNode *id=json_object_get_member(c,"Id");
        int number=json_node_get_value_type(id)==G_TYPE_STRING?atoi(json_node_get_string(id)):(int)json_node_get_int(id);
        int index=number==0?0:number==1?1:number==4?2:number==5?3:-1;
        if(index<0)continue;
        JsonObject *projection=json_object_get_object_member(c,"Projection"), *distortion=json_object_get_object_member(c,"Distortion");
        double size[2];
        if(!projection || !distortion || strcmp(json_object_get_string_member(projection,"Model"),"PinholeSymmetric") ||
           strcmp(json_object_get_string_member(distortion,"Model"),"Fisheye62") ||
           !array_values(projection,"Coefficients",camera[index].projection,3) ||
           !array_values(distortion,"Coefficients",camera[index].distortion,8) ||
           !array_values(c,"DeviceFromCamera",camera[index].pose,16) || !array_values(c,"ImageSize",size,2) ||
           size[0]!=(index<2?1280:2328) || size[1]!=(index<2?1024:1748) || (found&(1<<index))) {
            fail("Unsupported or invalid factory calibration");g_object_unref(p);return 0;
        }
        found|=1<<index;
    }
    g_object_unref(p);if(found!=15){fail("Missing factory camera pair");return 0;}return 1;
}
static char *hook_source(IrCamera camera[4],double brightness,const char *filter,double sharpness,const char *aa) {
    JsonObject *obj=json_object_new();
    json_object_set_string_member(obj,"directory",FRAMES);
    json_object_set_int_member(obj,"rotate",90);json_object_set_boolean_member(obj,"mirror",FALSE);
    json_object_set_boolean_member(obj,"swapEyes",FALSE);json_object_set_boolean_member(obj,"normalize",TRUE);
    json_object_set_double_member(obj,"zoom",1);json_object_set_boolean_member(obj,"stretch",FALSE);
    json_object_set_boolean_member(obj,"calibration",TRUE);json_object_set_boolean_member(obj,"poseCorrection",TRUE);
    json_object_set_string_member(obj,"poseMethod","image");json_object_set_int_member(obj,"poseAgeMs",8);
    json_object_set_int_member(obj,"irDelayMs",0);json_object_set_string_member(obj,"filter",filter);
    json_object_set_double_member(obj,"brightness",brightness);json_object_set_double_member(obj,"sgsrSharpness",sharpness);
    json_object_set_string_member(obj,"antialiasing",aa);
    double size[]={1280,960};json_object_set_array_member(obj,"outputSize",numbers(size,2));
    JsonArray *poses=json_array_new(), *warps=json_array_new();
    for(int eye=0;eye<2;eye++) {
        double pose[7],rotation[9];ir_camera_pose(&camera[2+eye],pose);ir_rotation(&camera[eye],&camera[2+eye],rotation);
        json_array_add_array_element(poses,numbers(pose,7));
        JsonObject *warp=json_object_new();json_object_set_array_member(warp,"rotation",numbers(rotation,9));
        json_object_set_array_member(warp,"projection",numbers(camera[eye].projection,3));
        json_object_set_array_member(warp,"distortion",numbers(camera[eye].distortion,8));
        double irsize[]={1280,1024};json_object_set_array_member(warp,"size",numbers(irsize,2));
        json_array_add_object_element(warps,warp);
    }
    json_object_set_array_member(obj,"cameraPoses",poses);json_object_set_array_member(obj,"warpParameters",warps);
    JsonNode *node=json_node_new(JSON_NODE_OBJECT);json_node_take_object(node,obj);char *options=json_text(node);
    char *c=quote(asset_c),*contexts=quote(asset_contexts),*sgsr=quote(asset_sgsr),*edge=quote(asset_aa),*warp=quote(asset_warp);
    char *source=g_strdup_printf("const OPTIONS=%s;\nconst C_SOURCE=%s;\nconst CONTEXT_SOURCE=%s;\nconst SGSR_SOURCE=%s;\nconst AA_SOURCE=%s;\nconst WARP_SOURCE=%s;\n%s",options,c,contexts,sgsr,edge,warp,asset_js);
    g_free(options);g_free(c);g_free(contexts);g_free(sgsr);g_free(edge);g_free(warp);json_node_free(node);return source;
}
static int remote_control(int stop) {
    int fd=open(BASE "/controller.pid",O_RDONLY|O_CLOEXEC);
    if(fd<0 || flock(fd,LOCK_EX|LOCK_NB)==0) {
        if(fd>=0)close(fd);g_print("{\"status\":\"stopped\"}\n");return 0;
    }
    char text[32]={0};read(fd,text,sizeof(text)-1);int pid=atoi(text);
    if(stop) {
        int handle=(int)syscall(SYS_pidfd_open,pid,0); char path[80],exe[256];
        snprintf(path,sizeof(path),"/proc/%d/exe",pid);ssize_t n=readlink(path,exe,sizeof(exe)-1);
        if(n>=0)exe[n]=0;
        if(handle<0 || n<0 || strcmp(exe,BASE "/ir-headset")) {if(handle>=0)close(handle);close(fd);g_printerr("Controller identity check failed\n");return 1;}
        int result=(int)syscall(SYS_pidfd_send_signal,handle,SIGTERM,NULL,0);close(handle);close(fd);
        g_print("{\"status\":\"stopping\"}\n");return result<0;
    }
    close(fd);char *output=NULL;
    if(!g_file_get_contents(BASE "/status.json",&output,NULL,NULL)) output=g_strdup("{\"status\":\"starting\"}");
    g_print("%s\n",output);g_free(output);return 0;
}

static int rpc_self_test(void) {
    GPid child=0;GError *error=NULL;char *argv[]={"/system/bin/sleep","120",NULL};
    if(!g_spawn_async(NULL,argv,NULL,G_SPAWN_DO_NOT_REAP_CHILD,NULL,NULL,&child,&error)) {
        fail(error?error->message:"Cannot start owned test process");g_clear_error(&error);return 1;
    }
    FridaDeviceManager *manager=frida_device_manager_new();
    FridaDevice *device=frida_device_manager_get_device_by_type_sync(manager,FRIDA_DEVICE_TYPE_LOCAL,0,NULL,&error);
    FridaSession *session=NULL;FridaScript *script=NULL;
    if(device)script=load_script(device,child,"rpc.exports.echo=(n,data)=>({n,size:data?data.byteLength:0});",&session);
    else {fail(error?error->message:"No local device");g_clear_error(&error);}
    char bytes[65536]={0};GBytes *data=g_bytes_new(bytes,sizeof(bytes));
    if(script) for(int i=0;i<2000 && !failed;i++) {
        char args[32];snprintf(args,sizeof(args),"[%d]",i);
        JsonNode *result=rpc(script,"echo",args,(i%2)?data:NULL);
        if(!result)break;
        JsonObject *obj=json_node_get_object(result);
        if(json_object_get_int_member(obj,"n")!=i || json_object_get_int_member(obj,"size")!=((i%2)?65536:0))fail("RPC result or binary payload mismatched");
        json_node_free(result);
    }
    g_bytes_unref(data);cleaning=1;
    if(script){frida_script_unload_sync(script,NULL,NULL);g_object_unref(script);}
    if(session){frida_session_detach_sync(session,NULL,NULL);g_object_unref(session);}
    if(device)g_object_unref(device);frida_device_manager_close_sync(manager,NULL,NULL);g_object_unref(manager);
    kill(child,SIGTERM);waitpid(child,NULL,0);
    if(!failed)g_print("Passed 2000 native RPC round trips, including 1000 binary payloads. Only an owned sleep process was instrumented.\n");
    return failed?1:0;
}

int main(int argc,char **argv) {
    /* The devkit's private GLib allocator must be initialized before any g_* call. */
    setenv("TMPDIR",BASE,1);frida_init();
    double brightness=1,seconds=0,sharpness=1.5;int refresh=0,fast=1,check=0,calibration_json=0,rpc_test=0;
    const char *filter="linear",*aa="off";
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"--status"))return remote_control(0);
        if(!strcmp(argv[i],"--stop"))return remote_control(1);
        if(!strcmp(argv[i],"--check-only")){check=1;continue;}
        if(!strcmp(argv[i],"--calibration-json")){calibration_json=1;continue;}
        if(!strcmp(argv[i],"--rpc-self-test")){rpc_test=1;continue;}
        if(!strcmp(argv[i],"--no-fast-mode")){fast=0;continue;}
        if(!strcmp(argv[i],"--help")){puts("ir-headset [--seconds N] [--brightness 0..4] [--display-refresh 72|80|90|120] [--filter linear|cubic|sgsr] [--sgsr-sharpness 1..2] [--aa off|edge] [--no-fast-mode] [--status|--stop|--check-only]");return 0;}
        if(i+1>=argc){fputs("Missing argument\n",stderr);return 2;}
        const char *name=argv[i++],*value=argv[i];char *end;
        if(!strcmp(name,"--filter")){filter=value;continue;}
        if(!strcmp(name,"--aa")){aa=value;continue;}
        double n=strtod(value,&end);if(*end || !*value || !isfinite(n)){fputs("Invalid number\n",stderr);return 2;}
        if(!strcmp(name,"--brightness"))brightness=n;
        else if(!strcmp(name,"--seconds"))seconds=n;
        else if(!strcmp(name,"--sgsr-sharpness"))sharpness=n;
        else if(!strcmp(name,"--display-refresh") && (n==72||n==80||n==90||n==120))refresh=(int)n;
        else{fputs("Unknown option\n",stderr);return 2;}
    }
    if(brightness<0||brightness>4||seconds<0||sharpness<1||sharpness>2||
       (refresh && refresh!=72 && refresh!=80 && refresh!=90 && refresh!=120)||
       (strcmp(filter,"linear")&&strcmp(filter,"cubic")&&strcmp(filter,"sgsr"))||(strcmp(aa,"off")&&strcmp(aa,"edge"))) {
        fputs("Invalid option value\n",stderr);return 2;
    }
    if(getuid()!=0){fputs("Temporary root is required; this controller does not run an exploit.\n",stderr);return 1;}
    struct statfs fs;if(statfs("/dev",&fs)||fs.f_type!=0x01021994){fputs("/dev must be tmpfs\n",stderr);return 1;}
    char *build=command("getprop ro.build.version.incremental");
    if(strcmp(build,BUILD)){g_free(build);fputs("Unsupported headset build\n",stderr);return 1;}g_free(build);
    IrCamera camera[4];if(!calibration(camera))return 1;
    if(rpc_test)return rpc_self_test();
    if(calibration_json) {
        JsonObject *obj=json_object_new();JsonArray *maps=json_array_new(),*poses=json_array_new(),*rotations=json_array_new();
        for(int eye=0;eye<2;eye++) {
            float sample[53*41*2];double pose[7],rotation[9];
            if(!ir_ray_map(&camera[2+eye],53,41,sample))return 1;
            char *encoded=g_base64_encode((guchar*)sample,sizeof(sample));json_array_add_string_element(maps,encoded);g_free(encoded);
            ir_camera_pose(&camera[2+eye],pose);ir_rotation(&camera[eye],&camera[2+eye],rotation);
            json_array_add_array_element(poses,numbers(pose,7));json_array_add_array_element(rotations,numbers(rotation,9));
        }
        json_object_set_array_member(obj,"sampleRayMaps",maps);json_object_set_array_member(obj,"cameraPoses",poses);
        json_object_set_array_member(obj,"rotations",rotations);
        JsonNode *node=json_node_new(JSON_NODE_OBJECT);json_node_take_object(node,obj);char *text=json_text(node);
        puts(text);g_free(text);json_node_free(node);return 0;
    }
    if(check){puts("Build, temporary root, tmpfs and per-headset factory calibration verified. No hooks installed.");return 0;}
    mkdir("/dev/quest3-ir-session",0755);mkdir(BASE,0700);chmod(BASE,0700);
    mkdir(FRAMES,0755);chmod(FRAMES,0755);umask(0077);
    int lock=open(BASE "/controller.pid",O_RDWR|O_CREAT|O_CLOEXEC,0600);
    if(lock<0||flock(lock,LOCK_EX|LOCK_NB)){fputs("Another headset controller is running\n",stderr);return 1;}
    ftruncate(lock,0);dprintf(lock,"%d\n",getpid());
    char *existing=command("pidof irfeed 2>/dev/null || true");
    if(*existing){g_free(existing);fputs("Stop the existing laptop IR controller before starting headset IR.\n",stderr);close(lock);return 1;}g_free(existing);
    struct sigaction action={0};action.sa_sigaction=on_signal;action.sa_flags=SA_SIGINFO;
    sigemptyset(&action.sa_mask);sigaction(SIGINT,&action,NULL);sigaction(SIGTERM,&action,NULL);
    signal(SIGHUP,SIG_IGN);signal(SIGPIPE,SIG_IGN);
    save_status(NULL);
    int readers[2]={0,0},restore_pipe[2]={-1,-1};pid_t restore_pid=0;
    FridaDeviceManager *manager=frida_device_manager_new();FridaDevice *device=NULL;
    FridaSession *session=NULL,*mode_session=NULL;float *maps[2]={NULL,NULL};
    char *old_refresh=NULL;GError *error=NULL;
    for(int eye=0;eye<2;eye++) {
        maps[eye]=g_malloc(MAP_BYTES);
        if(!ir_ray_map(&camera[2+eye],640,512,maps[eye])){fail("Factory ray map is invalid");goto cleanup;}
    }
    if(interrupted)goto cleanup;
    g_print("Factory ray maps calculated locally for both eyes.\n");
    if(refresh) {
        old_refresh=command("runcon u:r:shell:s0 /system/bin/getprop debug.oculus.refreshRate");
        if(failed || pipe2(restore_pipe,O_CLOEXEC)) {fail("Cannot start display restoration guard");goto cleanup;}
        restore_pid=fork();
        if(restore_pid<0){fail("Display guard fork failed");goto cleanup;}
        if(restore_pid==0) {
            close(restore_pipe[1]);close(lock);char b;
            while(read(restore_pipe[0],&b,1)<0 && errno==EINTR){}
            /* Exact previous override, even if the controller is killed. */
            execl("/system/bin/runcon","runcon","u:r:shell:s0","/system/bin/setprop","debug.oculus.refreshRate",old_refresh,NULL);_exit(1);
        }
        close(restore_pipe[0]);restore_pipe[0]=-1;
        char *cmd=g_strdup_printf("runcon u:r:shell:s0 /system/bin/sh -c 'setprop debug.oculus.refreshRate %d; getprop debug.oculus.refreshRate'",refresh),*actual=command(cmd);
        g_free(cmd);if(atoi(actual)!=refresh)fail("Display override was rejected");g_free(actual);if(failed)goto cleanup;
    }
    {char *out=command("runcon u:r:shell:s0 /system/bin/sh -c 'am broadcast -a com.oculus.vrpowermanager.prox_close >/dev/null; input keyevent KEYCODE_WAKEUP'");g_free(out);if(failed)goto cleanup;}
    device=frida_device_manager_get_device_by_type_sync(manager,FRIDA_DEVICE_TYPE_LOCAL,0,NULL,&error);
    if(!device){fail(error?error->message:"No local Frida device");g_clear_error(&error);goto cleanup;}
    int compositor=process("com.oculus.vrruntimeservice");if(failed)goto cleanup;
    if(fast) {
        int fidelity=process("trackingfidelityservice");if(failed)goto cleanup;
        mode=load_script(device,fidelity,asset_mode,&mode_session);if(!mode)goto cleanup;
        char *dump=command("runcon u:r:shell:s0 dumpsys oculus.internal.ITrackingFidelityService/default");
        GRegex *pattern=g_regex_new("worldTracking Translator:.*?MuxMode = (\\w+)",G_REGEX_DOTALL,0,NULL);GMatchInfo *match=NULL;
        char *name=NULL;if(g_regex_match(pattern,dump,0,&match))name=g_match_info_fetch(match,1);
        g_match_info_free(match);g_regex_unref(pattern);g_free(dump);
        if(!name || failed){g_free(name);fail("Cannot read original tracking mode");goto cleanup;}
        char *quoted=quote(name),*args=g_strdup_printf("[%s,60]",quoted);g_free(quoted);g_free(name);
        int ok=call(mode,"activate",args);g_free(args);if(!ok)goto cleanup;pause_ms(400);
    }
    int sensors=process("vendor.oculus.hardware.sensors@1.0-service");Buffer pool[26];
    if(failed||!buffers(sensors,pool))goto cleanup;
    g_print("Fresh sensor mapping: PID %d, 13 buffers per eye.\n",sensors);
    readers[0]=start_reader(sensors,"left",pool);readers[1]=start_reader(sensors,"right",pool+13);
    if(failed||!readers_advancing())goto cleanup;
    {char *source=hook_source(camera,brightness,filter,sharpness,aa);hook=load_script(device,compositor,source,&session);g_free(source);if(!hook)goto cleanup;}
    for(int eye=0;eye<2;eye++) for(int offset=0;offset<MAP_BYTES;offset+=65536) {
        int length=MAP_BYTES-offset;if(length>65536)length=65536;
        GBytes *data=g_bytes_new((char*)maps[eye]+offset,length);char args[64];snprintf(args,sizeof(args),"[%d,%d]",eye,offset);
        JsonNode *result=rpc(hook,"setmapchunk",args,data);g_bytes_unref(data);if(!result)goto cleanup;json_node_free(result);
    }
    for(int eye=0;eye<2;eye++){g_free(maps[eye]);maps[eye]=NULL;}
    status="waiting for normal passthrough";save_status(NULL);
    g_print("Headset controller ready. No laptop heartbeat or ADB connection is required.\n");
    gint64 started=g_get_monotonic_time(),last_check=0;unsigned long long previous[2]={0,0};int stale=0;
    while(!interrupted && !failed) {
        JsonNode *stats=rpc(hook,"stats","[]",NULL);if(!stats)break;
        JsonObject *obj=json_node_get_object(stats);
        if(!json_object_get_boolean_member(obj,"enabled")||json_object_get_int_member(obj,"errors"))fail("IR hook stopped; restoring RGB");
        if(json_object_get_int_member(obj,"substitutedDraws")) {
            if(strcmp(status,"IR active")){status="IR active";g_print("IR ACTIVE (headset-native).\n");}
        }
        save_status(stats);json_node_free(stats);
        gint64 now=g_get_monotonic_time();
        if(now-last_check>3000000) {
            unsigned long long value[2];int ok=read_sequence("left",&value[0])&&read_sequence("right",&value[1]);
            if(!ok || (last_check && (value[0]==previous[0]||value[1]==previous[1])))stale++;else stale=0;
            if(ok)memcpy(previous,value,sizeof(value));last_check=now;
            if(stale>=2)fail("IR reader stopped updating");
            for(int i=0;i<2;i++) if(readers[i]>0 && waitpid(readers[i],NULL,WNOHANG)==readers[i]){readers[i]=0;fail("IR reader exited");}
        }
        if(seconds && now-started>=seconds*1000000)break;
        pause_ms(500);
    }
cleanup:
    g_print("Controller cleanup: signal=%d sender=%d failed=%d seconds=%.3f\n",(int)stop_signal,(int)stop_sender,failed,seconds);
    cleaning=1;status="restoring RGB";save_status(NULL);
    if(hook) {
        call(hook,"stop","[]");
        for(int i=0;i<20;i++) {
            JsonNode *s=rpc(hook,"stats","[]",NULL);if(!s)break;
            int done=json_object_get_boolean_member(json_node_get_object(s),"cleaned");json_node_free(s);if(done)break;
            g_usleep(50000);pump();
        }
        call(hook,"dispose","[]");frida_script_unload_sync(hook,NULL,NULL);g_object_unref(hook);hook=NULL;
    }
    if(session){frida_session_detach_sync(session,NULL,NULL);g_object_unref(session);}
    if(mode){call(mode,"restore","[]");frida_script_unload_sync(mode,NULL,NULL);g_object_unref(mode);mode=NULL;}
    if(mode_session){frida_session_detach_sync(mode_session,NULL,NULL);g_object_unref(mode_session);}
    for(int eye=0;eye<2;eye++) {if(readers[eye]>0){kill(readers[eye],SIGTERM);waitpid(readers[eye],NULL,0);}g_free(maps[eye]);}
    if(restore_pipe[1]>=0)close(restore_pipe[1]);if(restore_pipe[0]>=0)close(restore_pipe[0]);
    if(restore_pid>0)waitpid(restore_pid,NULL,0);g_free(old_refresh);
    if(device)g_object_unref(device);frida_device_manager_close_sync(manager,NULL,NULL);g_object_unref(manager);
    status=failed?"stopped after error":"stopped";save_status(NULL);close(lock);
    g_print("Headset IR stopped; RGB hooks removed.\n");return failed?1:0;
}
