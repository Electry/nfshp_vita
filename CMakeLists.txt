cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(NFSHP)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "NFS Hot Pursuit")
set(VITA_TITLEID  "NFSHP0000")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wl,-q,--wrap,memcpy,--wrap,memmove,--wrap,memset -Wall -O3 -mfloat-abi=softfp")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11")

# ATTRIBUTE2: Memory Expansion Mode: 4 = +29MiB, 8 = +77MiB, 12 = +109MiB
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE2=12")

include_directories()

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(${PROJECT_NAME}
  loader/main.c
  loader/so_util.c
  loader/dynlib.c
  loader/jni.c
  loader/pthread.c
  loader/gl_patch.c
  loader/fios.c
)

target_link_libraries(${PROJECT_NAME}
  stdc++
  m
  kubridge_stub
  SceAppUtil_stub
  SceAudio_stub
  SceCtrl_stub
  SceMotion_stub
  SceFios2_stub
  SceLibcBridge_stub
  ScePower_stub
  SceTouch_stub
  SceVshBridge_stub
  libGLESv1_CM_stub_weak
  libIMGEGL_stub_weak
  libgpu_es4_ext_stub_weak
)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}

  FILE ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml

  FILE ${CMAKE_SOURCE_DIR}/prx/libgpu_es4_ext.suprx libgpu_es4_ext.suprx
  FILE ${CMAKE_SOURCE_DIR}/prx/libIMGEGL.suprx libIMGEGL.suprx
  FILE ${CMAKE_SOURCE_DIR}/prx/libGLESv1_CM.suprx module/libGLESv1_CM.suprx
  FILE ${CMAKE_SOURCE_DIR}/prx/libpvrPSP2_WSEGL.suprx module/libpvrPSP2_WSEGL.suprx
)
