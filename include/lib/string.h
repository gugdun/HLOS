#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

void* lib_memset(void *s, int c, size_t n);
void *lib_memcpy(void *dst, const void *src, size_t n);
void *lib_memmove(void *dst, const void *src, size_t n);

#endif
