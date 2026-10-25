/* Compiled in RAM by Frida CModule. Never modifies a camera-owned buffer. */
#include <gum/guminterceptor.h>

typedef unsigned int U;
typedef unsigned long long Q;
typedef struct { long sec, nsec; } Clock;
extern int clock_gettime(int, Clock *);
extern int open(const char *, int, ...);
extern long read(int, void *, unsigned long);
extern int close(int);
extern void glGetIntegerv(U, int *);
extern void glGetBooleanv(U, unsigned char *);
extern unsigned char glIsEnabled(U);
extern void glEnable(U), glDisable(U);
extern int glGetUniformLocation(U, const char *);
extern void glGetUniformiv(U, int, int *);
extern void glActiveTexture(U), glBindTexture(U, U);
extern void glBindFramebuffer(U, U);
extern void glFramebufferTexture2D(U, U, U, U, int);
extern U glCheckFramebufferStatus(U), glGetError(void);
extern void glBindBuffer(U, U), glPixelStorei(U, int);
extern void glTexSubImage2D(U, int, int, int, int, int, U, U, const void *);
extern void glViewport(int, int, int, int), glColorMask(unsigned char, unsigned char, unsigned char, unsigned char);
extern void glUseProgram(U), glUniform1i(int, int), glUniform2f(int, float, float);
extern void glDrawArrays(U, int, int);
extern void glGetIntegeri_v(U, U, int *), glGetInteger64i_v(U, U, long long *);
extern void glGetBufferPointerv(U, U, void **), glGenBuffers(int, U *), glDeleteBuffers(int, const U *);
extern void glGetBufferParameteriv(U, U, int *);
extern unsigned char glIsBuffer(U);
extern void glBufferData(U, long, const void *, U), glBufferSubData(U, long, long, const void *);
extern void glBufferStorageEXT(U, long, const void *, U);
extern void *glMapBufferRange(U, long, long, U);
extern void *glFenceSync(U, U);
extern U glClientWaitSync(void *, U, Q);
extern void glDeleteSync(void *);
extern void glBindBufferRange(U, U, U, long, long);
extern void glGetUniformIndices(U, int, const char **, U *), glGetActiveUniformsiv(U, int, const U *, U, int *);
extern void glGetActiveUniformBlockiv(U, U, U, int *);
extern void initialize_resources(void), release_resources(void);
typedef struct { unsigned char bytes[168]; } Pose;
extern Pose historical_pose(void *, long *);
typedef struct {
  U buffer;
  long offset;
  unsigned char *pointer;
  float original[14], written[14];
} PoseRecord;
extern PoseRecord pose_records[];

typedef struct {
  const char *path, *sequence_path, *stamp_path;
  unsigned char *pixels;
  U upload_texture, yuv_texture, map_texture, valid, uploads;
  Q sequence;
  int has_sequence;
  U last_hash, changed_samples;
  Q ingested_sequence, history_times[16], history_sequences[16];
  U has_ingested, history_count, history_next;
  Q history_capture_ns[16];
  float camera_pose[7];
  U pose_valid;
} Source;

typedef struct {
  volatile U enabled, ready, cleaned, errors;
  U normalize, swap_eyes, program, framebuffer, width, height, delay_ms;
  int image_location, range_location, map_location;
  Q heartbeat, last_refresh, last_substitution, draws, refreshes;
  Source sources[2];
  U cache_count;
  struct { U program; int location, pose_checked; } cache[64];
  void *tracker;
  float camera_from_device[2][7];
  U pose_age_ms, pose_buffer, pose_stride, pose_next;
  U pose_direct, pose_record_count;
  unsigned char *pose_mapping;
  void *pose_fences[32];
  struct { U buffer; unsigned char *pointer; } original_mappings[64];
  U original_mapping_count;
  Q pose_draws, pose_samples, pose_failures;
  unsigned char pose_scratch[4096];
} Control;

extern Control control;

static Q now_ms(void) {
  Clock t;
  clock_gettime(1, &t);
  return ((Q)t.sec * 1000) + ((Q)t.nsec / 1000000);
}

void configure(const char *left, const char *right, const char *left_seq,
    const char *right_seq, void *left_pixels, void *right_pixels,
    int normalize, int swap_eyes) {
  control.sources[0].path = left;
  control.sources[1].path = right;
  control.sources[0].sequence_path = left_seq;
  control.sources[1].sequence_path = right_seq;
  control.sources[0].pixels = left_pixels;
  control.sources[1].pixels = right_pixels;
  control.normalize = normalize;
  control.swap_eyes = swap_eyes;
  control.heartbeat = now_ms();
  control.enabled = 1;
}

