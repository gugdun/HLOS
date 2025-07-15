#ifndef _XENMAP_H
#define _XENMAP_H

void xenmap_init(void);
void *alloc_page(void);
void free_page(void *page);

#endif
