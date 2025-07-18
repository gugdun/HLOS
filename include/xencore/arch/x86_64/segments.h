#ifndef _SEGMENTS_H
#define _SEGMENTS_H

#define KERNEL_CS      ((GDT_KERNEL_CODE << 3) | 0)
#define KERNEL_DS      ((GDT_KERNEL_DATA << 3) | 0)
#define USER_CS        ((GDT_USER_CODE   << 3) | 3)
#define USER_DS        ((GDT_USER_DATA   << 3) | 3)
#define TSS_SELECTOR   ((GDT_TSS_LOW     << 3) | 0)

#endif
