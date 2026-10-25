/* Observe new compositor contexts. All mutable storage is supplied by JS. */
#include <gum/guminterceptor.h>
typedef unsigned int U;
extern void *eglGetCurrentContext(void);
extern void glGetIntegerv(U, int *);
extern int glGetUniformLocation(U, const char *);
extern int pthread_mutex_lock(void *), pthread_mutex_trylock(void *), pthread_mutex_unlock(void *);
extern int context_discovered(void *);
extern unsigned char registry_lock[];
extern struct { U enabled; void *contexts[8]; } registry;

void observe(GumInvocationContext *ic) {
  void *context = eglGetCurrentContext();
  int i, free_slot = -1, program = 0;
  if (!context || pthread_mutex_trylock(registry_lock) != 0) return;
  if (!registry.enabled) { pthread_mutex_unlock(registry_lock); return; }
  for (i = 0; i < 8; i++) {
    if (registry.contexts[i] == context) { pthread_mutex_unlock(registry_lock); return; }
    if (!registry.contexts[i]) free_slot = i;
  }
  glGetIntegerv(0x8b8d, &program);
  if (!program || glGetUniformLocation(program, "PassthroughColorImage") < 0) {
    pthread_mutex_unlock(registry_lock); return;
  }
  if (free_slot >= 0) registry.contexts[free_slot] = context;
  pthread_mutex_unlock(registry_lock);
  /* JS serializes creation and reports the bounded-context limit as an error. */
  context_discovered(context);
}

void forget(void *context) {
  int i;
  pthread_mutex_lock(registry_lock);
  for (i = 0; i < 8; i++) if (registry.contexts[i] == context) registry.contexts[i] = 0;
  pthread_mutex_unlock(registry_lock);
}

void disable(void) {
  pthread_mutex_lock(registry_lock);
  registry.enabled = 0;
  pthread_mutex_unlock(registry_lock);
}
