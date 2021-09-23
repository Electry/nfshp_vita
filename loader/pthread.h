#ifndef __PTHREAD_H__
#define __PTHREAD_H__
#include <vitasdk.h>

#define __NR_gettid 224
#define __NR_sched_setaffinity 241

typedef struct {
  SceKernelLwCondWork cond_work;
  SceKernelLwMutexWork mutex_work;
} cond_t;

int syscall_fake(int id, ...);

int pthread_create_fake(SceUID *thread, void *attr, void *(*entry)(void *arg), void *arg);
int pthread_detach_fake(SceUID thread);
int pthread_join_fake(SceUID thread, void **retval);
int pthread_self_fake(void);

int pthread_getattr_np_fake(SceUID thread, void *attr);

int pthread_key_create_fake(void **key, void (*destructor)(void*));
int pthread_key_delete_fake(void *key);
void *pthread_getspecific_fake(void *key);

int pthread_mutex_init_fake(SceKernelLwMutexWork **work);
int pthread_mutex_destroy_fake(SceKernelLwMutexWork **work);
int pthread_mutex_lock_fake(SceKernelLwMutexWork **work);
int pthread_mutex_unlock_fake(SceKernelLwMutexWork **work);
int pthread_mutex_trylock_fake(SceKernelLwMutexWork **work);

int pthread_cond_init_fake(cond_t **cond, const void *condattr);
int pthread_cond_destroy_fake(cond_t **cond);
int pthread_cond_broadcast_fake(cond_t **cond);
int pthread_cond_signal_fake(cond_t **cond);
int pthread_cond_wait_fake(cond_t **cond, SceKernelLwMutexWork **mutex);
int pthread_cond_timedwait_fake(cond_t **cond, SceKernelLwMutexWork **mutex, const struct timespec *abstime);

int pthread_attr_init_fake(void *attr);
int pthread_attr_destroy_fake(void *attr);
int pthread_attr_setschedpolicy_fake(void *attr, int policy);
int pthread_attr_setschedparam_fake(void *attr, const void *param);
int pthread_attr_setstacksize_fake(void *attr, size_t stacksize);
int pthread_attr_setdetachstate_fake(void *attr, int detachstate);
int pthread_attr_setstack_fake(void *attr, void *stackaddr, size_t stacksize);
int pthread_attr_getstack_fake(const void *attr, void **stackaddr, size_t *stacksize);

int pthread_mutexattr_init_fake(void *attr);
int pthread_mutexattr_destroy_fake(void *attr);
int pthread_mutexattr_setpshared_fake(void *attr, int pshared);
int pthread_mutexattr_settype_fake(void *attr, int type);

#endif
