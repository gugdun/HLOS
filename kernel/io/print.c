#include <stdarg.h>
#include <kernel/io/serial.h>
#include <kernel/io/print.h>

void print_char(char c)
{
    if (c == '\n') serial_write_char('\r');
    serial_write_char(c);
}

void print_str(const char* s)
{
    while (*s) print_char(*s++);
}

void print_uint(unsigned long long num, int base)
{
    char buffer[32];
    const char* digits = "0123456789abcdef";
    int i = 0;

    if (num == 0) {
        print_char('0');
        return;
    }

    while (num && i < (int)sizeof(buffer)) {
        buffer[i++] = digits[num % base];
        num /= base;
    }

    while (--i >= 0)
        print_char(buffer[i]);
}

void print_int(long long num)
{
    if (num < 0) {
        print_char('-');
        num = -num;
    }
    print_uint((unsigned long long)num, 10);
}

void print_float(double value, int precision)
{
    if (value < 0) {
        print_char('-');
        value = -value;
    }

    long long int_part = (long long)value;
    double frac_part = value - int_part;

    print_int(int_part);
    print_char('.');

    for (int i = 0; i < precision; ++i)
        frac_part *= 10;

    print_uint((unsigned long long)frac_part, 10);
}

void kprintf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            print_char(*fmt++);
            continue;
        }

        ++fmt;
        switch (*fmt) {
            case 'd':
            case 'i':
                print_int(va_arg(args, int));
                break;
            case 'u':
                print_uint(va_arg(args, unsigned int), 10);
                break;
            case 'x':
                print_uint(va_arg(args, unsigned int), 16);
                break;
            case 'f':
                print_float(va_arg(args, double), 6);
                break;
            case 's': {
                const char* str = va_arg(args, const char*);
                print_str(str ? str : "(null)");
                break;
            }
            case 'c':
                print_char((char)va_arg(args, int));
                break;
            case '%':
                print_char('%');
                break;
            default:
                print_char('%');
                print_char(*fmt);
                break;
        }
        ++fmt;
    }

    va_end(args);
}
