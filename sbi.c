#include <stdint.h>
#include <stdbool.h>
#include "csr.h"
#include "serial.h"
#include "sbi.h"
#include "exception.h"

//--------------------------------------------------------------------
// SYSCALLs
//--------------------------------------------------------------------
#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI 3
#define SBI_SEND_IPI 4
#define SBI_REMOTE_FENCE_I 5
#define SBI_REMOTE_SFENCE_VMA 6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN 8

//--------------------------------------------------------------------
// set_mtimecmp: Non-std mtimecmp support
//--------------------------------------------------------------------
static void set_mtimecmp(uint32_t next)
{
    csr_write(0x7c0, next);
}
//--------------------------------------------------------------------
// sbi_syscall:
//--------------------------------------------------------------------
struct irq_context *sbi_syscall(struct irq_context *ctx)
{
    uint32_t a0    = ctx->reg[REG_ARG0 + 0];
    uint32_t a1    = ctx->reg[REG_ARG0 + 1];
    uint32_t a2    = ctx->reg[REG_ARG0 + 2];
    uint32_t which = ctx->reg[REG_ARG0 + 7];    

    switch (which)
    {
        case SBI_SHUTDOWN:
            serial_putstr("Shutdown...\n");
            _exit(0);
            break;
        case SBI_CONSOLE_PUTCHAR:
            serial_putchar(a0);
            break;
        case SBI_CONSOLE_GETCHAR:
            if (serial_haschar())
                ctx->reg[REG_ARG0] = serial_getchar();
            else
                ctx->reg[REG_ARG0] = -1;
            break;
        case SBI_SET_TIMER:
            set_mtimecmp(a0);
            csr_set(mie, SR_IP_MTIP);
            csr_clear(sip, SR_IP_STIP);
        break;
        case SBI_REMOTE_FENCE_I:
        case SBI_REMOTE_SFENCE_VMA:
            break;
        default:
            serial_putstr("SBI: Unhandled SYSCALL, stopping...\n");
            _exit(-1);
            break;
    }

    return ctx;
}