void set_resources(U program, U framebuffer, U left_upload, U right_upload,
    U left_yuv, U right_yuv, int image_location, int range_location, U width, U height) {
  control.program = program;
  control.framebuffer = framebuffer;
  control.sources[0].upload_texture = left_upload;
  control.sources[1].upload_texture = right_upload;
  control.sources[0].yuv_texture = left_yuv;
  control.sources[1].yuv_texture = right_yuv;
  control.image_location = image_location;
  control.range_location = range_location;
  control.width = width;
  control.height = height;
  control.ready = 1;
}

void heartbeat(void) { control.heartbeat = now_ms(); }
void set_delay(U value) { if (value <= 200) control.delay_ms = value; }
void configure_pose(void *tracker, const float *cameras, U age,
    const char *left_stamp, const char *right_stamp) {
  int i, j;
  control.tracker = tracker;
  control.pose_age_ms = age;
  control.sources[0].stamp_path = left_stamp;
  control.sources[1].stamp_path = right_stamp;
  for (i = 0; i < 2; i++) for (j = 0; j < 7; j++)
    control.camera_from_device[i][j] = cameras[i * 7 + j];
}
void set_pose_age(U value) { if (value <= 100) control.pose_age_ms = value; }
void set_pose_method(U direct) { control.pose_direct = direct; }
void set_maps(U left, U right, int location) {
  control.sources[0].map_texture = left;
  control.sources[1].map_texture = right;
  control.map_location = location;
}
void stop(void) { control.enabled = 0; }
void fail(void) { control.errors++; control.enabled = 0; }

void stats(Q *out) {
  out[0] = control.enabled; out[1] = control.ready;
  out[2] = control.cleaned; out[3] = control.errors;
  out[4] = control.draws; out[5] = control.refreshes;
  out[6] = control.last_substitution;
  out[7] = control.sources[0].uploads; out[8] = control.sources[1].uploads;
  out[9] = control.sources[0].sequence; out[10] = control.sources[1].sequence;
  out[11] = control.sources[0].valid; out[12] = control.sources[1].valid;
  out[13] = control.sources[0].changed_samples;
  out[14] = control.sources[1].changed_samples;
  out[15] = control.pose_draws; out[16] = control.pose_samples;
  out[17] = control.pose_failures; out[18] = control.pose_age_ms;
}

static int integer(U key) { int value; glGetIntegerv(key, &value); return value; }

static int read_all(const char *path, void *buffer, int length) {
  int fd = open(path, 0);
  int total = 0;
  if (fd < 0) return 0;
  while (total < length) {
    long n = read(fd, (char *)buffer + total, length - total);
    if (n <= 0) break;
    total += n;
  }
  close(fd);
  return total == length;
}

static void multiply_quaternion(const float *a, const float *b, float *q) {
  q[0] = a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1];
  q[1] = a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0];
  q[2] = a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3];
  q[3] = a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2];
}

static int capture_pose(Source *source, Q observed_ns) {
  long timestamp = observed_ns - (Q)control.pose_age_ms * 1000000;
  Pose pose = historical_pose(control.tracker, &timestamp);
  float *head = (float *)(pose.bytes + 16), *position = (float *)(pose.bytes + 32);
  if (!pose.bytes[0] || !pose.bytes[160]) { source->pose_valid = 0; control.pose_failures++; return 0; }
  float norm = head[0]*head[0]+head[1]*head[1]+head[2]*head[2]+head[3]*head[3];
  if (!(norm > 0.99f && norm < 1.01f)) { source->pose_valid = 0; control.pose_failures++; return 0; }
  float inverse[4] = {-head[0], -head[1], -head[2], head[3]};
  const float *extrinsic = control.camera_from_device[source - control.sources];
  float *q = source->camera_pose;
  multiply_quaternion(extrinsic, inverse, q);
  float vx=position[0], vy=position[1], vz=position[2];
  float tx=2*(q[1]*vz-q[2]*vy), ty=2*(q[2]*vx-q[0]*vz), tz=2*(q[0]*vy-q[1]*vx);
  q[4] = extrinsic[4]-(vx+q[3]*tx+q[1]*tz-q[2]*ty);
  q[5] = extrinsic[5]-(vy+q[3]*ty+q[2]*tx-q[0]*tz);
  q[6] = extrinsic[6]-(vz+q[3]*tz+q[0]*ty-q[1]*tx);
  source->pose_valid = 1;
  control.pose_samples++;
  return 1;
}

