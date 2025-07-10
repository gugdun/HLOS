#ifndef _SERIAL_H
#define _SERIAL_H

#define COM1_PORT 0x3F8  // Standard COM1

void serial_init(void);
int  serial_is_transmit_ready();
void serial_print_char(char c);
void serial_print_str(const char* s);
void serial_print_uint(unsigned long long num, int base);
void serial_print_int(long long num);
void serial_print_float(double value, int precision);
void serial_printf(const char* fmt, ...);

#endif
