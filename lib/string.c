#include <string.h>
#include <stdint.h>
#include <emmintrin.h>

void *memset(void* dest, int value, size_t size)
{
    uint8_t* ptr = (uint8_t*)dest;
    size_t i = 0;

    // Set up 16-byte vector with the repeated byte value
    __m128i val = _mm_set1_epi8((char)value);

    // Align destination to 16 bytes
    while (((uintptr_t)(ptr + i) & 0xF) && i < size) {
        ptr[i++] = (uint8_t)value;
    }

    // Bulk fill using 128-bit stores
    for (; i + 16 <= size; i += 16) {
        _mm_store_si128((__m128i*)(ptr + i), val);
    }

    // Tail bytes
    for (; i < size; ++i) {
        ptr[i] = (uint8_t)value;
    }

    return dest;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    // Align to 16-byte boundaries
    while (((uintptr_t)d % 16 != 0) && n) {
        *d++ = *s++;
        --n;
    }

    // Copy 16 bytes at a time using SSE
    while (n >= 16) {
        __m128i reg = _mm_loadu_si128((__m128i *)s);
        _mm_storeu_si128((__m128i *)d, reg);
        s += 16;
        d += 16;
        n -= 16;
    }

    // Copy any remaining bytes
    while (n--) {
        *d++ = *s++;
    }

    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    if (d == s || n == 0) return dst;
    if (d < s) {
        // Forward copy (no overlap or safe)
        // Align to 16-byte boundaries
        while (((uintptr_t)d % 16 != 0) && n) {
            *d++ = *s++;
            --n;
        }
        // Copy 16 bytes at a time using SSE
        while (n >= 16) {
            __m128i reg = _mm_loadu_si128((__m128i *)s);
            _mm_storeu_si128((__m128i *)d, reg);
            s += 16;
            d += 16;
            n -= 16;
        }
        // Copy any remaining bytes
        while (n--) {
            *d++ = *s++;
        }
    } else {
        // Backward copy (overlap)
        d += n;
        s += n;
        // Align to 16-byte boundaries
        while (n && ((uintptr_t)d % 16 != 0)) {
            *(--d) = *(--s);
            --n;
        }
        // Copy 16 bytes at a time using SSE
        while (n >= 16) {
            d -= 16;
            s -= 16;
            __m128i reg = _mm_loadu_si128((__m128i *)s);
            _mm_storeu_si128((__m128i *)d, reg);
            n -= 16;
        }
        // Copy any remaining bytes
        while (n--) {
            *(--d) = *(--s);
        }
    }
    return dst;
}
