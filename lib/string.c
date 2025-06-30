#include <string.h>

void memset(void *s, int c, size_t n)
{
    for (int i = 0; (size_t)i < n; ++i) {
        ((char *)s)[i] = (char)c;
    }
}
