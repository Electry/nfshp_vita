/*
 * movie.c -- opening movie playback
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
#include <vita2d.h>

#include <stdlib.h>
#include <malloc.h>

#include "main.h"
#include "movie.h"

SceAvPlayerHandle movie_player;
SceUID movie_sema;
bool av_audio_thread_run = true;

void *av_alloc(void *arg, uint32_t alignment, uint32_t size) {
  return memalign(alignment, size);
}

void av_free(void* arg, void* ptr) {
  free(ptr);
}

void *av_alloc_gpu(void *arg, uint32_t alignment, uint32_t size) {
  void *ptr = NULL;

  if (alignment < 0x40000) {
    alignment = 0x40000;
  }

  size = ALIGN(size, alignment);

  SceKernelAllocMemBlockOpt opt;
  memset(&opt, 0, sizeof(opt));
  opt.size = sizeof(SceKernelAllocMemBlockOpt);
  opt.attr = SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_ALIGNMENT;
  opt.alignment = alignment;

  SceUID uid = sceKernelAllocMemBlock("av_player", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, size, &opt);

  sceKernelGetMemBlockBase(uid, &ptr);
  sceGxmMapMemory(ptr, size, (SceGxmMemoryAttribFlags)(SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE));
  return ptr;
}

void av_free_gpu(void *arg, void *ptr) {
  SceUID uid = sceKernelFindMemBlockByAddr(ptr, 0);
  sceGxmUnmapMemory(ptr);
  sceKernelFreeMemBlock(uid);
}

int movie_audio_thread(SceSize argc, void *argp) {
  SceUID audioPort = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM,
    1024, 48000, SCE_AUDIO_OUT_MODE_STEREO);

  SceAvPlayerFrameInfo frame;
  memset(&frame, 0, sizeof(SceAvPlayerFrameInfo));

  while (sceAvPlayerIsActive(movie_player) && av_audio_thread_run) {
    if (sceAvPlayerGetAudioData(movie_player, &frame)) {
      sceAudioOutOutput(audioPort, frame.pData);
    }
  }

  sceAudioOutReleasePort(audioPort);
  return sceKernelExitDeleteThread(0);
}

int movie_thread(SceSize argc, void *argp) {
  SceAvPlayerInitData player_init;
  memset(&player_init, 0, sizeof(SceAvPlayerInitData));

  player_init.memoryReplacement.allocate = av_alloc;
  player_init.memoryReplacement.deallocate = av_free;
  player_init.memoryReplacement.allocateTexture = av_alloc_gpu;
  player_init.memoryReplacement.deallocateTexture = av_free_gpu;

  //player_init.debugLevel = 3;
  player_init.basePriority = 0xA0;
  player_init.numOutputVideoFrameBuffers = 3;
  player_init.autoStart = true;

  movie_player = sceAvPlayerInit(&player_init);
  if (!movie_player) {
    goto AV_EXIT;
  }

  vita2d_init();
  vita2d_texture *frame_tex = (vita2d_texture *)malloc(sizeof(vita2d_texture));

  sceAvPlayerAddSource(movie_player, MOVIE_PATH);

  // spawn av audio thread
  SceUID audio_thid = sceKernelCreateThread("av_player_audio", (SceKernelThreadEntry)movie_audio_thread,
    96, 0x4000, 0, SCE_KERNEL_CPU_MASK_USER_2, NULL);
  sceKernelStartThread(audio_thid, 0, NULL);

  SceAvPlayerFrameInfo frame;

  SceCtrlData pad;
  memset(&pad, 0, sizeof(SceCtrlData));

  while (sceAvPlayerIsActive(movie_player) && !(pad.buttons & (SCE_CTRL_CIRCLE | SCE_CTRL_CROSS))) {
    sceDisplayWaitVblankStart();

    if (sceAvPlayerGetVideoData(movie_player, &frame)) {
      vita2d_start_drawing();
      vita2d_clear_screen();

      sceGxmTextureInitLinear(
        &frame_tex->gxm_tex,
        frame.pData,
        SCE_GXM_TEXTURE_FORMAT_YVU420P2_CSC1,
        frame.details.video.width,
        frame.details.video.height, 0);

      sceGxmTextureSetMinFilter(&frame_tex->gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);
      sceGxmTextureSetMagFilter(&frame_tex->gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);

      float scale_x = 960.0f / (float)frame.details.video.width;
      float scale_y = 544.0f / (float)frame.details.video.height;
      vita2d_draw_texture_scale(frame_tex, 0, 0, scale_x, scale_y);

      vita2d_end_drawing();
      vita2d_wait_rendering_done();
      vita2d_swap_buffers();
    }

    sceCtrlPeekBufferPositive(0, &pad, 1);
  }

  vita2d_fini();
  vita2d_free_texture(frame_tex);

  av_audio_thread_run = false;
  sceKernelWaitThreadEnd(audio_thid, NULL, NULL);
  sceAvPlayerClose(movie_player);

AV_EXIT:
  sceKernelSignalSema(movie_sema, 1);
  sceKernelExitDeleteThread(0);
  return 0;
}
