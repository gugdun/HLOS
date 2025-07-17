#ifndef _XENALLOC_H
#define _XENALLOC_H

#include <stddef.h>

void *xen_alloc_aligned(size_t size);
void *xen_alloc(size_t size);
void xen_free(void *ptr);

#endif
