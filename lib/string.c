#include <string.h>
#include <stdint.h>

void *lib_memset(void *ptr, int value, size_t num) {
    unsigned char *p = (unsigned char *)ptr;
    unsigned char val = (unsigned char)value;

    for (size_t i = 0; i < num; ++i) {
        p[i] = val;
    }

    return ptr;
}

void *lib_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    for (size_t i = 0; i < n; ++i) {
        d[i] = s[i];
    }

    return dest;
}

void *lib_memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d == s || n == 0) {
        return dest;
    }

    if (d < s) {
        // Copy forwards
        for (size_t i = 0; i < n; ++i) {
            d[i] = s[i];
        }
    } else {
        // Copy backwards to avoid overwrite when regions overlap
        for (size_t i = n; i > 0; --i) {
            d[i - 1] = s[i - 1];
        }
    }

    return dest;
}
