#include <string.h>
#include <stdint.h>
#include <emmintrin.h>

void* memset(void* dest, int value, size_t size)
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
