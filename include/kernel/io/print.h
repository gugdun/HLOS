#ifndef _PRINT_H
#define _PRINT_H

void print_char(char c);
void print_str(const char* s);
void print_uint(unsigned long long num, int base);
void print_int(long long num);
void print_float(double value, int precision);
void kprintf(const char* fmt, ...);

#endif
