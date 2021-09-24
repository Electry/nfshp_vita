/*
 * dynlib.c -- .so import resolution
 *
 * Copyright (C) 2021 Andy Nguyen
 *               2021 Electry <dev@electry.sk>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <errno.h>
#include <setjmp.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <PVR_PSP2/GLES/gl.h>
#include <PVR_PSP2/GLES/glext.h>

#include "main.h"
#include "pthread.h"
#include "gl_patch.h"
#include "dynlib.h"

int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
#ifdef DEBUG
  va_list list;
  static char string[0x1000];

  va_start(list, fmt);
  vsprintf(string, fmt, list);
  va_end(list);

  debugPrintf("%s: %s\n", tag, string);
#endif
  return 0;
}

int __android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap) {
#ifdef DEBUG
  static char string[0x1000];
  vsprintf(string, fmt, ap);
  debugPrintf("%s: %s\n", tag, string);
#endif
  return 0;
}

int __android_log_write(int prio, const char *tag, const char *text) {
#ifdef DEBUG
  debugPrintf("%s: %s\n", tag, text);
#endif
  return 0;
}

int sem_init_fake(int *uid, int pshared, unsigned int value) {
  *uid = sceKernelCreateSema("sema", 0, value, 0x7fffffff, NULL);
  if (*uid < 0)
    return -1;
  return 0;
}

int sem_post_fake(int *uid) {
  if (sceKernelSignalSema(*uid, 1) < 0)
    return -1;
  return 0;
}

int sem_wait_fake(int *uid) {
  if (sceKernelWaitSema(*uid, 1, NULL) < 0)
    return -1;
  return 0;
}

int sem_trywait_fake(int *uid) {
  SceUInt timeout = 0;
  if (sceKernelWaitSema(*uid, 1, &timeout) < 0)
    return -1;
  return 0;
}

int sem_timedwait_fake(int *uid, const struct timespec *abs_timeout) {
  SceUInt timeout = (abs_timeout->tv_sec * 1000 * 1000 + abs_timeout->tv_nsec / 1000);
  if (sceKernelWaitSema(*uid, 1, &timeout) < 0)
    return -1;
  return 0;
}

int sem_destroy_fake(int *uid) {
  if (sceKernelDeleteSema(*uid) < 0)
    return -1;
  return 0;
}

int sem_getvalue_fake(int *uid, int *sval) {
  SceKernelSemaInfo info;
  if (sceKernelGetSemaInfo(*uid, &info) < 0)
    return -1;
	*sval = info.numWaitThreads > 0 ? -info.numWaitThreads : info.currentCount;
  return 0;
}

void *mmap_fake(void *addr, size_t len, int prot, int flags, int fd, size_t offset) {
  return malloc(len);
}

int munmap_fake(void *addr, size_t len) {
  free(addr);
  return 0;
}

int stat_hook(const char *pathname, stat64_t *statbuf) {
  struct stat statbuf_fake;
  memset(&statbuf_fake, 0, sizeof(statbuf_fake));

  int res = stat(pathname, &statbuf_fake);
  statbuf->st_dev = statbuf_fake.st_dev;
  statbuf->st_mode = statbuf_fake.st_mode;
  statbuf->st_size = statbuf_fake.st_size;

  return res;
}

int __isinf(double d) {
  return isinf(d);
}

// std::streambuf::seekpos(std::fpos<mbstate_t>,std::_Ios_Openmode)
uintptr_t *_ZNSt15basic_streambufIcSt11char_traitsIcEE7seekposESt4fposI9mbstate_tESt13_Ios_Openmode(uintptr_t *result, int a2, int a3, int a4, int a5, int a6) {
  result[0] = 0xffffffff;
  result[1] = 0xffffffff;
  result[2] = 0;
  return result;
}

int clock_gettime_fake(int clk_id, struct timespec *tp) {
  if (clk_id == CLOCK_MONOTONIC) {
    SceKernelSysClock ticks;
    sceKernelGetProcessTime(&ticks);

    tp->tv_sec = ticks / (1000 * 1000);
    tp->tv_nsec = (ticks * 1000) % (1000 * 1000 * 1000);

    return 0;
  } else if (clk_id == CLOCK_REALTIME) {
    time_t seconds;
    SceDateTime time;
    sceRtcGetCurrentClockLocalTime(&time);

    sceRtcGetTime_t(&time, &seconds);

    tp->tv_sec = seconds;
    tp->tv_nsec = time.microsecond * 1000;

    return 0;
  }

  return -ENOSYS;
}

int nanosleep_fake(const struct timespec *req, struct timespec *rem) {
  return usleep(req->tv_sec * 1000 * 1000 + req->tv_nsec / 1000);
}

static FILE __sF_fake[0x100][3];

static const short _C_tolower_[] = {
  -1,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  0x40, 'a',  'b',  'c',  'd',  'e',  'f',  'g',
  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
  'x',  'y',  'z',  0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
  0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
  0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
  0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
  0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

static const short _C_toupper_[] = {
  -1,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  0x40, 'A',  'B',  'C',  'D',  'E',  'F',  'G',
  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
  'X',  'Y',  'Z',  0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
  0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
  0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
  0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
  0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

static const short *_tolower_tab_ = _C_tolower_;
static const short *_toupper_tab_ = _C_toupper_;

extern void *__cxa_guard_acquire;
extern void *__cxa_guard_release;

extern void *__aeabi_atexit;

extern void *__cxa_allocate_exception;
extern void *__cxa_bad_cast;
extern void *__cxa_begin_catch;
extern void *__cxa_begin_cleanup;
extern void *__cxa_call_unexpected;
extern void *__cxa_end_catch;
extern void *__cxa_end_cleanup;
extern void *__cxa_finalize;
extern void *__cxa_free_exception;
extern void *__cxa_guard_abort;
extern void *__cxa_guard_acquire;
extern void *__cxa_guard_release;
extern void *__cxa_pure_virtual;
extern void *__cxa_rethrow;
extern void *__cxa_throw;
extern void *__cxa_type_match;

extern void *_ZNKSt11logic_error4whatEv;
extern void *_ZNKSt13runtime_error4whatEv;
extern void *_ZNKSt5ctypeIcE13_M_widen_initEv;
extern void *_ZNKSt9type_infoeqERKS_;
extern void *_ZNSo3putEc;
extern void *_ZNSo5flushEv;
extern void *_ZNSo9_M_insertIPKvEERSoT_;
extern void *_ZNSo9_M_insertIbEERSoT_;
extern void *_ZNSo9_M_insertIdEERSoT_;
extern void *_ZNSo9_M_insertIlEERSoT_;
extern void *_ZNSo9_M_insertImEERSoT_;
extern void *_ZNSo9_M_insertIyEERSoT_;
extern void *_ZNSolsEi;
extern void *_ZNSs4_Rep10_M_destroyERKSaIcE;
extern void *_ZNSs4_Rep20_S_empty_rep_storageE;
extern void *_ZNSs6appendEPKcj;
extern void *_ZNSs6appendERKSs;
extern void *_ZNSs6assignERKSs;
extern void *_ZNSsC1EPKcRKSaIcE;
extern void *_ZNSsC1ERKSs;
extern void *_ZNSt12length_errorC1ERKSs;
extern void *_ZNSt12length_errorD1Ev;
extern void *_ZNSt12length_errorD2Ev;
extern void *_ZNSt13runtime_errorC2ERKSs;
extern void *_ZNSt13runtime_errorD2Ev;
extern void *_ZNSt15basic_streambufIcSt11char_traitsIcEE4syncEv;
extern void *_ZNSt15basic_streambufIcSt11char_traitsIcEE5imbueERKSt6locale;
extern void *_ZNSt15basic_streambufIcSt11char_traitsIcEE5uflowEv;
extern void *_ZNSt15basic_streambufIcSt11char_traitsIcEE6setbufEPci;
extern void *_ZNSt15basic_streambufIcSt11char_traitsIcEE6xsgetnEPci;
extern void *_ZNSt15basic_streambufIcSt11char_traitsIcEE6xsputnEPKci;
extern void *_ZNSt15basic_streambufIcSt11char_traitsIcEE7seekoffExSt12_Ios_SeekdirSt13_Ios_Openmode;
extern void *_ZNSt15basic_streambufIcSt11char_traitsIcEE9pbackfailEi;
extern void *_ZNSt15basic_streambufIcSt11char_traitsIcEE9showmanycEv;
extern void *_ZNSt15basic_streambufIcSt11char_traitsIcEE9underflowEv;
extern void *_ZNSt6localeC1Ev;
extern void *_ZNSt6localeD1Ev;
extern void *_ZNSt8__detail15_List_node_base9_M_unhookEv;
extern void *_ZNSt8ios_base4InitC1Ev;
extern void *_ZNSt8ios_base4InitD1Ev;
extern void *_ZNSt8ios_baseC2Ev;
extern void *_ZNSt8ios_baseD2Ev;
extern void *_ZNSt9basic_iosIcSt11char_traitsIcEE4initEPSt15basic_streambufIcS1_E;
extern void *_ZNSt9basic_iosIcSt11char_traitsIcEE5clearESt12_Ios_Iostate;
extern void *_ZNSt9exceptionD2Ev;
extern void *_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_i;
extern void *_ZSt16__throw_bad_castv;
extern void *_ZSt17__throw_bad_allocv;
extern void *_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_;
extern void *_ZSt9terminatev;
extern void *_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc;
extern void *_ZTISt12length_error;
extern void *_ZTISt13runtime_error;
extern void *_ZTISt15basic_streambufIcSt11char_traitsIcEE;
extern void *_ZTISt9exception;
extern void *_ZTVN10__cxxabiv117__class_type_infoE;
extern void *_ZTVN10__cxxabiv119__pointer_type_infoE;
extern void *_ZTVN10__cxxabiv120__function_type_infoE;
extern void *_ZTVN10__cxxabiv120__si_class_type_infoE;
extern void *_ZTVN10__cxxabiv121__vmi_class_type_infoE;
extern void *_ZTVSo;
extern void *_ZTVSt11logic_error;
extern void *_ZTVSt12length_error;
extern void *_ZTVSt13runtime_error;
extern void *_ZTVSt15basic_streambufIcSt11char_traitsIcEE;
extern void *_ZTVSt9basic_iosIcSt11char_traitsIcEE;

extern void *__dynamic_cast;

uint32_t __page_size_fake = 4096; // mmap alignment

so_default_dynlib default_dynlib[] = {
  { "__aeabi_atexit", (uintptr_t)&__aeabi_atexit },

  { "__android_log_print", (uintptr_t)&__android_log_print },
  { "__android_log_vprint", (uintptr_t)&__android_log_vprint },
  { "__android_log_write", (uintptr_t)&__android_log_write },

  { "__cxa_allocate_exception", (uintptr_t)&__cxa_allocate_exception },
  { "__cxa_bad_cast", (uintptr_t)&__cxa_bad_cast },
  { "__cxa_begin_catch", (uintptr_t)&__cxa_begin_catch },
  { "__cxa_begin_cleanup", (uintptr_t)&__cxa_begin_cleanup },
  { "__cxa_call_unexpected", (uintptr_t)&__cxa_call_unexpected },
  { "__cxa_end_catch", (uintptr_t)&__cxa_end_catch },
  { "__cxa_end_cleanup", (uintptr_t)&__cxa_end_cleanup },
  { "__cxa_finalize", (uintptr_t)&__cxa_finalize },
  { "__cxa_free_exception", (uintptr_t)&__cxa_free_exception },
  { "__cxa_guard_abort", (uintptr_t)&__cxa_guard_abort },
  { "__cxa_guard_acquire", (uintptr_t)&__cxa_guard_acquire },
  { "__cxa_guard_release", (uintptr_t)&__cxa_guard_release },
  { "__cxa_pure_virtual", (uintptr_t)&__cxa_pure_virtual },
  { "__cxa_rethrow", (uintptr_t)&__cxa_rethrow },
  { "__cxa_throw", (uintptr_t)&__cxa_throw },
  { "__cxa_type_match", (uintptr_t)&__cxa_type_match },

  { "__dynamic_cast", (uintptr_t)&__dynamic_cast },
  { "__errno", (uintptr_t)&__errno },

  // { "__gnu_Unwind_Find_exidx", (uintptr_t)&__gnu_Unwind_Find_exidx },
  // { "__gxx_personality_v0", (uintptr_t)&__gxx_personality_v0 },

  { "__isinf", (uintptr_t)&__isinf },
  { "__page_size", (uintptr_t)&__page_size_fake },
  { "__sF", (uintptr_t)&__sF_fake },
  { "_tolower_tab_", (uintptr_t)&_tolower_tab_ },
  { "_toupper_tab_", (uintptr_t)&_toupper_tab_ },

  { "pthread_attr_destroy", (uintptr_t)&pthread_attr_destroy_fake },
  { "pthread_attr_getstack", (uintptr_t)&pthread_attr_getstack_fake },
  { "pthread_attr_init", (uintptr_t)&pthread_attr_init_fake },
  { "pthread_attr_setdetachstate", (uintptr_t)&pthread_attr_setdetachstate_fake },
  { "pthread_attr_setschedparam", (uintptr_t)&pthread_attr_setschedparam_fake },
  { "pthread_attr_setschedpolicy", (uintptr_t)&pthread_attr_setschedpolicy_fake },
  { "pthread_attr_setstack", (uintptr_t)&pthread_attr_setstack_fake },
  { "pthread_attr_setstacksize", (uintptr_t)&pthread_attr_setstacksize_fake },

  { "pthread_cond_broadcast", (uintptr_t)&pthread_cond_broadcast_fake },
  { "pthread_cond_destroy", (uintptr_t)&pthread_cond_destroy_fake },
  { "pthread_cond_init", (uintptr_t)&pthread_cond_init_fake },
  { "pthread_cond_signal", (uintptr_t)&pthread_cond_signal_fake },
  { "pthread_cond_timedwait", (uintptr_t)&pthread_cond_timedwait_fake },
  { "pthread_cond_wait", (uintptr_t)&pthread_cond_wait_fake },

  { "pthread_create", (uintptr_t)&pthread_create_fake },
  { "pthread_detach", (uintptr_t)&pthread_detach_fake },
  { "pthread_getattr_np", (uintptr_t)&pthread_getattr_np_fake },
  { "pthread_getspecific", (uintptr_t)&pthread_getspecific_fake },
  { "pthread_join", (uintptr_t)&pthread_join_fake },
  { "pthread_key_create", (uintptr_t)&pthread_key_create_fake },
  { "pthread_key_delete", (uintptr_t)&pthread_key_delete_fake },
  { "pthread_self", (uintptr_t)&pthread_self_fake },

  { "pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy_fake },
  { "pthread_mutex_init", (uintptr_t)&pthread_mutex_init_fake },
  { "pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_fake },
  { "pthread_mutex_trylock", (uintptr_t)&pthread_mutex_trylock_fake },
  { "pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_fake },

  { "pthread_mutexattr_destroy", (uintptr_t)&pthread_mutexattr_destroy_fake },
  { "pthread_mutexattr_init", (uintptr_t)&pthread_mutexattr_init_fake },
  { "pthread_mutexattr_setpshared", (uintptr_t)&pthread_mutexattr_setpshared_fake },
  { "pthread_mutexattr_settype", (uintptr_t)&pthread_mutexattr_settype_fake },

  { "sem_destroy", (uintptr_t)&sem_destroy_fake },
  { "sem_getvalue", (uintptr_t)&sem_getvalue_fake },
  { "sem_init", (uintptr_t)&sem_init_fake },
  { "sem_post", (uintptr_t)&sem_post_fake },
  { "sem_timedwait", (uintptr_t)&sem_timedwait_fake },
  { "sem_trywait", (uintptr_t)&sem_trywait_fake },
  { "sem_wait", (uintptr_t)&sem_wait_fake },

  { "acos", (uintptr_t)&acos },
  { "acosf", (uintptr_t)&acosf },
  { "asinf", (uintptr_t)&asinf },
  { "atan2", (uintptr_t)&atan2 },
  { "atan2f", (uintptr_t)&atan2f },
  { "ceil", (uintptr_t)&ceil },
  { "ceilf", (uintptr_t)&ceilf },
  { "cos", (uintptr_t)&cos },
  { "cosf", (uintptr_t)&cosf },
  { "exp", (uintptr_t)&exp },
  { "expf", (uintptr_t)&expf },
  { "floor", (uintptr_t)&floor },
  { "floorf", (uintptr_t)&floorf },
  { "fmod", (uintptr_t)&fmod },
  { "fmodf", (uintptr_t)&fmodf },
  { "log", (uintptr_t)&log },
  { "log10", (uintptr_t)&log10 },
  { "logf", (uintptr_t)&logf },
  { "modf", (uintptr_t)&modf },
  { "pow", (uintptr_t)&pow },
  { "powf", (uintptr_t)&powf },
  { "qsort", (uintptr_t)&qsort },
  { "roundf", (uintptr_t)&roundf },
  { "sin", (uintptr_t)&sin },
  { "sinf", (uintptr_t)&sinf },
  { "sqrt", (uintptr_t)&sqrt },
  { "sqrtf", (uintptr_t)&sqrtf },
  { "tanf", (uintptr_t)&tanf },

  { "atoi", (uintptr_t)&atoi },

  { "free", (uintptr_t)&free },
  { "malloc", (uintptr_t)&malloc },
  { "realloc", (uintptr_t)&realloc },
  { "mmap", (uintptr_t)&mmap_fake },
  { "munmap", (uintptr_t)&munmap_fake },

  // { "clock", (uintptr_t)&clock },
  { "clock_gettime", (uintptr_t)&clock_gettime_fake },
  { "gettimeofday", (uintptr_t)&gettimeofday },
  { "localtime", (uintptr_t)&localtime },
  { "time", (uintptr_t)&time },

  { "nanosleep", (uintptr_t)&nanosleep_fake },
  { "usleep", (uintptr_t)&usleep },

  { "abort", (uintptr_t)&abort },
  { "syscall", (uintptr_t)&syscall_fake },

  // { "close", (uintptr_t)&close },
  // { "closedir", (uintptr_t)&closedir },
  // { "lseek", (uintptr_t)&lseek },
  { "mkdir", (uintptr_t)&mkdir },
  // { "open", (uintptr_t)&open },
  // { "opendir", (uintptr_t)&opendir },
  // { "read", (uintptr_t)&read },
  // { "readdir", (uintptr_t)&readdir },
  { "stat", (uintptr_t)&stat_hook },
  // { "write", (uintptr_t)&write },

  { "fclose", (uintptr_t)&sceLibcBridge_fclose },
  // { "fcntl", (uintptr_t)&fcntl },
  { "fflush", (uintptr_t)&sceLibcBridge_fflush },
  { "fopen", (uintptr_t)&sceLibcBridge_fopen },
  { "fputs", (uintptr_t)&sceLibcBridge_fputs },
  { "fread", (uintptr_t)&sceLibcBridge_fread },
  { "fseek", (uintptr_t)&sceLibcBridge_fseek },
  // { "fsync", (uintptr_t)&fsync },
  { "ftell", (uintptr_t)&sceLibcBridge_ftell },
  // { "ftruncate", (uintptr_t)&ftruncate },
  { "fwrite", (uintptr_t)&sceLibcBridge_fwrite },

  { "getenv", (uintptr_t)&getenv },

  { "longjmp", (uintptr_t)&longjmp },
  { "setjmp", (uintptr_t)&setjmp },

  { "memchr", (uintptr_t)&memchr },
  { "memcmp", (uintptr_t)&memcmp },
  { "memcpy", (uintptr_t)&memcpy },
  { "memmove", (uintptr_t)&memmove },
  { "memset", (uintptr_t)&memset },

  // { "puts", (uintptr_t)&puts },
  // { "putchar", (uintptr_t)&putchar },
  { "qsort", (uintptr_t)&qsort },

  { "rand", (uintptr_t)&rand },
  { "srand", (uintptr_t)&srand },

  { "prctl", (uintptr_t)&ret0 },

  { "srand48", (uintptr_t)&srand48 },
  { "lrand48", (uintptr_t)&lrand48 },

  { "strcat", (uintptr_t)&strcat },
  { "strchr", (uintptr_t)&strchr },
  { "strcmp", (uintptr_t)&strcmp },
  { "strcpy", (uintptr_t)&strcpy },
  { "strdup", (uintptr_t)&strdup },
  { "strlen", (uintptr_t)&strlen },
  { "strncmp", (uintptr_t)&strncmp },
  { "strncpy", (uintptr_t)&strncpy },
  { "strrchr", (uintptr_t)&strrchr },
  { "strstr", (uintptr_t)&strstr },
  { "strtod", (uintptr_t)&strtod },
  { "strtol", (uintptr_t)&strtol },

  { "sprintf", (uintptr_t)&sprintf },
  { "snprintf", (uintptr_t)&snprintf },

  { "glActiveTexture", (uintptr_t)&glActiveTexture },
  { "glAlphaFunc", (uintptr_t)&glAlphaFunc_wrapper },
  { "glAlphaFuncx", (uintptr_t)&glAlphaFuncx },
  { "glBindBuffer", (uintptr_t)&glBindBuffer },
  { "glBindFramebufferOES", (uintptr_t)&glBindFramebufferOES },
  { "glBindRenderbufferOES", (uintptr_t)&glBindRenderbufferOES },
  { "glBindTexture", (uintptr_t)&glBindTexture },
  { "glBlendEquationOES", (uintptr_t)&glBlendEquationOES },
  { "glBlendEquationSeparateOES", (uintptr_t)&glBlendEquationSeparateOES },
  { "glBlendFunc", (uintptr_t)&glBlendFunc },
  { "glBlendFuncSeparateOES", (uintptr_t)&glBlendFuncSeparateOES },
  { "glBufferData", (uintptr_t)&glBufferData },
  { "glBufferSubData", (uintptr_t)&glBufferSubData },
  { "glCheckFramebufferStatusOES", (uintptr_t)&glCheckFramebufferStatusOES },
  { "glClear", (uintptr_t)&glClear },
  { "glClearColor", (uintptr_t)&glClearColor_wrapper },
  { "glClearColorx", (uintptr_t)&glClearColorx },
  { "glClearDepthf", (uintptr_t)&glClearDepthf_wrapper },
  { "glClearDepthx", (uintptr_t)&glClearDepthx },
  { "glClearStencil", (uintptr_t)&glClearStencil },
  { "glClientActiveTexture", (uintptr_t)&glClientActiveTexture },
  { "glClipPlanef", (uintptr_t)&glClipPlanef_wrapper },
  { "glClipPlanex", (uintptr_t)&glClipPlanex },
  { "glColor4f", (uintptr_t)&glColor4f_wrapper },
  { "glColor4ub", (uintptr_t)&glColor4ub },
  { "glColor4x", (uintptr_t)&glColor4x },
  { "glColorMask", (uintptr_t)&glColorMask },
  { "glColorPointer", (uintptr_t)&glColorPointer },
  { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
  { "glCompressedTexSubImage2D", (uintptr_t)&glCompressedTexSubImage2D },
  { "glCopyTexImage2D", (uintptr_t)&glCopyTexImage2D },
  { "glCopyTexSubImage2D", (uintptr_t)&glCopyTexSubImage2D },
  { "glCullFace", (uintptr_t)&glCullFace },
  { "glCurrentPaletteMatrixOES", (uintptr_t)&glCurrentPaletteMatrixOES },
  { "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
  { "glDeleteFramebuffersOES", (uintptr_t)&glDeleteFramebuffersOES },
  { "glDeleteRenderbuffersOES", (uintptr_t)&glDeleteRenderbuffersOES },
  { "glDeleteTextures", (uintptr_t)&glDeleteTextures },
  { "glDepthFunc", (uintptr_t)&glDepthFunc },
  { "glDepthMask", (uintptr_t)&glDepthMask },
  { "glDepthRangef", (uintptr_t)&glDepthRangef_wrapper },
  { "glDepthRangex", (uintptr_t)&glDepthRangex },
  { "glDisable", (uintptr_t)&glDisable },
  { "glDisableClientState", (uintptr_t)&glDisableClientState },
  { "glDrawArrays", (uintptr_t)&glDrawArrays },
  { "glDrawElements", (uintptr_t)&glDrawElements },
  { "glDrawTexfOES", (uintptr_t)&glDrawTexfOES_wrapper },
  { "glDrawTexfvOES", (uintptr_t)&glDrawTexfvOES },
  { "glDrawTexiOES", (uintptr_t)&glDrawTexiOES},
  { "glDrawTexivOES", (uintptr_t)&glDrawTexivOES},
  { "glDrawTexsOES", (uintptr_t)&glDrawTexsOES},
  { "glDrawTexsvOES", (uintptr_t)&glDrawTexsvOES},
  { "glDrawTexxOES", (uintptr_t)&glDrawTexxOES},
  { "glDrawTexxvOES", (uintptr_t)&glDrawTexxvOES},
  { "glEGLImageTargetRenderbufferStorageOES", (uintptr_t)&glEGLImageTargetRenderbufferStorageOES},
  { "glEGLImageTargetTexture2DOES", (uintptr_t)&glEGLImageTargetTexture2DOES},
  { "glEnable", (uintptr_t)&glEnable },
  { "glEnableClientState", (uintptr_t)&glEnableClientState },
  { "glFinish", (uintptr_t)&glFinish },
  { "glFlush", (uintptr_t)&glFlush },
  { "glFogf", (uintptr_t)&glFogf_wrapper },
  { "glFogfv", (uintptr_t)&glFogfv },
  { "glFogx", (uintptr_t)&glFogx },
  { "glFogxv", (uintptr_t)&glFogxv },
  { "glFramebufferRenderbufferOES", (uintptr_t)&glFramebufferRenderbufferOES },
  { "glFramebufferTexture2DOES", (uintptr_t)&glFramebufferTexture2DOES },
  { "glFrontFace", (uintptr_t)&glFrontFace },
  { "glFrustumf", (uintptr_t)&glFrustumf_wrapper },
  { "glFrustumx", (uintptr_t)&glFrustumx },
  { "glGenBuffers", (uintptr_t)&glGenBuffers },
  { "glGenFramebuffersOES", (uintptr_t)&glGenFramebuffersOES },
  { "glGenRenderbuffersOES", (uintptr_t)&glGenRenderbuffersOES },
  { "glGenTextures", (uintptr_t)&glGenTextures },
  { "glGenerateMipmapOES", (uintptr_t)&glGenerateMipmapOES },
  { "glGetBooleanv", (uintptr_t)&glGetBooleanv },
  { "glGetBufferParameteriv", (uintptr_t)&glGetBufferParameteriv },
  { "glGetBufferPointervOES", (uintptr_t)&glGetBufferPointervOES },
  { "glGetClipPlanef", (uintptr_t)&glGetClipPlanef },
  { "glGetClipPlanex", (uintptr_t)&glGetClipPlanex },
  { "glGetError", (uintptr_t)&glGetError },
  { "glGetFixedv", (uintptr_t)&glGetFixedv },
  { "glGetFloatv", (uintptr_t)&glGetFloatv },
  { "glGetFramebufferAttachmentParameterivOES", (uintptr_t)&glGetFramebufferAttachmentParameterivOES },
  { "glGetIntegerv", (uintptr_t)&glGetIntegerv },
  { "glGetLightfv", (uintptr_t)&glGetLightfv },
  { "glGetLightxv", (uintptr_t)&glGetLightxv },
  { "glGetMaterialfv", (uintptr_t)&glGetMaterialfv },
  { "glGetMaterialxv", (uintptr_t)&glGetMaterialxv },
  { "glGetPointerv", (uintptr_t)&glGetPointerv },
  { "glGetRenderbufferParameterivOES", (uintptr_t)&glGetRenderbufferParameterivOES },
  { "glGetString", (uintptr_t)&glGetString },
  { "glGetTexEnvfv", (uintptr_t)&glGetTexEnvfv },
  { "glGetTexEnviv", (uintptr_t)&glGetTexEnviv },
  { "glGetTexEnvxv", (uintptr_t)&glGetTexEnvxv },
  { "glGetTexGenfvOES", (uintptr_t)&glGetTexGenfvOES },
  { "glGetTexGenivOES", (uintptr_t)&glGetTexGenivOES },
  { "glGetTexGenxvOES", (uintptr_t)&glGetTexGenxvOES },
  { "glGetTexParameterfv", (uintptr_t)&glGetTexParameterfv },
  { "glGetTexParameteriv", (uintptr_t)&glGetTexParameteriv },
  { "glGetTexParameterxv", (uintptr_t)&glGetTexParameterxv },
  { "glHint", (uintptr_t)&glHint },
  { "glIsBuffer", (uintptr_t)&glIsBuffer },
  { "glIsEnabled", (uintptr_t)&glIsEnabled },
  { "glIsFramebufferOES", (uintptr_t)&glIsFramebufferOES },
  { "glIsRenderbufferOES", (uintptr_t)&glIsRenderbufferOES },
  { "glIsTexture", (uintptr_t)&glIsTexture },
  { "glLightModelf", (uintptr_t)&glLightModelf_wrapper },
  { "glLightModelfv", (uintptr_t)&glLightModelfv },
  { "glLightModelx", (uintptr_t)&glLightModelx },
  { "glLightModelxv", (uintptr_t)&glLightModelxv },
  { "glLightf", (uintptr_t)&glLightf_wrapper },
  { "glLightfv", (uintptr_t)&glLightfv },
  { "glLightx", (uintptr_t)&glLightx },
  { "glLightxv", (uintptr_t)&glLightxv },
  { "glLineWidth", (uintptr_t)&glLineWidth_wrapper },
  { "glLineWidthx", (uintptr_t)&glLineWidthx },
  { "glLoadIdentity", (uintptr_t)&glLoadIdentity },
  { "glLoadMatrixf", (uintptr_t)&glLoadMatrixf },
  { "glLoadMatrixx", (uintptr_t)&glLoadMatrixx },
  { "glLoadPaletteFromModelViewMatrixOES", (uintptr_t)&glLoadPaletteFromModelViewMatrixOES },
  { "glLogicOp", (uintptr_t)&glLogicOp },
  { "glMapBufferOES", (uintptr_t)&glMapBufferOES },
  { "glMaterialf", (uintptr_t)&glMaterialf_wrapper },
  { "glMaterialfv", (uintptr_t)&glMaterialfv },
  { "glMaterialx", (uintptr_t)&glMaterialx },
  { "glMaterialxv", (uintptr_t)&glMaterialxv },
  { "glMatrixIndexPointerOES", (uintptr_t)&glMatrixIndexPointerOES },
  { "glMatrixMode", (uintptr_t)&glMatrixMode },
  { "glMultMatrixf", (uintptr_t)&glMultMatrixf },
  { "glMultMatrixx", (uintptr_t)&glMultMatrixx },
  { "glMultiTexCoord4f", (uintptr_t)&glMultiTexCoord4f_wrapper },
  { "glMultiTexCoord4x", (uintptr_t)&glMultiTexCoord4x },
  { "glNormal3f", (uintptr_t)&glNormal3f_wrapper },
  { "glNormal3x", (uintptr_t)&glNormal3x },
  { "glNormalPointer", (uintptr_t)&glNormalPointer },
  { "glOrthof", (uintptr_t)&glOrthof_wrapper },
  { "glOrthox", (uintptr_t)&glOrthox },
  { "glPixelStorei", (uintptr_t)&glPixelStorei },
  { "glPointParameterf", (uintptr_t)&glPointParameterf_wrapper },
  { "glPointParameterfv", (uintptr_t)&glPointParameterfv },
  { "glPointParameterx", (uintptr_t)&glPointParameterx },
  { "glPointParameterxv", (uintptr_t)&glPointParameterxv },
  { "glPointSize", (uintptr_t)&glPointSize_wrapper },
  { "glPointSizePointerOES", (uintptr_t)&glPointSizePointerOES },
  { "glPointSizex", (uintptr_t)&glPointSizex },
  { "glPolygonOffset", (uintptr_t)&glPolygonOffset_wrapper },
  { "glPolygonOffsetx", (uintptr_t)&glPolygonOffsetx },
  { "glPopMatrix", (uintptr_t)&glPopMatrix },
  { "glPushMatrix", (uintptr_t)&glPushMatrix },
  { "glQueryMatrixxOES", (uintptr_t)&glQueryMatrixxOES },
  { "glReadPixels", (uintptr_t)&glReadPixels },
  { "glRenderbufferStorageOES", (uintptr_t)&glRenderbufferStorageOES },
  { "glRotatef", (uintptr_t)&glRotatef_wrapper },
  { "glRotatex", (uintptr_t)&glRotatex },
  { "glSampleCoverage", (uintptr_t)&glSampleCoverage_wrapper },
  { "glSampleCoveragex", (uintptr_t)&glSampleCoveragex },
  { "glScalef", (uintptr_t)&glScalef_wrapper },
  { "glScalex", (uintptr_t)&glScalex },
  { "glScissor", (uintptr_t)&glScissor },
  { "glShadeModel", (uintptr_t)&glShadeModel },
  { "glStencilFunc", (uintptr_t)&glStencilFunc },
  { "glStencilMask", (uintptr_t)&glStencilMask },
  { "glStencilOp", (uintptr_t)&glStencilOp },
  { "glTexCoordPointer", (uintptr_t)&glTexCoordPointer },
  { "glTexEnvf", (uintptr_t)&glTexEnvf_wrapper },
  { "glTexEnvfv", (uintptr_t)&glTexEnvfv },
  { "glTexEnvi", (uintptr_t)&glTexEnvi },
  { "glTexEnviv", (uintptr_t)&glTexEnviv },
  { "glTexEnvx", (uintptr_t)&glTexEnvx },
  { "glTexEnvxv", (uintptr_t)&glTexEnvxv },
  { "glTexGenfOES", (uintptr_t)&glTexGenfOES_wrapper },
  { "glTexGenfvOES", (uintptr_t)&glTexGenfvOES },
  { "glTexGeniOES", (uintptr_t)&glTexGeniOES },
  { "glTexGenivOES", (uintptr_t)&glTexGenivOES },
  { "glTexGenxOES", (uintptr_t)&glTexGenxOES },
  { "glTexGenxvOES", (uintptr_t)&glTexGenxvOES },
  { "glTexImage2D", (uintptr_t)&glTexImage2D },
  { "glTexParameterf", (uintptr_t)&glTexParameterf_wrapper },
  { "glTexParameterfv", (uintptr_t)&glTexParameterfv },
  { "glTexParameteri", (uintptr_t)&glTexParameteri },
  { "glTexParameteriv", (uintptr_t)&glTexParameteriv },
  { "glTexParameterx", (uintptr_t)&glTexParameterx },
  { "glTexParameterxv", (uintptr_t)&glTexParameterxv },
  { "glTexSubImage2D", (uintptr_t)&glTexSubImage2D },
  { "glTranslatef", (uintptr_t)&glTranslatef_wrapper },
  { "glTranslatex", (uintptr_t)&glTranslatex },
  { "glUnmapBufferOES", (uintptr_t)&glUnmapBufferOES },
  { "glVertexPointer", (uintptr_t)&glVertexPointer },
  { "glViewport", (uintptr_t)&glViewport },
  { "glWeightPointerOES", (uintptr_t)&glWeightPointerOES },

  { "_ZNKSt11logic_error4whatEv", (uintptr_t)&_ZNKSt11logic_error4whatEv },
  { "_ZNKSt13runtime_error4whatEv", (uintptr_t)&_ZNKSt13runtime_error4whatEv },
  { "_ZNKSt5ctypeIcE13_M_widen_initEv", (uintptr_t)&_ZNKSt5ctypeIcE13_M_widen_initEv },
  { "_ZNKSt9type_infoeqERKS_", (uintptr_t)&_ZNKSt9type_infoeqERKS_ },
  { "_ZNSo3putEc", (uintptr_t)&_ZNSo3putEc },
  { "_ZNSo5flushEv", (uintptr_t)&_ZNSo5flushEv },
  { "_ZNSo9_M_insertIPKvEERSoT_", (uintptr_t)&_ZNSo9_M_insertIPKvEERSoT_ },
  { "_ZNSo9_M_insertIbEERSoT_", (uintptr_t)&_ZNSo9_M_insertIbEERSoT_ },
  { "_ZNSo9_M_insertIdEERSoT_", (uintptr_t)&_ZNSo9_M_insertIdEERSoT_ },
  { "_ZNSo9_M_insertIlEERSoT_", (uintptr_t)&_ZNSo9_M_insertIlEERSoT_ },
  { "_ZNSo9_M_insertImEERSoT_", (uintptr_t)&_ZNSo9_M_insertImEERSoT_ },
  { "_ZNSo9_M_insertIyEERSoT_", (uintptr_t)&_ZNSo9_M_insertIyEERSoT_ },
  { "_ZNSolsEi", (uintptr_t)&_ZNSolsEi },
  { "_ZNSs4_Rep10_M_destroyERKSaIcE", (uintptr_t)&_ZNSs4_Rep10_M_destroyERKSaIcE },
  { "_ZNSs4_Rep20_S_empty_rep_storageE", (uintptr_t)&_ZNSs4_Rep20_S_empty_rep_storageE },
  { "_ZNSs6appendEPKcj", (uintptr_t)&_ZNSs6appendEPKcj },
  { "_ZNSs6appendERKSs", (uintptr_t)&_ZNSs6appendERKSs },
  { "_ZNSs6assignERKSs", (uintptr_t)&_ZNSs6assignERKSs },
  { "_ZNSsC1EPKcRKSaIcE", (uintptr_t)&_ZNSsC1EPKcRKSaIcE },
  { "_ZNSsC1ERKSs", (uintptr_t)&_ZNSsC1ERKSs },
  { "_ZNSt12length_errorC1ERKSs", (uintptr_t)&_ZNSt12length_errorC1ERKSs },
  { "_ZNSt12length_errorD1Ev", (uintptr_t)&_ZNSt12length_errorD1Ev },
  { "_ZNSt12length_errorD2Ev", (uintptr_t)&_ZNSt12length_errorD2Ev },
  { "_ZNSt13runtime_errorC2ERKSs", (uintptr_t)&_ZNSt13runtime_errorC2ERKSs },
  { "_ZNSt13runtime_errorD2Ev", (uintptr_t)&_ZNSt13runtime_errorD2Ev },
  { "_ZNSt15basic_streambufIcSt11char_traitsIcEE4syncEv", (uintptr_t)&_ZNSt15basic_streambufIcSt11char_traitsIcEE4syncEv },
  { "_ZNSt15basic_streambufIcSt11char_traitsIcEE5imbueERKSt6locale", (uintptr_t)&_ZNSt15basic_streambufIcSt11char_traitsIcEE5imbueERKSt6locale },
  { "_ZNSt15basic_streambufIcSt11char_traitsIcEE5uflowEv", (uintptr_t)&_ZNSt15basic_streambufIcSt11char_traitsIcEE5uflowEv },
  { "_ZNSt15basic_streambufIcSt11char_traitsIcEE6setbufEPci", (uintptr_t)&_ZNSt15basic_streambufIcSt11char_traitsIcEE6setbufEPci },
  { "_ZNSt15basic_streambufIcSt11char_traitsIcEE6xsgetnEPci", (uintptr_t)&_ZNSt15basic_streambufIcSt11char_traitsIcEE6xsgetnEPci },
  { "_ZNSt15basic_streambufIcSt11char_traitsIcEE6xsputnEPKci", (uintptr_t)&_ZNSt15basic_streambufIcSt11char_traitsIcEE6xsputnEPKci },
  { "_ZNSt15basic_streambufIcSt11char_traitsIcEE7seekoffExSt12_Ios_SeekdirSt13_Ios_Openmode", (uintptr_t)&_ZNSt15basic_streambufIcSt11char_traitsIcEE7seekoffExSt12_Ios_SeekdirSt13_Ios_Openmode },
  { "_ZNSt15basic_streambufIcSt11char_traitsIcEE7seekposESt4fposI9mbstate_tESt13_Ios_Openmode", (uintptr_t)&_ZNSt15basic_streambufIcSt11char_traitsIcEE7seekposESt4fposI9mbstate_tESt13_Ios_Openmode },
  { "_ZNSt15basic_streambufIcSt11char_traitsIcEE9pbackfailEi", (uintptr_t)&_ZNSt15basic_streambufIcSt11char_traitsIcEE9pbackfailEi },
  { "_ZNSt15basic_streambufIcSt11char_traitsIcEE9showmanycEv", (uintptr_t)&_ZNSt15basic_streambufIcSt11char_traitsIcEE9showmanycEv },
  { "_ZNSt15basic_streambufIcSt11char_traitsIcEE9underflowEv", (uintptr_t)&_ZNSt15basic_streambufIcSt11char_traitsIcEE9underflowEv },
  { "_ZNSt6localeC1Ev", (uintptr_t)&_ZNSt6localeC1Ev },
  { "_ZNSt6localeD1Ev", (uintptr_t)&_ZNSt6localeD1Ev },
  { "_ZNSt8__detail15_List_node_base9_M_unhookEv", (uintptr_t)&_ZNSt8__detail15_List_node_base9_M_unhookEv },
  { "_ZNSt8ios_base4InitC1Ev", (uintptr_t)&_ZNSt8ios_base4InitC1Ev },
  { "_ZNSt8ios_base4InitD1Ev", (uintptr_t)&_ZNSt8ios_base4InitD1Ev },
  { "_ZNSt8ios_baseC2Ev", (uintptr_t)&_ZNSt8ios_baseC2Ev },
  { "_ZNSt8ios_baseD2Ev", (uintptr_t)&_ZNSt8ios_baseD2Ev },
  { "_ZNSt9basic_iosIcSt11char_traitsIcEE4initEPSt15basic_streambufIcS1_E", (uintptr_t)&_ZNSt9basic_iosIcSt11char_traitsIcEE4initEPSt15basic_streambufIcS1_E },
  { "_ZNSt9basic_iosIcSt11char_traitsIcEE5clearESt12_Ios_Iostate", (uintptr_t)&_ZNSt9basic_iosIcSt11char_traitsIcEE5clearESt12_Ios_Iostate },
  { "_ZNSt9exceptionD2Ev", (uintptr_t)&_ZNSt9exceptionD2Ev },
  { "_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_i", (uintptr_t)&_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_i },
  { "_ZSt16__throw_bad_castv", (uintptr_t)&_ZSt16__throw_bad_castv },
  { "_ZSt17__throw_bad_allocv", (uintptr_t)&_ZSt17__throw_bad_allocv },
  { "_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_", (uintptr_t)&_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_ },
  { "_ZSt9terminatev", (uintptr_t)&_ZSt9terminatev },
  { "_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc", (uintptr_t)&_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc },
  { "_ZTISt12length_error", (uintptr_t)&_ZTISt12length_error },
  { "_ZTISt13runtime_error", (uintptr_t)&_ZTISt13runtime_error },
  { "_ZTISt15basic_streambufIcSt11char_traitsIcEE", (uintptr_t)&_ZTISt15basic_streambufIcSt11char_traitsIcEE },
  { "_ZTISt9exception", (uintptr_t)&_ZTISt9exception },
  { "_ZTVN10__cxxabiv117__class_type_infoE", (uintptr_t)&_ZTVN10__cxxabiv117__class_type_infoE + 8 },
  { "_ZTVN10__cxxabiv119__pointer_type_infoE", (uintptr_t)&_ZTVN10__cxxabiv119__pointer_type_infoE + 8 },
  { "_ZTVN10__cxxabiv120__function_type_infoE", (uintptr_t)&_ZTVN10__cxxabiv120__function_type_infoE + 8 },
  { "_ZTVN10__cxxabiv120__si_class_type_infoE", (uintptr_t)&_ZTVN10__cxxabiv120__si_class_type_infoE + 8 },
  { "_ZTVN10__cxxabiv121__vmi_class_type_infoE", (uintptr_t)&_ZTVN10__cxxabiv121__vmi_class_type_infoE + 8 },
  { "_ZTVSo", (uintptr_t)&_ZTVSo },
  { "_ZTVSt11logic_error", (uintptr_t)&_ZTVSt11logic_error },
  { "_ZTVSt12length_error", (uintptr_t)&_ZTVSt12length_error },
  { "_ZTVSt13runtime_error", (uintptr_t)&_ZTVSt13runtime_error },
  { "_ZTVSt15basic_streambufIcSt11char_traitsIcEE", (uintptr_t)&_ZTVSt15basic_streambufIcSt11char_traitsIcEE },
  { "_ZTVSt9basic_iosIcSt11char_traitsIcEE", (uintptr_t)&_ZTVSt9basic_iosIcSt11char_traitsIcEE },
};

int numfunc_default_dynlib = sizeof(default_dynlib) / sizeof(so_default_dynlib);
