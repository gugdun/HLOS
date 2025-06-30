#ifndef _SERIAL_H
#define _SERIAL_H

#define COM1_PORT 0x3F8  // Standard COM1

void serial_init(void);
int serial_is_transmit_ready();
void serial_write_char(char c);

#endif
