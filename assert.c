#include "serial.h"

extern void _exit(int rc); 

//-----------------------------------------------------------------
// assert_handler:
//-----------------------------------------------------------------
void assert_handler(const char * type, const char *reason, const char *file, int line)
{
	serial_putstr("ASSERT: ");
	serial_putstr(type);
	serial_putstr(" ");
	serial_putstr(reason);
	serial_putstr(" ");
	serial_putstr_hex(":0x", line);
    _exit(-1);
}
