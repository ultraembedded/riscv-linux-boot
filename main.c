#include <stdint.h>
#include "csr.h"
#include "exception.h"
#include "syscalls.h"
#include "serial.h"

//-----------------------------------------------------------------
// Defines:
//-----------------------------------------------------------------
#define SERIAL_BASE        0x92000000

//-----------------------------------------------------------------
// Globals:
//-----------------------------------------------------------------
extern uint32_t _payload_start;
extern uint32_t _dtb_start;
extern uint32_t _sp;

//-----------------------------------------------------------------
// main:
//-----------------------------------------------------------------
int main(void)
{
    uint32_t entry_addr = (uint32_t)&_payload_start;
    uint32_t dtb_addr   = (uint32_t)&_dtb_start;   

    // Setup serial port
    serial_init(SERIAL_BASE, 0 /* NOT USED */);
    serial_putstr("Booting...\n");

    // Register fault handlers
    exception_set_handler(CAUSE_ECALL_S, handle_syscall);

    // Boot kernel
    csr_write(mstatus, PRV_S << MSTATUS_MPP_SHIFT);

    // Configure interrupt / exception delegation
    // Delegate everything except super/machine level syscalls
    csr_write(medeleg, ~(1 << CAUSE_ECALL_S));
    csr_write(mideleg, ~0);

    // Boot target
    csr_write(mepc, entry_addr);

    // Set machine mode exception stack
    csr_write(mscratch, &_sp);

    // Switch from machine mode to supervisor mode
    register uintptr_t a0 asm ("a0") = 0;
    register uintptr_t a1 asm ("a1") = dtb_addr;
    asm volatile ("mret" : : "r" (a0), "r" (a1));

    return 0;
}

