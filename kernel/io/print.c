#include <kernel/io/serial.h>
#include <kernel/io/print.h>

void kprint(const char* str) {
    while (*str) {
        if (*str == '\n') {
            serial_write_char('\r');  // Carriage return for newlines
        }
        serial_write_char(*str++);
    }
}