static void refresh_source(Source *s) {
  Q sequence;
  if (!read_all(s->sequence_path, &sequence, 8)) return;
  if (!s->has_ingested || sequence != s->ingested_sequence) {
    U slot = s->history_next;
    Q stamp[2] = {0, 0};
    if (control.tracker && (!read_all(s->stamp_path, stamp, 16) || stamp[0] != sequence)) return;
    if (!read_all(s->path, s->pixels + (slot + 1) * 640 * 512, 640 * 512)) return;
    s->history_times[slot] = now_ms();
    s->history_capture_ns[slot] = stamp[1];
    s->history_sequences[slot] = sequence;
    s->ingested_sequence = sequence;
    s->has_ingested = 1;
    s->history_next = (slot + 1) & 15;
    if (s->history_count < 16) s->history_count++;
  }
  if (!s->history_count) return;
  Q target = now_ms() - control.delay_ms;
  int best = -1, oldest = 0, index;
  for (index = 0; index < (int)s->history_count; index++) {
    if (s->history_times[index] < s->history_times[oldest]) oldest = index;
    if (s->history_times[index] <= target &&
        (best < 0 || s->history_times[index] > s->history_times[best])) best = index;
  }
  if (best < 0) best = oldest;
  sequence = s->history_sequences[best];
  if (s->has_sequence && sequence == s->sequence) return;
  if (control.tracker && !capture_pose(s, s->history_capture_ns[best])) return;
  unsigned char *pixels = s->pixels + (best + 1) * 640 * 512;
  U hash = 2166136261U;
  int sample;
  for (sample = 0; sample < 640 * 512; sample += 16)
    hash = (hash ^ pixels[sample]) * 16777619U;
  if (!s->has_sequence || hash != s->last_hash) s->changed_samples++;
  s->last_hash = hash;
  float low = 0.0f, scale = 1.0f;
  if (control.normalize) {
    U histogram[256] = {0};
    int x, y, sum = 0, lo = -1, hi = 255;
    for (y = 0; y < 512; y += 4)
      for (x = 0; x < 640; x += 4) histogram[pixels[y * 640 + x]]++;
    for (x = 0; x < 256; x++) {
      sum += histogram[x];
      if (lo < 0 && sum >= 1024) lo = x;
      if (sum >= 18432) { hi = x; break; }
    }
    low = (float)lo / 255.0f;
    scale = 255.0f / (float)(hi - lo < 12 ? 12 : hi - lo);
  }
  glBindTexture(0x0de1, s->upload_texture);
  glTexSubImage2D(0x0de1, 0, 0, 0, 640, 512, 0x1903, 0x1401, pixels);
  if (s->map_texture) {
    glActiveTexture(0x84ce);
    glBindTexture(0x0de1, s->map_texture);
    glActiveTexture(0x84cf);
  }
  glFramebufferTexture2D(0x8ca9, 0x8ce0, 0x8d65, s->yuv_texture, 0);
  if (glCheckFramebufferStatus(0x8ca9) != 0x8cd5) { fail(); return; }
  glUniform2f(control.range_location, low, scale);
  glDrawArrays(4, 0, 3);
  if (glGetError() != 0) { fail(); return; }
  s->sequence = sequence;
  s->has_sequence = 1;
  s->valid = 1;
  s->uploads++;
}

