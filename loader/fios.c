/*
 * fios.c -- use FIOS2 for optimized I/O
 *
 * Copyright (C) 2021 Andy Nguyen
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
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "fios.h"
#include "so_util.h"

#define MAX_PATH_LENGTH 256
#define RAMCACHEBLOCKSIZE (64 * 1024)
#define RAMCACHEBLOCKNUM 1024

static int64_t g_OpStorage[SCE_FIOS_OP_STORAGE_SIZE(64, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
static int64_t g_ChunkStorage[SCE_FIOS_CHUNK_STORAGE_SIZE(1024) / sizeof(int64_t) + 1];
static int64_t g_FHStorage[SCE_FIOS_FH_STORAGE_SIZE(1024, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
static int64_t g_DHStorage[SCE_FIOS_DH_STORAGE_SIZE(32, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];

static SceFiosRamCacheContext g_RamCacheContext = SCE_FIOS_RAM_CACHE_CONTEXT_INITIALIZER;
static char *g_RamCacheWorkBuffer;

int fios_init(void) {
  int res;

  SceFiosParams params = SCE_FIOS_PARAMS_INITIALIZER;
  params.opStorage.pPtr = g_OpStorage;
  params.opStorage.length = sizeof(g_OpStorage);
  params.chunkStorage.pPtr = g_ChunkStorage;
  params.chunkStorage.length = sizeof(g_ChunkStorage);
  params.fhStorage.pPtr = g_FHStorage;
  params.fhStorage.length = sizeof(g_FHStorage);
  params.dhStorage.pPtr = g_DHStorage;
  params.dhStorage.length = sizeof(g_DHStorage);
  params.pathMax = MAX_PATH_LENGTH;

  params.threadAffinity[SCE_FIOS_IO_THREAD] = SCE_KERNEL_CPU_MASK_USER_1;
  params.threadAffinity[SCE_FIOS_CALLBACK_THREAD] = SCE_KERNEL_CPU_MASK_USER_1;
  params.threadAffinity[SCE_FIOS_DECOMPRESSOR_THREAD] = SCE_KERNEL_CPU_MASK_USER_1;

  params.threadPriority[SCE_FIOS_IO_THREAD] = 64;
  params.threadPriority[SCE_FIOS_CALLBACK_THREAD] = 191;
  params.threadPriority[SCE_FIOS_DECOMPRESSOR_THREAD] = 191;

  res = sceFiosInitialize(&params);
  if (res < 0)
    return res;

  g_RamCacheWorkBuffer = memalign(8, RAMCACHEBLOCKNUM * RAMCACHEBLOCKSIZE);
  if (!g_RamCacheWorkBuffer)
    return -1;

  g_RamCacheContext.pPath = DATA_PATH;
  g_RamCacheContext.pWorkBuffer = g_RamCacheWorkBuffer;
  g_RamCacheContext.workBufferSize = RAMCACHEBLOCKNUM * RAMCACHEBLOCKSIZE;
  g_RamCacheContext.blockSize = RAMCACHEBLOCKSIZE;
  res = sceFiosIOFilterAdd(0, sceFiosIOFilterCache, &g_RamCacheContext);
  if (res < 0)
    return res;

  return 0;
}

void fios_terminate(void) {
  sceFiosTerminate();
  free(g_RamCacheWorkBuffer);
}
