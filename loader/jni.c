/*
 * jni_patch.c -- Fake Java Native Interface
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "jni.h"

static char fake_vm[0x1000];
static char fake_env[0x1000];

static jni_class_t name_to_class_ids[] = {
  { "java/io/InputStream", CLASS_JAVA_IO_INPUT_STREAM },
  { "android/content/res/AssetFileDescriptor", CLASS_ANDROID_ASSET_FILE_DESCRIPTOR },
  { "android/media/AudioTrack", CLASS_ANDROID_AUDIO_TRACK },
  { "com/ea/EAActivityArguments/EAActivityArguments", CLASS_EA_ACTIVITY_ARGUMENTS },
  { "com/ea/EAMIO/StorageDirectory", CLASS_EAMIO_STORAGE_DIRECTORY },
  { "com/ea/blast/AccelerometerAndroidDelegate", CLASS_EA_ACCELEROMETER },
  { "com/ea/blast/DeviceOrientationHandlerAndroidDelegate", CLASS_EA_DEVICE_ORIENTATION_HANDLER },
  { "com/ea/blast/DisplayAndroidDelegate", CLASS_EA_DISPLAY },
  { "com/ea/blast/MainActivity", CLASS_EA_MAIN_ACTIVITY },
  { "com/ea/blast/PhysicalKeyboardAndroidDelegate", CLASS_EA_PHYSICAL_KEYBOARD },
  { "com/ea/blast/PowerManagerAndroid", CLASS_EA_POWER_MANAGER },
  { "com/ea/blast/SystemAndroidDelegate", CLASS_EA_SYSTEM },
  { "com/ea/blast/TouchSurfaceAndroid", CLASS_EA_TOUCH_SURFACE },
  { "com/ea/blast/VibratorAndroidDelegate", CLASS_EA_VIBRATOR },
  { "com/ea/blast/ViewAndroidDelegate", CLASS_EA_VIEW },
  { "com/mpp/android/main/crossActivity/CrossActivity", CLASS_MPP_CROSS_ACTIVITY },
  { "com/mpp/android/fmod/FModPlayer", CLASS_MPP_FMOD_PLAYER },
};

static jni_method_t name_to_method_ids[] = {
  { CLASS_UNKNOWN, "<init>", INIT },

  // java/io/InputStream
  { CLASS_JAVA_IO_INPUT_STREAM, "read", READ },
  { CLASS_JAVA_IO_INPUT_STREAM, "close", CLOSE },
  { CLASS_JAVA_IO_INPUT_STREAM, "skip", SKIP },

  // android/content/res/AssetFileDescriptor
  { CLASS_ANDROID_ASSET_FILE_DESCRIPTOR, "open", OPEN },
  { CLASS_ANDROID_ASSET_FILE_DESCRIPTOR, "openFd", OPEN_FD },
  { CLASS_ANDROID_ASSET_FILE_DESCRIPTOR, "list", LIST },
  { CLASS_ANDROID_ASSET_FILE_DESCRIPTOR, "getLength", GET_LENGTH },
  { CLASS_ANDROID_ASSET_FILE_DESCRIPTOR, "close", CLOSE_2 },

  // android/media/AudioTrack
  { CLASS_ANDROID_AUDIO_TRACK, "getMinBufferSize", GET_MIN_BUFFER_SIZE },

  // com/ea/EAActivityArguments/EAActivityArguments
  { CLASS_EA_ACTIVITY_ARGUMENTS, "GetCommandLineArguments", GET_COMMAND_LINE_ARGUMENTS },

  // com/ea/blast/MainActivity
  { CLASS_EA_MAIN_ACTIVITY, "getDisplayWidth", GET_DISPLAY_WIDTH },
  { CLASS_EA_MAIN_ACTIVITY, "getDisplayHeight", GET_DISPLAY_HEIGHT },

  // com/ea/blast/PowerManagerAndroid
  { CLASS_EA_POWER_MANAGER, "ApplyKeepAwake", APPLY_KEEP_AWAKE },

  // com/ea/blast/DisplayAndroidDelegate
  { CLASS_EA_DISPLAY, "GetDpiX", GET_DPI_X },
  { CLASS_EA_DISPLAY, "GetDpiY", GET_DPI_Y },
  { CLASS_EA_DISPLAY, "GetCurrentWidth", GET_CURRENT_WIDTH },
  { CLASS_EA_DISPLAY, "GetCurrentHeight", GET_CURRENT_HEIGHT },
  { CLASS_EA_DISPLAY, "GetStdOrientation", GET_STD_ORIENTATION },
  { CLASS_EA_DISPLAY, "SetStdOrientation", SET_STD_ORIENTATION },
  { CLASS_EA_DISPLAY, "GetGLView", GET_GL_VIEW },

  // com/ea/blast/PhysicalKeyboardAndroidDelegate
  { CLASS_EA_PHYSICAL_KEYBOARD, "IsPhysicalKeyboardVisible", IS_PHYSICAL_KEYBOARD_VISIBLE },

  // com/ea/EAMIO/StorageDirectory
  { CLASS_EAMIO_STORAGE_DIRECTORY, "GetObbFilePath", GET_OBB_FILE_PATH },
  { CLASS_EAMIO_STORAGE_DIRECTORY, "GetInternalStorageDirectory", GET_INTERNAL_STORAGE_DIRECTORY },
  { CLASS_EAMIO_STORAGE_DIRECTORY, "GetPrimaryExternalStorageDirectoryRoot", GET_PRIMARY_EXTERNAL_STORAGE_DIRECTORY_ROOT },
  { CLASS_EAMIO_STORAGE_DIRECTORY, "GetPrimaryExternalStorageDirectory", GET_PRIMARY_EXTERNAL_STORAGE_DIRECTORY },
  { CLASS_EAMIO_STORAGE_DIRECTORY, "GetPrimaryExternalStorageState", GET_PRIMARY_EXTERNAL_STORAGE_STATE },

  // com/ea/blast/DeviceOrientationHandlerAndroidDelegate
  { CLASS_EA_DEVICE_ORIENTATION_HANDLER, "OnLifeCycleFocusGained", ON_LIFE_CYCLE_FOCUS_GAINED },
  { CLASS_EA_DEVICE_ORIENTATION_HANDLER, "SetEnabled", SET_ENABLED },

  // com/ea/blast/SystemAndroidDelegate
  { CLASS_EA_SYSTEM, "GetAccelerometerCount", GET_ACCELEROMETER_COUNT },
  { CLASS_EA_SYSTEM, "GetTouchScreenCount", GET_TOUCH_SCREEN_COUNT },
  { CLASS_EA_SYSTEM, "GetCameraCount", GET_CAMERA_COUNT },
  { CLASS_EA_SYSTEM, "GetGyroscopeCount", GET_GYROSCOPE_COUNT },
  { CLASS_EA_SYSTEM, "GetMicrophoneCount", GET_MICROPHONE_COUNT },
  { CLASS_EA_SYSTEM, "GetVibratorCount", GET_VIBRATOR_COUNT },
  { CLASS_EA_SYSTEM, "GetApplicationVersionCode", GET_APPLICATION_VERSION_CODE },
  { CLASS_EA_SYSTEM, "GetApplicationId", GET_APPLICATION_ID },
  { CLASS_EA_SYSTEM, "GetApplicationVersion", GET_APPLICATION_VERSION },
  { CLASS_EA_SYSTEM, "GetDeviceUniqueId", GET_DEVICE_UNIQUE_ID },
  { CLASS_EA_SYSTEM, "GetLocale", GET_LOCALE },
  { CLASS_EA_SYSTEM, "GetLanguage", GET_LANGUAGE },
  { CLASS_EA_SYSTEM, "IntentView", INTENT_VIEW },

  // com/ea/blast/TouchSurfaceAndroid
  { CLASS_EA_TOUCH_SURFACE, "IsTouchScreenMultiTouch", IS_TOUCH_SCREEN_MULTI_TOUCH },

  // com/mpp/android/main/crossActivity/CrossActivity
  { CLASS_MPP_CROSS_ACTIVITY, "isAnyMusicPlaying", IS_ANY_MUSIC_PLAYING },

  // com/mpp/android/fmod/FModPlayer
  { CLASS_MPP_FMOD_PLAYER, "initAudioTrack", INIT_AUDIO_TRACK },
  { CLASS_MPP_FMOD_PLAYER, "stopAudioTrack", STOP_AUDIO_TRACK },
};

const jni_method_t *jni_find_method(int class_id, int method_id, const char *name) {
  if (class_id == CLASS_UNKNOWN && (method_id == UNKNOWN || name == NULL)) {
    return NULL;
  }

  for (int i = 0; i < sizeof(name_to_method_ids) / sizeof(jni_method_t); i++) {
    if ((class_id == CLASS_UNKNOWN || name_to_method_ids[i].class == class_id)
        && (method_id == UNKNOWN || name_to_method_ids[i].id == method_id)
        && (name == NULL || strcmp(name, name_to_method_ids[i].name) == 0)) {
      return &name_to_method_ids[i];
    }
  }
  return NULL;
}

const jni_class_t *jni_find_class(int class_id, const char *name) {
  if (class_id == CLASS_UNKNOWN && name == NULL) {
    return NULL;
  }

  for (int i = 0; i < sizeof(name_to_class_ids) / sizeof(jni_class_t); i++) {
    if ((class_id == CLASS_UNKNOWN || name_to_class_ids[i].id == class_id)
        && (name == NULL || strcmp(name, name_to_class_ids[i].name) == 0)) {
      return &name_to_class_ids[i];
    }
  }
  return NULL;
}

void *FindClass(void *env, const char *name) {
  const jni_class_t *class = jni_find_class(CLASS_UNKNOWN, name);
  return (void *)(class ? class->id : CLASS_UNKNOWN);
}

int CallBooleanMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case IS_PHYSICAL_KEYBOARD_VISIBLE:
      return 0;
    case INTENT_VIEW:
      return 1;
    default:
      return 0;
  }
}

float CallFloatMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case GET_DPI_X:
    case GET_DPI_Y:
      return 220.68f;
      break;
    default:
      return 0.0f;
  }
}

int CallIntMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case GET_DISPLAY_WIDTH:
    case GET_CURRENT_WIDTH:
      return SCREEN_W;
    case GET_DISPLAY_HEIGHT:
    case GET_CURRENT_HEIGHT:
      return SCREEN_H;
    case GET_STD_ORIENTATION:
      return DISPLAY_ORIENTATION_ROTATED_RIGHT;
    case GET_ACCELEROMETER_COUNT:
      // The game requires an accelerometer...
      // if we return 0 here, we get NPE crash at 0xA6530
      return 1;
    case GET_TOUCH_SCREEN_COUNT:
    case GET_GYROSCOPE_COUNT:
    case GET_MICROPHONE_COUNT:
      return 1;
    case GET_VIBRATOR_COUNT:
      return 0;
    case GET_CAMERA_COUNT:
      return 2;
    case GET_APPLICATION_VERSION_CODE:
      return 2028;
    default:
      return 0;
  }
}

long CallLongMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  return 0L;
}

void *CallObjectMethod(void *env, void *obj, int methodID, ...) {
  return NULL;
}

void *CallObjectMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case GET_LOCALE:
      return "en_US";
    case GET_LANGUAGE:
      return "en";
    case GET_APPLICATION_ID:
      return "com.eamobile.nfshp_row_wf";
    case GET_APPLICATION_VERSION:
      return "2.0.28";
    case GET_DEVICE_UNIQUE_ID:
      return "1337";
    default:
      return NULL;
  }
}

void CallVoidMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case APPLY_KEEP_AWAKE:
      keep_awake = (bool)args[0];
      break;
    case SET_STD_ORIENTATION:
      break;
    case INIT_AUDIO_TRACK:
      nfshp_init_audio_track(args[0], args[1], args[2], args[3]);
      break;
    case STOP_AUDIO_TRACK:
      nfshp_stop_audio_track();
      break;
  }
}

int CallStaticIntMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case GET_MIN_BUFFER_SIZE:
      return 4096;
    default:
      return 0;
  }
}

void *CallStaticObjectMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case GET_INTERNAL_STORAGE_DIRECTORY:
    case GET_PRIMARY_EXTERNAL_STORAGE_DIRECTORY:
      return VAR_PATH;
    case GET_OBB_FILE_PATH:
      return OBB_PATH;
    default:
      return NULL;
  }
}

int CallStaticBooleanMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case IS_ANY_MUSIC_PLAYING:
    case IS_TOUCH_SCREEN_MULTI_TOUCH:
    default:
      return 0;
  }
}

void CallStaticVoidMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  // ok
}

int GetMethodID(void *env, void *class, const char *name, const char *sig) {
  const jni_method_t *method = jni_find_method((int)class, UNKNOWN, name);
  return method ? method->id : UNKNOWN;
}

int GetStaticMethodID(void *env, void *class, const char *name, const char *sig) {
  const jni_method_t *method = jni_find_method((int)class, UNKNOWN, name);
  return method ? method->id : UNKNOWN;
}

void RegisterNatives(void *env, int r1, void *r2) {
  // ok
}

int GetJavaVM(void *env, void **vm) {
  *vm = fake_vm;
  return 0;
}

void *NewGlobalRef(void *env, void *ptr) {
  return ptr;
}

char *NewStringUTF(void *env, char *bytes) {
  return bytes;
}

char *GetStringUTFChars(void *env, char *string, int *isCopy) {
  return string;
}

void *GetObjectClass(void *env, void *globalRef) {
  return globalRef;
}

bool *NewBooleanArray(void *env, int len) {
  return malloc(sizeof(bool) * len);
}

void *NewObjectV(void *env, void *class, int methodID, va_list args) {
  return class;
}

void *PopLocalFrame(void *env, void *result) {
  return result;
}

int AttachCurrentThread(void *vm, void **env, int r2) {
  *env = fake_env;
  return 0;
}

int GetEnv(void *vm, void **env, int r2) {
  *env = fake_env;
  return 0;
}

size_t GetArrayLength(void *env, jni_byte_array_t *array) {
  return array->size;
}

uint8_t *GetByteArrayElements(void *env, jni_byte_array_t *array, bool *isCopy) {
  return array->data;
}

void *jni_create_env() {
  memset(fake_env, 'A', sizeof(fake_env));
  *(uintptr_t *)(fake_env + 0x00) = (uintptr_t)fake_env; // just point to itself...
  *(uintptr_t *)(fake_env + 0x18) = (uintptr_t)FindClass; // FindClass
  *(uintptr_t *)(fake_env + 0x3C) = (uintptr_t)ret0; // ExceptionOccurred
  *(uintptr_t *)(fake_env + 0x40) = (uintptr_t)ret0; // ExceptionDescribe
  *(uintptr_t *)(fake_env + 0x44) = (uintptr_t)ret0; // ExceptionClear
  *(uintptr_t *)(fake_env + 0x4C) = (uintptr_t)ret0; // PushLocalFrame
  *(uintptr_t *)(fake_env + 0x50) = (uintptr_t)PopLocalFrame;
  *(uintptr_t *)(fake_env + 0x54) = (uintptr_t)NewGlobalRef;
  *(uintptr_t *)(fake_env + 0x58) = (uintptr_t)ret0; // DeleteGlobalRef
  *(uintptr_t *)(fake_env + 0x5C) = (uintptr_t)ret0; // DeleteLocalRef
  *(uintptr_t *)(fake_env + 0x74) = (uintptr_t)NewObjectV;
  *(uintptr_t *)(fake_env + 0x7C) = (uintptr_t)GetObjectClass;
  *(uintptr_t *)(fake_env + 0x84) = (uintptr_t)GetMethodID;
  *(uintptr_t *)(fake_env + 0x88) = (uintptr_t)CallObjectMethod;
  *(uintptr_t *)(fake_env + 0x8C) = (uintptr_t)CallObjectMethodV;
  *(uintptr_t *)(fake_env + 0x98) = (uintptr_t)CallBooleanMethodV;
  *(uintptr_t *)(fake_env + 0xC8) = (uintptr_t)CallIntMethodV;
  *(uintptr_t *)(fake_env + 0xD4) = (uintptr_t)CallLongMethodV;
  *(uintptr_t *)(fake_env + 0xE0) = (uintptr_t)CallFloatMethodV;
  *(uintptr_t *)(fake_env + 0xF8) = (uintptr_t)CallVoidMethodV;
  *(uintptr_t *)(fake_env + 0x1C4) = (uintptr_t)GetStaticMethodID;
  *(uintptr_t *)(fake_env + 0x1CC) = (uintptr_t)CallStaticObjectMethodV;
  *(uintptr_t *)(fake_env + 0x1D8) = (uintptr_t)CallStaticBooleanMethodV;
  *(uintptr_t *)(fake_env + 0x208) = (uintptr_t)CallStaticIntMethodV;
  *(uintptr_t *)(fake_env + 0x238) = (uintptr_t)CallStaticVoidMethodV;
  *(uintptr_t *)(fake_env + 0x29C) = (uintptr_t)NewStringUTF;
  *(uintptr_t *)(fake_env + 0x2A4) = (uintptr_t)GetStringUTFChars;
  *(uintptr_t *)(fake_env + 0x2A8) = (uintptr_t)ret0; // ReleaseStringUTFChars
  *(uintptr_t *)(fake_env + 0x2AC) = (uintptr_t)GetArrayLength;
  *(uintptr_t *)(fake_env + 0x2C0) = (uintptr_t)NewBooleanArray;
  *(uintptr_t *)(fake_env + 0x2E0) = (uintptr_t)GetByteArrayElements;
  *(uintptr_t *)(fake_env + 0x300) = (uintptr_t)ret0; // ReleaseByteArrayElements
  *(uintptr_t *)(fake_env + 0x35C) = (uintptr_t)RegisterNatives;
  *(uintptr_t *)(fake_env + 0x36C) = (uintptr_t)GetJavaVM;
  return fake_env;
}

void *jni_create_vm() {
  memset(fake_vm, 'A', sizeof(fake_vm));
  *(uintptr_t *)(fake_vm + 0x00) = (uintptr_t)fake_vm; // just point to itself...
  *(uintptr_t *)(fake_vm + 0x10) = (uintptr_t)AttachCurrentThread;
  *(uintptr_t *)(fake_vm + 0x14) = (uintptr_t)ret0; // DetachCurrentThread
  *(uintptr_t *)(fake_vm + 0x18) = (uintptr_t)GetEnv;
  return fake_vm;
}

void *jni_get_vm() {
  return fake_vm;
}

void *jni_get_env() {
  return fake_env;
}