static void refresh(void) {
  const U caps[] = {0x0be2, 0x0b71, 0x0b44, 0x0b90, 0x0c11, 0x8c89, 0x809e, 0x80a0};
  int viewport[4], i;
  unsigned char enabled[8], mask[4];
  int active = integer(0x84e0), program = integer(0x8b8d);
  int framebuffer = integer(0x8ca6), unpack = integer(0x88ef);
  int alignment = integer(0x0cf5), row = integer(0x0cf2);
  int skip_rows = integer(0x0cf3), skip_pixels = integer(0x0cf4);
  glGetIntegerv(0x0ba2, viewport);
  glGetBooleanv(0x0c23, mask);
  for (i = 0; i < 8; i++) enabled[i] = glIsEnabled(caps[i]);
  glActiveTexture(0x84cf);
  int texture_2d = integer(0x8069), texture_external = integer(0x8d67);
  glActiveTexture(0x84ce);
  int map_binding = integer(0x8069);
  glActiveTexture(0x84cf);
  glBindBuffer(0x88ec, 0);
  glPixelStorei(0x0cf5, 1); glPixelStorei(0x0cf2, 0);
  glPixelStorei(0x0cf3, 0); glPixelStorei(0x0cf4, 0);

  if (!control.ready) initialize_resources();
  if (control.ready && control.enabled) {
    glBindFramebuffer(0x8ca9, control.framebuffer);
    glUseProgram(control.program);
    glUniform1i(control.image_location, 15);
    if (control.sources[0].map_texture) glUniform1i(control.map_location, 14);
    glViewport(0, 0, control.width, control.height);
    glColorMask(1, 1, 1, 1);
    for (i = 0; i < 8; i++) glDisable(caps[i]);
    refresh_source(&control.sources[0]);
    if (control.enabled) refresh_source(&control.sources[1]);
    control.refreshes++;
  }

  glBindFramebuffer(0x8ca9, framebuffer);
  glUseProgram(program);
  glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
  glColorMask(mask[0], mask[1], mask[2], mask[3]);
  for (i = 0; i < 8; i++) { if (enabled[i]) glEnable(caps[i]); else glDisable(caps[i]); }
  glBindTexture(0x0de1, texture_2d);
  glBindTexture(0x8d65, texture_external);
  glActiveTexture(0x84ce);
  glBindTexture(0x0de1, map_binding);
  glActiveTexture(0x84cf);
  glBindBuffer(0x88ec, unpack);
  glPixelStorei(0x0cf5, alignment); glPixelStorei(0x0cf2, row);
  glPixelStorei(0x0cf3, skip_rows); glPixelStorei(0x0cf4, skip_pixels);
  glActiveTexture(active);
}

typedef struct {
  int active, texture, unit, replaced, ubo_replaced, ubo, generic_ubo, fence_block;
  long long ubo_start, ubo_size;
} Invocation;

static int validate_pose_layout(U program) {
  const char *names[] = {"ColorCameraTopRowRotationQuaternion", "ColorCameraTopRowTranslation",
      "ColorCameraBottomRowRotationQuaternion", "ColorCameraBottomRowTranslation"};
  U indices[4]; int offsets[4], blocks[4], binding, i;
  glGetUniformIndices(program, 4, names, indices);
  for (i = 0; i < 4; i++) if (indices[i] == 0xffffffffU) return 0;
  glGetActiveUniformsiv(program, 4, indices, 0x8a3b, offsets);
  glGetActiveUniformsiv(program, 4, indices, 0x8a3a, blocks);
  for (i = 0; i < 4; i++) if (offsets[i] != 2912 + i*16 || blocks[i] != blocks[0]) return 0;
  glGetActiveUniformBlockiv(program, blocks[0], 0x8a3f, &binding);
  return binding == 0;
}

static const int camera_offsets[14] = {
  2912,2916,2920,2924,2928,2932,2936,
  2944,2948,2952,2956,2960,2964,2968
};

static void restore_direct_poses(void) {
  U i, j;
  int old = integer(0x8a28);
  for (i = 0; i < control.pose_record_count; i++) {
    PoseRecord *record = &pose_records[i];
    if (!glIsBuffer(record->buffer)) continue;
    void *pointer = 0;
    glBindBuffer(0x8a11, record->buffer);
    glGetBufferPointerv(0x8a11, 0x88bd, &pointer);
    /* A name can be reused after a buffer allocation. Restore only the exact
     * current mapping and fields still containing our last correction. */
    int capacity = 0;
    glGetBufferParameteriv(0x8a11, 0x8764, &capacity);
    if (pointer != record->pointer || record->offset < 0 || record->offset+3184 > capacity) continue;
    unsigned char *group = (unsigned char *)pointer + record->offset;
    for (j = 0; j < 14; j++) if (*(float *)(group+camera_offsets[j]) != record->written[j]) break;
    if (j != 14) continue;
    for (j = 0; j < 14; j++) *(float *)(group+camera_offsets[j]) = record->original[j];
  }
  glBindBuffer(0x8a11, old);
  control.pose_record_count = 0;
}

