#ifndef __JNI_H__
#define __JNI_H__

typedef enum {
  CLASS_UNKNOWN = 0,
  CLASS_JAVA_IO_INPUT_STREAM = 0x1000,
  CLASS_ANDROID_ASSET_FILE_DESCRIPTOR,
  CLASS_ANDROID_AUDIO_TRACK,
  CLASS_EA_ACTIVITY_ARGUMENTS,
  CLASS_EAMIO_STORAGE_DIRECTORY,
  CLASS_EA_ACCELEROMETER,
  CLASS_EA_DEVICE_ORIENTATION_HANDLER,
  CLASS_EA_DISPLAY,
  CLASS_EA_MAIN_ACTIVITY,
  CLASS_EA_PHYSICAL_KEYBOARD,
  CLASS_EA_POWER_MANAGER,
  CLASS_EA_SYSTEM,
  CLASS_EA_TOUCH_SURFACE,
  CLASS_EA_VIBRATOR,
  CLASS_EA_VIEW,
  CLASS_MPP_CROSS_ACTIVITY,
  CLASS_MPP_FMOD_PLAYER,
} jni_class_id_t;

typedef enum {
  UNKNOWN = 0,
  INIT,
  // java/io/InputStream
  READ,
  CLOSE,
  SKIP,
  // android/content/res/AssetFileDescriptor
  OPEN,
  OPEN_FD,
  LIST,
  GET_LENGTH,
  CLOSE_2,
  // android/media/AudioTrack
  GET_MIN_BUFFER_SIZE,
  // com/ea/EAActivityArguments/EAActivityArguments
  GET_COMMAND_LINE_ARGUMENTS,
  // com/ea/blast/MainActivity
  GET_DISPLAY_WIDTH,
  GET_DISPLAY_HEIGHT,
  // com/ea/blast/PowerManagerAndroid
  APPLY_KEEP_AWAKE,
  // com/ea/blast/DisplayAndroidDelegate
  GET_DPI_X,
  GET_DPI_Y,
  GET_CURRENT_WIDTH,
  GET_CURRENT_HEIGHT,
  GET_STD_ORIENTATION,
  SET_STD_ORIENTATION,
  GET_GL_VIEW,
  // com/ea/blast/PhysicalKeyboardAndroidDelegate
  IS_PHYSICAL_KEYBOARD_VISIBLE,
  // com/ea/EAMIO/StorageDirectory
  GET_OBB_FILE_PATH,
  GET_INTERNAL_STORAGE_DIRECTORY,
  GET_PRIMARY_EXTERNAL_STORAGE_DIRECTORY_ROOT,
  GET_PRIMARY_EXTERNAL_STORAGE_DIRECTORY,
  GET_PRIMARY_EXTERNAL_STORAGE_STATE,
  // com/ea/blast/DeviceOrientationHandlerAndroidDelegate
  ON_LIFE_CYCLE_FOCUS_GAINED,
  SET_ENABLED,
  // com/ea/blast/SystemAndroidDelegate
  GET_ACCELEROMETER_COUNT,
  GET_TOUCH_SCREEN_COUNT,
  GET_CAMERA_COUNT,
  GET_GYROSCOPE_COUNT,
  GET_MICROPHONE_COUNT,
  GET_VIBRATOR_COUNT,
  GET_APPLICATION_VERSION_CODE,
  GET_APPLICATION_ID,
  GET_APPLICATION_VERSION,
  GET_DEVICE_UNIQUE_ID,
  GET_LOCALE,
  GET_LANGUAGE,
  INTENT_VIEW,
  // com/ea/blast/TouchSurfaceAndroid
  IS_TOUCH_SCREEN_MULTI_TOUCH,
  // com/mpp/android/main/crossActivity/CrossActivity
  IS_ANY_MUSIC_PLAYING,
  // com/mpp/android/fmod/FModPlayer
  INIT_AUDIO_TRACK,
  STOP_AUDIO_TRACK,
} jni_method_id_t;

typedef struct {
  jni_class_id_t class;
  const char *name;
  jni_method_id_t id;
} jni_method_t;

typedef struct {
  const char *name;
  jni_class_id_t id;
} jni_class_t;

typedef struct {
  size_t size;
  uint8_t *data;
} jni_byte_array_t;

void *jni_create_vm(void);
void *jni_create_env(void);

void *jni_get_vm(void);
void *jni_get_env(void);

#endif
