/*
 * main.c -- Need for Speed: Hot Pursuit .so loader
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
#include <kubridge.h>

#include <stdio.h>
#include <stdlib.h>

#include <PVR_PSP2/EGL/eglplatform.h>
#include <PVR_PSP2/EGL/egl.h>
#include <PVR_PSP2/GLES/gl.h>
#include <PVR_PSP2/GLES/glext.h>
#include <PVR_PSP2/gpu_es4/psp2_pvr_hint.h>

#include "main.h"
#include "so_util.h"
#include "dynlib.h"
#include "jni.h"
#include "fios.h"

int _newlib_heap_size_user = MEMORY_NEWLIB_MB * 1024 * 1024;
unsigned int sceLibcHeapSize = MEMORY_SCELIBC_MB * 1024 * 1024;

bool input_thread_run = true;
bool audio_thread_run = true;
SceUInt main_thid = 0;
SceUInt audio_thid = 0;
SceUInt input_thid = 0;

EGLDisplay egl_display;
EGLSurface egl_surface;

bool keep_awake = false;

so_module nfshp_mod;

void (* JNI_OnLoad)(void *vm, void *reserved);
void (* Java_com_ea_EAThread_EAThread_Init)(void *env, void *obj);
void (* Java_com_ea_EAIO_EAIO_StartupNativeImpl)(void *env, void *obj, int assets, const char *data_dir_path, const char *files_dir_path, const char *external_storage_path);
void (* Java_com_ea_EAMIO_StorageDirectory_StartupNativeImpl)(void *env, void *obj);
void (* Java_com_ea_blast_MainActivity_NativeOnCreate)(void *env, void *obj);
void (* Java_com_ea_blast_MainActivity_NativeOnLowMemory)(void *env, void *obj);
void (* Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated)(void *env, void *obj);
void (* Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceChanged)(void *env, void *obj, int width, int height);
void (* Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame)(void *env, void *obj);
void (* Java_com_ea_blast_TouchSurfaceAndroid_NativeOnPointerEvent)(void *env, void *obj, int msg_id, nfshp_module_type_id_t module_id, int pointer_id, float x, float y);
void (* Java_com_ea_blast_AccelerometerAndroidDelegate_NativeOnAcceleration)(void *env, void *obj, float x, float y, float z);
void (* Java_com_mpp_android_fmod_FModPlayer_audioCallback)(void *env, void *obj, jni_byte_array_t *buffer, int userData1, int userData2);

void *__wrap_memcpy(void *dest, const void *src, size_t n) {
  return sceClibMemcpy(dest, src, n);
}

void *__wrap_memmove(void *dest, const void *src, size_t n) {
  return sceClibMemmove(dest, src, n);
}

void *__wrap_memset(void *s, int c, size_t n) {
  return sceClibMemset(s, c, n);
}

#ifdef DEBUG
int debugPrintf(char *text, ...) {
  va_list list;
  static char string[0x1000];

  va_start(list, text);
  vsprintf(string, text, list);
  va_end(list);

#ifdef DEBUG_FILE
  SceUID fd = sceIoOpen("ux0:data/nfshp/log.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
  if (fd >= 0) {
    sceIoWrite(fd, string, strlen(string));
    sceIoClose(fd);
  }
#else
  SceKernelThreadInfo status;
  status.size = sizeof(SceKernelThreadInfo);
  int ret = sceKernelGetThreadInfo(sceKernelGetThreadId(), &status);

  printf("[%s] %s", ret == 0 ? status.name : "?", string);
#endif
  return 0;
}
#endif

int ret0(void) {
  return 0;
}

int ret1(void) {
  return 1;
}

int check_kubridge(void) {
  int search_unk[2];
  return _vshKernelSearchModuleByName("kubridge", search_unk);
}

int file_exists(const char *path) {
  SceIoStat stat;
  return sceIoGetstat(path, &stat) >= 0;
}

int loadlib(const char *path) {
  return sceKernelLoadStartModule(path, 0, NULL, 0, NULL, NULL);
}

void crash_at(uintptr_t addr) {
  uint32_t bkpt = 0xE120BE70;
  kuKernelCpuUnrestrictedMemcpy((void *)(nfshp_mod.text_base + addr), &bkpt, sizeof(bkpt));
}

void nop_at(uintptr_t addr) {
  uint32_t nop = 0xE320F000;
  kuKernelCpuUnrestrictedMemcpy((void *)(nfshp_mod.text_base + addr), &nop, sizeof(nop));
}

int gl_init() {
  EGLBoolean res;
  EGLint major_version, minor_version;

  EGLContext context;

  EGLConfig config;
  EGLint config_num;
  EGLint config_attr[] = {
    EGL_BUFFER_SIZE,    EGL_DONT_CARE,
    EGL_DEPTH_SIZE,     24,
    EGL_STENCIL_SIZE,   0,
    EGL_RED_SIZE,       5,
    EGL_GREEN_SIZE,     6,
    EGL_BLUE_SIZE,      5,
    EGL_ALPHA_SIZE,     0,
    EGL_SAMPLE_BUFFERS, 1,
    EGL_SAMPLES,        4,
    EGL_NONE
  };

  egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  res = eglInitialize(egl_display, &major_version, &minor_version);
  if (res == EGL_FALSE) {
      return -1;
  }

  res = eglGetConfigs(egl_display, &config, 1, &config_num);
  if (config_num == 0) {
      return -1;
  }

  res = eglChooseConfig(egl_display, config_attr, &config, 1, &config_num);
  if (res == EGL_FALSE) {
      return -1;
  }

  egl_surface = eglCreateWindowSurface(egl_display, config, (EGLNativeWindowType)0, NULL);
  context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, NULL);

  eglMakeCurrent(egl_display, egl_surface, egl_surface, context);
  eglSwapInterval(egl_display, (EGLint)1);
  return 0;
}

void input_map_button(SceCtrlData *pad, SceCtrlData *prev_pad, SceCtrlButtons button, nfshp_input_event_t press, nfshp_input_event_t release) {
  if (pad->buttons & button && ~prev_pad->buttons & button) {
    if (press != INPUT_EVENT_INVALID)
      nfshp_send_input_event(press, 0);
  } else if (~pad->buttons & button && prev_pad->buttons & button) {
    if (release != INPUT_EVENT_INVALID)
      nfshp_send_input_event(release, 0);
  }
}

int input_thread(SceSize argc, void *argp) {
  void *jni_env = jni_get_env();

  bool touch_pointer_down = false;
  float touch_x = 0.0f;
  float touch_y = 0.0f;
  SceCtrlData prev_pad;
  memset(&prev_pad, 0, sizeof(SceCtrlData));

  SceTouchPanelInfo panel_info_front;
  sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
  sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
  sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &panel_info_front);
  sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);

  if (ENABLE_ACCELEROMETER) {
    sceMotionStartSampling();
  }

  while (input_thread_run) {
    // Touchscreen
    SceTouchData touch;
    sceTouchPeek(0, &touch, 1);

    if (touch.reportNum > 0 || touch_pointer_down) {
      nfshp_touch_action_id_t action = TOUCH_UNDEFINED;

      if (touch.reportNum == 0 && touch_pointer_down) {
        touch_pointer_down = false;
        action = TOUCH_RAW_POINTER_UP;
      }
      else if (touch.reportNum > 0) {
        touch_x = (float)touch.report[0].x / 1920.0f * SCREEN_W;
        touch_y = (float)touch.report[0].y  / 1088.0f * SCREEN_H;

        action = touch_pointer_down ? TOUCH_RAW_POINTER_MOVE : TOUCH_RAW_POINTER_DOWN;
        touch_pointer_down = true;
      }

      Java_com_ea_blast_TouchSurfaceAndroid_NativeOnPointerEvent(jni_env, NULL, action, MODULE_TOUCHSCREEN, 0, touch_x, touch_y);
    }

    // Ctrl
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(0, &pad, 1);

    float steering_value = (pad.lx - 128.0f) * -32.0f / 128.0f; // normalize to inverted [-32, 32]
    nfshp_send_input_event(DRIVING_INPUT_EVENT_STEERING, *(uint32_t *)&steering_value);

    input_map_button(&pad, &prev_pad, SCE_CTRL_RTRIGGER, DRIVING_INPUT_EVENT_START_ACCELERATING, DRIVING_INPUT_EVENT_STOP_ACCELERATING);
    input_map_button(&pad, &prev_pad, SCE_CTRL_LTRIGGER, DRIVING_INPUT_EVENT_START_BRAKING, DRIVING_INPUT_EVENT_STOP_BRAKING);
    input_map_button(&pad, &prev_pad, SCE_CTRL_CROSS, DRIVING_INPUT_EVENT_TOGGLE_NITRO, INPUT_EVENT_INVALID);
    input_map_button(&pad, &prev_pad, SCE_CTRL_CIRCLE, DRIVING_INPUT_EVENT_START_HANDBRAKING, DRIVING_INPUT_EVENT_STOP_HANDBRAKING);

    input_map_button(&pad, &prev_pad, SCE_CTRL_UP, DRIVING_INPUT_EVENT_START_OVERDRIVE, INPUT_EVENT_INVALID);
    input_map_button(&pad, &prev_pad, SCE_CTRL_LEFT, DRIVING_INPUT_EVENT_START_OILSLICK, INPUT_EVENT_INVALID);
    input_map_button(&pad, &prev_pad, SCE_CTRL_RIGHT, DRIVING_INPUT_EVENT_START_JAMMER, INPUT_EVENT_INVALID);

    input_map_button(&pad, &prev_pad, SCE_CTRL_UP, DRIVING_INPUT_EVENT_START_ROADBLOCK, INPUT_EVENT_INVALID);
    input_map_button(&pad, &prev_pad, SCE_CTRL_LEFT, DRIVING_INPUT_EVENT_START_SPIKESTRIP, INPUT_EVENT_INVALID);
    input_map_button(&pad, &prev_pad, SCE_CTRL_RIGHT, DRIVING_INPUT_EVENT_START_EMP, INPUT_EVENT_INVALID);

    memcpy(&prev_pad, &pad, sizeof(SceCtrlData));

    // Accelerometer
    if (ENABLE_ACCELEROMETER) {
      SceMotionSensorState motion;
      sceMotionGetSensorState(&motion, 1);

      Java_com_ea_blast_AccelerometerAndroidDelegate_NativeOnAcceleration(jni_env, NULL,
        // swap x/y to match device rotation
        motion.accelerometer.y * 9.81f,
        motion.accelerometer.x * 9.81f,
        motion.accelerometer.z * 9.81f
      );
    }

    sceKernelDelayThread(16666);
  }

  if (ENABLE_ACCELEROMETER) {
    sceMotionStopSampling();
  }

  return 0;
}

int main_thread(SceSize argc, void *argp) {
  void *jni_vm = jni_create_vm();
  void *jni_env = jni_create_env();

  JNI_OnLoad(jni_vm, NULL);
  Java_com_ea_EAThread_EAThread_Init(jni_env, NULL);
  Java_com_ea_EAIO_EAIO_StartupNativeImpl(jni_env, NULL, 0x69696969, VAR_PATH, VAR_PATH, VAR_PATH);
  Java_com_ea_EAMIO_StorageDirectory_StartupNativeImpl(jni_env, NULL);
  Java_com_ea_blast_MainActivity_NativeOnCreate(jni_env, NULL);

  gl_init();

  Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated(jni_env, NULL);
  Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceChanged(jni_env, NULL, SCREEN_W, SCREEN_H);

  // Spawn input thread
  input_thid = sceKernelCreateThread("input_thread", (SceKernelThreadEntry)input_thread,
    96, DEFAULT_STACK_SIZE, 0, SCE_KERNEL_CPU_MASK_USER_2, NULL);
  input_thread_run = true;
  sceKernelStartThread(input_thid, 0, NULL);

  while (true) {
    if (keep_awake) {
      sceKernelPowerTick(0);
    }

    Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame(jni_env, NULL);
    eglSwapBuffers(egl_display, egl_surface);
  }

  input_thread_run = false;
  sceKernelWaitThreadEnd(input_thid, NULL, NULL);
  sceKernelDeleteThread(input_thid);

  return 0;
}

int audio_thread(SceSize args, int *argp) {
  void *jni_env = jni_get_env();

  int sampleRate = argp[0];
  // int outFormat = argp[1];
  int bufferLen = argp[2];
  int userData1 = argp[3];
  int userData2 = argp[4];

  jni_byte_array_t *byte_array = malloc(sizeof(jni_byte_array_t));
  byte_array->data = malloc(bufferLen);
  byte_array->size = bufferLen;

  int audio_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, bufferLen >> 2, sampleRate, SCE_AUDIO_OUT_MODE_STEREO);

  while (audio_thread_run) {
    Java_com_mpp_android_fmod_FModPlayer_audioCallback(jni_env, NULL, byte_array, userData1, userData2);
    sceAudioOutSetConfig(audio_port, bufferLen >> 2, sampleRate, SCE_AUDIO_OUT_MODE_STEREO);
    sceAudioOutOutput(audio_port, byte_array->data);
  }

  free(byte_array->data);
  free(byte_array);

  sceAudioOutReleasePort(audio_port);
  return 0;
}

void nfshp_init_audio_track(int sampleRate, int outFormat, int bufferLen, long long userData) {
  nfshp_stop_audio_track();

  audio_thid = sceKernelCreateThread("audio_thread", (SceKernelThreadEntry)audio_thread,
    SCE_KERNEL_PROCESS_PRIORITY_USER_DEFAULT, DEFAULT_STACK_SIZE, 0, SCE_KERNEL_CPU_MASK_USER_2, NULL);
  audio_thread_run = true;

  int args[] = {
    sampleRate,
    outFormat,
    bufferLen,
    ((int)userData) & -1,
    (int)((userData >> 32) & -1)
  };

  sceKernelStartThread(audio_thid, sizeof(args), &args);
}

void nfshp_stop_audio_track(void) {
  if (audio_thid == 0) {
    return;
  }

  audio_thread_run = false;
  sceKernelWaitThreadEnd(audio_thid, NULL, NULL);
  sceKernelDeleteThread(audio_thid);
  audio_thid = 0;
}

void nfshp_send_input_event(nfshp_input_event_t event, uint32_t arg) {
  uintptr_t a1 = *(uintptr_t *)(nfshp_mod.data_base + 0x28118); // dword_82875C
  if (!a1) {
    return; // not yet init
  }

  void (* sub_2474F4)(uintptr_t a1, uintptr_t *event_args) = (void *)(nfshp_mod.text_base + 0x2474F4);

  uintptr_t vfunc_offset = 0;
  switch (event) {
    case DRIVING_INPUT_EVENT_START_ACCELERATING:
      vfunc_offset = 0x802F80;
      break;
    case DRIVING_INPUT_EVENT_STOP_ACCELERATING:
      vfunc_offset = 0x802F98;
      break;
    case DRIVING_INPUT_EVENT_START_BRAKING:
      vfunc_offset = 0x802FB0;
      break;
    case DRIVING_INPUT_EVENT_STOP_BRAKING:
      vfunc_offset = 0x802FC8;
      break;
    case DRIVING_INPUT_EVENT_START_HANDBRAKING:
      vfunc_offset = 0x802130;
      break;
    case DRIVING_INPUT_EVENT_STOP_HANDBRAKING:
      vfunc_offset = 0x802FE0;
      break;
    case DRIVING_INPUT_EVENT_TOGGLE_NITRO:
      vfunc_offset = 0x803018;
      break;
    case DRIVING_INPUT_EVENT_START_OVERDRIVE:
      vfunc_offset = 0x803030;
      break;
    case DRIVING_INPUT_EVENT_START_OILSLICK:
      vfunc_offset = 0x803048;
      break;
    case DRIVING_INPUT_EVENT_START_JAMMER:
      vfunc_offset = 0x803060;
      break;
    case DRIVING_INPUT_EVENT_START_ROADBLOCK:
      vfunc_offset = 0x803078;
      break;
    case DRIVING_INPUT_EVENT_START_SPIKESTRIP:
      vfunc_offset = 0x803090;
      break;
    case DRIVING_INPUT_EVENT_START_EMP:
      vfunc_offset = 0x8030A8;
      break;
    case DRIVING_INPUT_EVENT_STEERING:
      vfunc_offset = 0x802F68;
      break;
    case SWIPE_EVENT_SWIPE_LEFT_STARTED:
      vfunc_offset = 0x803398;
      break;
    case SWIPE_EVENT_SWIPE_RIGHT_STARTED:
      vfunc_offset = 0x8033B0;
      break;
    case SWIPE_EVENT_SWIPE_UP_STARTED:
      vfunc_offset = 0x8033C8;
      break;
    case SWIPE_EVENT_SWIPE_DOWN_STARTED:
      vfunc_offset = 0x8033E0;
      break;
    case SWIPE_EVENT_SWIPE_LEFT_FINISHED:
      vfunc_offset = 0x803410;
      break;
    case SWIPE_EVENT_SWIPE_RIGHT_FINISHED:
      vfunc_offset = 0x803428;
      break;
    case SWIPE_EVENT_SWIPE_UP_FINISHED:
      vfunc_offset = 0x803440;
      break;
    case SWIPE_EVENT_SWIPE_DOWN_FINISHED:
      vfunc_offset = 0x803458;
      break;
    case INPUT_EVENT_INVALID:
      break;
  }

  if (!vfunc_offset) {
    return;
  }

  uintptr_t event_args[] = {
    nfshp_mod.text_base + vfunc_offset,
    event,
    arg,
    0, 0, 0, 0
  };

  sub_2474F4(a1, event_args);
}

void sub_9A270_hook(int a1, int ms) {
  sceKernelDelayThreadCB(ms * 1000);
}

void sub_55A910_hook(float sec) {
  sceKernelDelayThreadCB((SceUInt)(sec * 1000 * 1000));
}

int main(int argc, char *argv[]) {
  SceAppUtilInitParam init_param;
  SceAppUtilBootParam boot_param;
  memset(&init_param, 0, sizeof(SceAppUtilInitParam));
  memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
  sceAppUtilInit(&init_param, &boot_param);

  loadlib("app0:libgpu_es4_ext.suprx");
  loadlib("app0:libIMGEGL.suprx");

  PVRSRV_PSP2_APPHINT hint;
  PVRSRVInitializeAppHint(&hint);
  PVRSRVCreateVirtualAppHint(&hint);

  scePowerSetArmClockFrequency(444);
  scePowerSetBusClockFrequency(222);
  scePowerSetGpuClockFrequency(222);
  scePowerSetGpuXbarClockFrequency(166);

  if (check_kubridge() < 0)
    return 0;

  if (so_load(&nfshp_mod, SO_PATH, LOAD_ADDRESS) < 0)
    return 0;

  so_relocate(&nfshp_mod);
  so_resolve(&nfshp_mod, default_dynlib, numfunc_default_dynlib, 0);
  so_flush_caches(&nfshp_mod);
  so_initialize(&nfshp_mod);

  if (fios_init() < 0)
    return 0;

  JNI_OnLoad = (void *)so_symbol(&nfshp_mod, "JNI_OnLoad");
  Java_com_ea_EAThread_EAThread_Init = (void *)so_symbol(&nfshp_mod, "Java_com_ea_EAThread_EAThread_Init");
  Java_com_ea_EAIO_EAIO_StartupNativeImpl = (void *)so_symbol(&nfshp_mod, "Java_com_ea_EAIO_EAIO_StartupNativeImpl");
  Java_com_ea_EAMIO_StorageDirectory_StartupNativeImpl = (void *)so_symbol(&nfshp_mod, "Java_com_ea_EAMIO_StorageDirectory_StartupNativeImpl");
  Java_com_ea_blast_MainActivity_NativeOnCreate = (void *)so_symbol(&nfshp_mod, "Java_com_ea_blast_MainActivity_NativeOnCreate");
  Java_com_ea_blast_MainActivity_NativeOnLowMemory = (void *)so_symbol(&nfshp_mod, "Java_com_ea_blast_MainActivity_NativeOnLowMemory");
  Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated = (void *)so_symbol(&nfshp_mod, "Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated");
  Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceChanged = (void *)so_symbol(&nfshp_mod, "Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceChanged");
  Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame = (void *)so_symbol(&nfshp_mod, "Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame");
  Java_com_ea_blast_TouchSurfaceAndroid_NativeOnPointerEvent = (void *)so_symbol(&nfshp_mod, "Java_com_ea_blast_TouchSurfaceAndroid_NativeOnPointerEvent");
  Java_com_ea_blast_AccelerometerAndroidDelegate_NativeOnAcceleration = (void *)so_symbol(&nfshp_mod, "Java_com_ea_blast_AccelerometerAndroidDelegate_NativeOnAcceleration");
  Java_com_mpp_android_fmod_FModPlayer_audioCallback = (void *)so_symbol(&nfshp_mod, "Java_com_mpp_android_fmod_FModPlayer_audioCallback");

  // apply patches
  hook_arm(nfshp_mod.text_base + 0x9A270, (uintptr_t)&sub_9A270_hook);
  hook_arm(nfshp_mod.text_base + 0x55A910, (uintptr_t)&sub_55A910_hook);

  // accept terms of service by default since the button is broken
  uint8_t patch[] = {0x29, 0x62, 0xC4, 0xE5}; // STRB R6, [R4,#0x229], R6 = 0x22
  kuKernelCpuUnrestrictedMemcpy((void *)(nfshp_mod.text_base + 0x21F37C), &patch, sizeof(patch));

  uint8_t patch2[] = {0x02, 0x00, 0x52, 0xE1}; // CMP R2, R2
  kuKernelCpuUnrestrictedMemcpy((void *)(nfshp_mod.text_base + 0x56DBF4), &patch2, sizeof(patch2));

  if (!ENABLE_ACCELEROMETER) {
    // disable accelerometer for steering
    nop_at(0x242E68);
    nop_at(0x242F60);
  }

  main_thid = sceKernelCreateThread("main_thread", (SceKernelThreadEntry)main_thread,
    64, DEFAULT_STACK_SIZE, 0, SCE_KERNEL_CPU_MASK_USER_0, NULL);

  sceKernelStartThread(main_thid, 0, NULL);
  return sceKernelExitDeleteThread(0);
}
