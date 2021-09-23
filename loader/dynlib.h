#ifndef __DYNLIB_H__
#define __DYNLIB_H__
#include "so_util.h"

#define CLOCK_MONOTONIC 0

typedef struct {
  unsigned long long  st_dev;
  unsigned char       __pad0[4];
  unsigned long       __st_ino;
  unsigned int        st_mode;
  unsigned int        st_nlink;
  unsigned long       st_uid;
  unsigned long       st_gid;
  unsigned long long  st_rdev;
  unsigned char       __pad3[4];
  long long           st_size;
  unsigned long	      st_blksize;
  unsigned long long  st_blocks;
  unsigned long       st_atime;
  unsigned long       st_atime_nsec;
  unsigned long       st_mtime;
  unsigned long       st_mtime_nsec;
  unsigned long       st_ctime;
  unsigned long       st_ctime_nsec;
  unsigned long long  st_ino;
} stat64_t;

FILE *sceLibcBridge_fopen(const char *filename, const char *mode);
int sceLibcBridge_fclose(FILE *stream);
size_t sceLibcBridge_fread(void *ptr, size_t size, size_t count, FILE *stream);
int sceLibcBridge_fseek(FILE *stream, long int offset, int origin);
long int sceLibcBridge_ftell(FILE *stream);
size_t sceLibcBridge_fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
int sceLibcBridge_ferror(FILE *stream);
int sceLibcBridge_feof(FILE *stream);
int sceLibcBridge_fflush(FILE *stream);
int sceLibcBridge_fputs(const char *s, FILE *stream);

extern so_default_dynlib default_dynlib[];
extern int numfunc_default_dynlib;

#endif
