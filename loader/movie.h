#ifndef __MOVIE_H__
#define __MOVIE_H__
#include <vitasdk.h>

extern SceUID movie_sema;

int movie_thread(SceSize argc, void *argp);

#endif
