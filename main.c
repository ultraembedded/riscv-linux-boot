#include "csr.h"
#include "exception.h"
#include "sbi.h"
#include "emulation.h"
#include "serial.h"
#include "assert.h"

#ifndef CONFIG_UARTLITE_BASE
#define CONFIG_UARTLITE_BASE 0x92000000
#endif

//-----------------------------------------------------------------
// Defines:
//-----------------------------------------------------------------
#define MSTATUS_MPP_SHIFT   11
#define PRV_S 1
#define PRV_M 3

//-----------------------------------------------------------------
// Defines:
//-----------------------------------------------------------------
extern uint32_t _sp;

#ifdef CONFIG_KERNEL_EMBEDDED
extern uint32_t _payload_start;
extern uint32_t _dtb_start;
#endif

//-----------------------------------------------------------------
// flash_memcpy: Word copy (rounded up)
//-----------------------------------------------------------------
static void flash_memcpy(void *dst, void *src, int length)
{
    int words = (length + 3) / 4;
    uint32_t *pSrc = (uint32_t*)src;
    uint32_t *pDst = (uint32_t*)dst;

    assert(((uint32_t)pSrc & 3) == 0);
    assert(((uint32_t)pDst & 3) == 0);

    while (words--)
        *pDst++ = *pSrc++;
}
//-----------------------------------------------------------------
// irqctrl_handler: Interrupt handler
//-----------------------------------------------------------------
static struct irq_context * irq_callback(struct irq_context *ctx)
{
    uint32_t cause = ctx->cause & 0xF;

    if (cause == IRQ_M_TIMER)
    {
        emulation_take_irq();

        // Raise to supervisor
        csr_set(sip, SR_IP_STIP);
        csr_clear(mie, SR_IP_MTIP);
        csr_clear(mip, SR_IP_MTIP);
    }
    else
    {
        serial_putstr_hex("ERROR: Unhandled IRQ: ", cause);
        _exit(-1);
    }
    return ctx;
}
//-----------------------------------------------------------------
// boot_kernel:
//-----------------------------------------------------------------
static int boot_kernel(uint32_t entry_addr, uint32_t dtb_addr)
{
    // Register fault handlers
    exception_set_handler(CAUSE_ECALL_S, sbi_syscall);

    // Register interrupt handler
    exception_set_irq_handler(irq_callback);

    // Enable timer IRQ (on return from exception)
    csr_write(mie, 1 << IRQ_M_TIMER);
    csr_write(mstatus, (PRV_S << MSTATUS_MPP_SHIFT) | SR_MPIE);

    // Configure interrupt / exception delegation
    // Delegate everything except super/machine level syscalls
    // and machine timer IRQ
    csr_write(medeleg, ~((1 << CAUSE_ECALL_S) | (1 << CAUSE_ILLEGAL_INSTRUCTION)));
    csr_write(mideleg, ~(1 << IRQ_M_TIMER));

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
//-----------------------------------------------------------------
// main:
//-----------------------------------------------------------------
int main(void)
{
    serial_init(CONFIG_UARTLITE_BASE, 0);

    serial_putstr("\n");
    serial_putstr(" _____  _____  _____  _____   __      __  _      _                    ____              _   \n");
    serial_putstr("|  __ \\|_   _|/ ____|/ ____|  \\ \\    / / | |    (_)                  |  _ \\            | |  \n");
    serial_putstr("| |__) | | | | (___ | |   _____\\ \\  / /  | |     _ _ __  _   ___  __ | |_) | ___   ___ | |_ \n");
    serial_putstr("|  _  /  | |  \\___ \\| |  |______\\ \\/ /   | |    | | '_ \\| | | \\ \\/ / |  _ < / _ \\ / _ \\| __|\n");
    serial_putstr("| | \\ \\ _| |_ ____) | |____      \\  /    | |____| | | | | |_| |>  <  | |_) | (_) | (_) | |_ \n");
    serial_putstr("|_|  \\_\\_____|_____/ \\_____|      \\/     |______|_|_| |_|\\__,_/_/\\_\\ |____/ \\___/ \\___/ \\__|\n");
    serial_putstr("\n");

    emulation_init();

#ifdef CONFIG_KERNEL_EMBEDDED
    boot_kernel((uint32_t)&_payload_start, (uint32_t)&_dtb_start);
#else
    serial_putstr("Copying DTB from FLASH to RAM...\n");
    flash_memcpy(CONFIG_DTB_DST, CONFIG_DTB_SRC, CONFIG_DTB_SIZE);
    serial_putstr("Copying KERNEL from FLASH to RAM...\n");
    flash_memcpy(CONFIG_KERNEL_DST, CONFIG_KERNEL_SRC, CONFIG_KERNEL_SIZE);
    serial_putstr("Booting...\n");
    boot_kernel(CONFIG_KERNEL_DST, CONFIG_DTB_DST);
#endif
    return 0;
} 