static int write_ir_pose(Source *source, Invocation *call) {
  int buffer = 0, old = integer(0x8a28), i;
  long long start = 0, size = 0;
  unsigned char *pointer = 0;
  if (!source->pose_valid) return 0;
  glGetIntegeri_v(0x8a28, 0, &buffer);
  glGetInteger64i_v(0x8a29, 0, &start);
  glGetInteger64i_v(0x8a2a, 0, &size);
  if (!buffer || size != 3184) { fail(); return 0; }
  glBindBuffer(0x8a11, buffer);
  glGetBufferPointerv(0x8a11, 0x88bd, (void **)&pointer);
  int flags = 0, capacity = 0;
  glGetBufferParameteriv(0x8a11, 0x911f, &flags);
  glGetBufferParameteriv(0x8a11, 0x8764, &capacity);
  glBindBuffer(0x8a11, old);
  if (!pointer || (flags & 0xc2) != 0xc2 || start < 0 || start+3184 > capacity) { fail(); return 0; }
  PoseRecord *record = 0;
  for (i = 0; i < (int)control.pose_record_count; i++)
    if (pose_records[i].buffer == buffer && pose_records[i].offset == start && pose_records[i].pointer == pointer) {
      record = &pose_records[i]; break;
    }
  if (!record) {
    if (control.pose_record_count >= 1024) { fail(); return 0; }
    record = &pose_records[control.pose_record_count++];
    record->buffer = buffer; record->offset = start; record->pointer = pointer;
    for (i = 0; i < 14; i++) record->original[i] = *(float *)(pointer+start+camera_offsets[i]);
  } else {
    for (i = 0; i < 14; i++) if (*(float *)(pointer+start+camera_offsets[i]) != record->written[i]) break;
    if (i != 14) for (i = 0; i < 14; i++) record->original[i] = *(float *)(pointer+start+camera_offsets[i]);
  }
  for (i = 0; i < 14; i++) {
    float value = source->camera_pose[i % 7];
    *(float *)(pointer+start+camera_offsets[i]) = value;
    record->written[i] = value;
  }
  control.pose_draws++;
  return 1;
}

static int bind_ir_pose(Source *source, Invocation *call) {
  unsigned char *pointer = 0;
  if (!source->pose_valid) return 0;
  glGetIntegeri_v(0x8a28, 0, &call->ubo);
  glGetInteger64i_v(0x8a29, 0, &call->ubo_start);
  glGetInteger64i_v(0x8a2a, 0, &call->ubo_size);
  if (!call->ubo || call->ubo_size != 3184) { fail(); return 0; }
  call->generic_ubo = integer(0x8a28);
  int i;
  for (i = 0; i < (int)control.original_mapping_count; i++)
    if (control.original_mappings[i].buffer == call->ubo) { pointer = control.original_mappings[i].pointer; break; }
  if (!pointer) {
    glBindBuffer(0x8a11, call->ubo);
    glGetBufferPointerv(0x8a11, 0x88bd, (void **)&pointer);
    if (pointer && control.original_mapping_count < 64) {
      U slot = control.original_mapping_count++;
      control.original_mappings[slot].buffer = call->ubo;
      control.original_mappings[slot].pointer = pointer;
    }
  }
  if (!pointer) { glBindBuffer(0x8a11, call->generic_ubo); fail(); return 0; }
  if (!control.pose_buffer) {
    int alignment = integer(0x8a34);
    if (alignment <= 0 || alignment > 4096) { glBindBuffer(0x8a11, call->generic_ubo); fail(); return 0; }
    control.pose_stride = (3184 + alignment - 1) / alignment * alignment;
    glGenBuffers(1, &control.pose_buffer);
    glBindBuffer(0x8a11, control.pose_buffer);
    long size = (long)control.pose_stride * 4096;
    glBufferStorageEXT(0x8a11, size, 0, 0xc2);
    control.pose_mapping = glMapBufferRange(0x8a11, 0, size, 0xc2);
    glBindBuffer(0x8a11, call->generic_ubo);
    if (!control.pose_mapping || glGetError()) { fail(); return 0; }
  }
  U slot = control.pose_next, block = slot >> 7;
  if (!(slot & 127) && control.pose_fences[block]) {
    U result = glClientWaitSync(control.pose_fences[block], 0, 0);
    if (result != 0x911a && result != 0x911c) { fail(); return 0; }
    glDeleteSync(control.pose_fences[block]); control.pose_fences[block] = 0;
  }
  long offset = (long)control.pose_stride * slot;
  unsigned char *target = control.pose_mapping + offset;
  for (i = 0; i < 3184 / 8; i++) ((Q *)target)[i] = ((Q *)(pointer + call->ubo_start))[i];
  /* Own UBO storage: never edit the compositor's persistently mapped memory.
   * All other geometry, depth, eye prediction and distortion stay intact. */
  for (i = 0; i < 4; i++) {
    ((float *)(target+2912))[i] = source->camera_pose[i];
    ((float *)(target+2944))[i] = source->camera_pose[i];
  }
  for (i = 0; i < 3; i++) {
    ((float *)(target+2928))[i] = source->camera_pose[4+i];
    ((float *)(target+2960))[i] = source->camera_pose[4+i];
  }
  control.pose_next = (control.pose_next+1) & 4095;
  glBindBufferRange(0x8a11, 0, control.pose_buffer, offset, 3184);
  glBindBuffer(0x8a11, call->generic_ubo);
  if ((slot & 127) == 127) call->fence_block = block;
  call->ubo_replaced = 1;
  control.pose_draws++;
  return 1;
}

