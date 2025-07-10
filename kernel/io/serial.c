#include <stdarg.h>

#include <kernel/io/serial.h>
#include <kernel/io/ports.h>

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00); // Disable interrupts
    outb(COM1_PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03); // Set divisor to 3 (38400 baud)
    outb(COM1_PORT + 1, 0x00); //                  (high byte)
    outb(COM1_PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set
    serial_printf("[Serial] Initialized!\n");
}

int serial_is_transmit_ready() {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_print_char(char c) {
    while (!serial_is_transmit_ready()) {}
    if (c == '\n') outb(COM1_PORT, '\r');
    outb(COM1_PORT, c);
}

void serial_print_str(const char* s)
{
    while (*s) serial_print_char(*s++);
}

void serial_print_uint(unsigned long long num, int base)
{
    char buffer[32];
    const char* digits = "0123456789ABCDEF";
    int i = 0;

    if (num == 0) {
        serial_print_char('0');
        return;
    }

    while (num && i < (int)sizeof(buffer)) {
        buffer[i++] = digits[num % base];
        num /= base;
    }

    while (--i >= 0)
        serial_print_char(buffer[i]);
}

void serial_print_int(long long num)
{
    if (num < 0) {
        serial_print_char('-');
        num = -num;
    }
    serial_print_uint((unsigned long long)num, 10);
}

void serial_print_float(double value, int precision)
{
    if (value < 0) {
        serial_print_char('-');
        value = -value;
    }

    long long int_part = (long long)value;
    double frac_part = value - int_part;

    serial_print_int(int_part);
    serial_print_char('.');

    for (int i = 0; i < precision; ++i)
        frac_part *= 10;

    serial_print_uint((unsigned long long)frac_part, 10);
}

void serial_printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            serial_print_char(*fmt++);
            continue;
        }

        ++fmt;
        switch (*fmt) {
            case 'd':
            case 'i':
                serial_print_int(va_arg(args, long long));
                break;
            case 'u':
                serial_print_uint(va_arg(args, unsigned long long), 10);
                break;
            case 'x':
                serial_print_uint(va_arg(args, unsigned long long), 16);
                break;
            case 'f':
                serial_print_float(va_arg(args, double), 6);
                break;
            case 's': {
                const char* str = va_arg(args, const char*);
                serial_print_str(str ? str : "(null)");
                break;
            }
            case 'c':
                serial_print_char((char)va_arg(args, int));
                break;
            case '%':
                serial_print_char('%');
                break;
            default:
                serial_print_char('%');
                serial_print_char(*fmt);
                break;
        }
        ++fmt;
    }

    va_end(args);
}
