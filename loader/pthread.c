/*
 * pthread.c -- fake pthread impl.
 *
 * Copyright (C) 2021 Electry <dev@electry.sk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <vitasdk.h>

#include <malloc.h>

#include "main.h"
#include "pthread.h"

int syscall_fake(int id, ...) {
  switch (id) {
    case __NR_gettid:
      return sceKernelGetThreadId();
    case __NR_sched_setaffinity:
      return 0;
    default:
      return 0;
  }
}

int pthread_thread_entry_fake(SceSize args, uintptr_t *argp) {
  void *(*entry)(void *arg) = (void *)argp[0];
  void *entry_arg = (void *)argp[1];

  entry(entry_arg);

  return sceKernelExitThread(0);
}

int pthread_create_fake(SceUID *thread, void *attr, void *(*entry)(void *arg), void *arg) {
  const char *name = (const char *)((uintptr_t)arg + 5);

  char *sce_name = "thread";
  int stack_size = DEFAULT_STACK_SIZE;
  int priority = SCE_KERNEL_PROCESS_PRIORITY_USER_DEFAULT; // 64 - 191
  int affinity = SCE_KERNEL_CPU_MASK_USER_ALL;

  if (name[0] == '\0') {
    sce_name = "pthread_thread";
    priority = 96;
    affinity = SCE_KERNEL_CPU_MASK_USER_1;
  } else if (!strcmp(name, "FMOD thread for FMOD_NONBLOCKING")) {
    sce_name = "fmod_thread";
    priority = 96;
    affinity = SCE_KERNEL_CPU_MASK_USER_2;
  } else if (!strcmp(name, "FMOD file thread")) {
    sce_name = "fmod_file_thread";
    priority = 96;
    affinity = SCE_KERNEL_CPU_MASK_USER_1;
  } else if (!strcmp(name, "FMOD mixer thread")) {
    sce_name = "fmod_mixer_thread";
    priority = 64;
    affinity = SCE_KERNEL_CPU_MASK_USER_2;
  } else if (!strcmp(name, "FMOD stream thread")) {
    sce_name = "fmod_stream_thread";
    priority = 80;
    affinity = SCE_KERNEL_CPU_MASK_USER_2;
  }

  SceUID thid = sceKernelCreateThread(
    sce_name,
    (SceKernelThreadEntry)pthread_thread_entry_fake,
    priority,
    stack_size,
    0,
    affinity,
    NULL
  );

  uintptr_t args[] = { (uintptr_t)entry, (uintptr_t)arg };

  int ret = sceKernelStartThread(thid, sizeof(args), args);
  if (ret < 0) {
    sceKernelDeleteThread(thid);
    return -1;
  }

  *thread = thid;
  return 0;
}

int pthread_detach_fake(SceUID thread) {
  sceKernelWaitThreadEnd(thread, NULL, NULL);
  return sceKernelDeleteThread(thread);
}

int pthread_join_fake(SceUID thread, void **retval) {
  sceKernelWaitThreadEnd(thread, (int *)retval, NULL);
  return sceKernelDeleteThread(thread);
}

int pthread_self_fake(void) {
  return sceKernelGetThreadId();
}

int pthread_getattr_np_fake(SceUID thread, void *attr) {
  return 1; // fail
}

int pthread_key_create_fake(void **key, void (*destructor)(void*)) {
	sceKernelGetRandomNumber(key, 4);
  return 0;
}

int pthread_key_delete_fake(void *key) {
  return 0;
}

void *pthread_getspecific_fake(void *key) {
  return NULL;
}

int pthread_mutex_init_fake(SceKernelLwMutexWork **work) {
  *work = (SceKernelLwMutexWork *)memalign(8, sizeof(SceKernelLwMutexWork));
  if (sceKernelCreateLwMutex(*work, "mutex", SCE_KERNEL_MUTEX_ATTR_RECURSIVE, 0, NULL) < 0)
    return -1;
  return 0;
}

int pthread_mutex_destroy_fake(SceKernelLwMutexWork **work) {
  if (sceKernelDeleteLwMutex(*work) < 0)
    return -1;
  free(*work);
  return 0;
}

int pthread_mutex_lock_fake(SceKernelLwMutexWork **work) {
  if (!*work)
    pthread_mutex_init_fake(work);
  if (sceKernelLockLwMutex(*work, 1, NULL) < 0)
    return -1;
  return 0;
}

int pthread_mutex_unlock_fake(SceKernelLwMutexWork **work) {
  if (sceKernelUnlockLwMutex(*work, 1) < 0)
    return -1;
  return 0;
}

int pthread_mutex_trylock_fake(SceKernelLwMutexWork **work) {
  if (sceKernelTryLockLwMutex(*work, 1) < 0)
    return -1;
  return 0;
}

// dunno if conds are implemented properly but ¯\_(ツ)_/¯
int pthread_cond_init_fake(cond_t **cond, const void *condattr) {
  *cond = (cond_t *)memalign(8, sizeof(cond_t));

  int ret = sceKernelCreateLwMutex(&(*cond)->mutex_work, "mutex", SCE_KERNEL_MUTEX_ATTR_RECURSIVE, 0, NULL);
  if (ret < 0)
    return -1;

  if (sceKernelCreateLwCond(&(*cond)->cond_work, "cond", 0, &(*cond)->mutex_work, NULL) < 0)
    return -1;
  return 0;
}

int pthread_cond_destroy_fake(cond_t **cond) {
  if (sceKernelDeleteLwCond(&(*cond)->cond_work) < 0)
    return -1;
  if (sceKernelDeleteLwMutex(&(*cond)->mutex_work) < 0)
    return -1;

  free(*cond);
  return 0;
}

int pthread_cond_broadcast_fake(cond_t **cond) {
  if (sceKernelSignalLwCond(&(*cond)->cond_work) < 0)
    return -1;
  return 0;
}

int pthread_cond_signal_fake(cond_t **cond) {
  if (sceKernelSignalLwCond(&(*cond)->cond_work) < 0)
    return -1;
  return 0;
}

int pthread_cond_wait_fake(cond_t **cond, SceKernelLwMutexWork **mutex) {
 if (sceKernelWaitLwCond(&(*cond)->cond_work, NULL) < 0)
    return -1;
  return 0;
}

int pthread_cond_timedwait_fake(cond_t **cond, SceKernelLwMutexWork **mutex, const struct timespec *abstime) {
  SceUInt timeout = (abstime->tv_sec * 1000 * 1000 + abstime->tv_nsec / 1000);
  if (sceKernelWaitLwCond(&(*cond)->cond_work, &timeout) < 0)
    return -1;
  return 0;
}

int pthread_attr_init_fake(void *attr) {
  return 0;
}

int pthread_attr_destroy_fake(void *attr) {
  return 0;
}

int pthread_attr_setschedpolicy_fake(void *attr, int policy) {
  return 0; // return 0 or else the thread never gets created
}

int pthread_attr_setschedparam_fake(void *attr, const void *param) {
  return 0;
}

int pthread_attr_setstacksize_fake(void *attr, size_t stacksize) {
  return 0;
}

int pthread_attr_setdetachstate_fake(void *attr, int detachstate) {
  return 0;
}

int pthread_attr_setstack_fake(void *attr, void *stackaddr, size_t stacksize) {
  return 0;
}

int pthread_attr_getstack_fake(const void *attr, void **stackaddr, size_t *stacksize) {
  *stackaddr = NULL;
  *stacksize = 0;
  return 1; // fail
}

int pthread_mutexattr_init_fake(void *attr) {
  return 0;
}

int pthread_mutexattr_destroy_fake(void *attr) {
  return 0;
}

int pthread_mutexattr_setpshared_fake(void *attr, int pshared) {
  return 0;
}

int pthread_mutexattr_settype_fake(void *attr, int type) {
  return 0;
}
