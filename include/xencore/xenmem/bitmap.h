#ifndef _BITMAP_H
#define _BITMAP_H

void bitmap_init(void);
void *alloc_page(void);
void free_page(void *page);

#endif