void on_enter(GumInvocationContext *ic) {
  Invocation *call = GUM_IC_GET_INVOCATION_DATA(ic, Invocation);
  call->replaced = 0;
  call->ubo_replaced = 0;
  call->fence_block = -1;
  Q now = now_ms();
  if (now - control.heartbeat > 4000) control.enabled = 0;
  if (!control.enabled) {
    if (!control.cleaned) {
      if (control.pose_direct) restore_direct_poses();
      int j;
      for (j = 0; j < 32; j++) if (control.pose_fences[j]) {
        glDeleteSync(control.pose_fences[j]); control.pose_fences[j] = 0;
      }
      if (control.pose_buffer) { glDeleteBuffers(1, &control.pose_buffer); control.pose_buffer = 0; }
      release_resources(); control.cleaned = 1;
    }
    return;
  }
  U program = integer(0x8b8d);
  int location = -1;
  U i;
  for (i = 0; i < control.cache_count; i++)
    if (control.cache[i].program == program) { location = control.cache[i].location; break; }
  if (i == control.cache_count) {
    location = glGetUniformLocation(program, "PassthroughColorImage");
    if (control.cache_count < 64) {
      control.cache[i].program = program;
      control.cache[i].location = location;
      control.cache_count++;
    }
  }
  if (location < 0) return;
  if (control.tracker && (i >= control.cache_count || !control.cache[i].pose_checked)) {
    if (!validate_pose_layout(program)) { fail(); return; }
    if (i < control.cache_count) control.cache[i].pose_checked = 1;
  }
  int viewport[4], scissor[4], eye;
  glGetIntegerv(0x0ba2, viewport);
  glGetIntegerv(0x0c10, scissor);
  int middle = viewport[0] + viewport[2] / 2;
  if (!glIsEnabled(0x0c11) || scissor[2] <= 0 || scissor[2] > viewport[2] / 2) return;
  if (scissor[0] + scissor[2] <= middle) eye = 0;
  else if (scissor[0] >= middle) eye = 1;
  else return;
  if (control.swap_eyes) eye = 1 - eye;
  if (now - control.last_refresh >= 12) { refresh(); control.last_refresh = now; }
  Source *source = &control.sources[eye];
  if (!control.enabled || !source->valid) return;
  if (control.tracker && !(control.pose_direct ? write_ir_pose(source, call) : bind_ir_pose(source, call))) return;
  glGetUniformiv(program, location, &call->unit);
  call->active = integer(0x84e0);
  glActiveTexture(0x84c0 + call->unit);
  call->texture = integer(0x8d67);
  glBindTexture(0x8d65, source->yuv_texture);
  glActiveTexture(call->active);
  call->replaced = 1;
  control.draws++;
  control.last_substitution = now;
}

void on_leave(GumInvocationContext *ic) {
  Invocation *call = GUM_IC_GET_INVOCATION_DATA(ic, Invocation);
  if (call->ubo_replaced) {
    if (call->fence_block >= 0) control.pose_fences[call->fence_block] = glFenceSync(0x9117, 0);
    glBindBufferRange(0x8a11, 0, call->ubo, call->ubo_start, call->ubo_size);
    glBindBuffer(0x8a11, call->generic_ubo);
  }
  if (call->replaced) {
    glActiveTexture(0x84c0 + call->unit);
    glBindTexture(0x8d65, call->texture);
    glActiveTexture(call->active);
  }
}
