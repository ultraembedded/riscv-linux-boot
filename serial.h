#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <stdint.h>

//-----------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------
void serial_init(uint32_t base_addr, uint32_t baud_rate);
int  serial_putchar(char c);
void serial_putstr(const char *str);
int  serial_haschar(void);
int  serial_getchar(void);

#endif
