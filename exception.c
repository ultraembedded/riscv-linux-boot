#include "exception.h"
#include "csr.h"
#include "serial.h"

//-----------------------------------------------------------------
// Locals
//-----------------------------------------------------------------
#define CAUSE_MAX_EXC      (CAUSE_PAGE_FAULT_STORE + 1)
static fp_exception        _exception_table[CAUSE_MAX_EXC];

//-----------------------------------------------------------------
// exception_set_handler: Register exception handler
//-----------------------------------------------------------------
void exception_set_handler(int cause, fp_exception handler)
{
    _exception_table[cause] = handler;
}
//-----------------------------------------------------------------
// exception_handler:
//-----------------------------------------------------------------
struct irq_context * exception_handler(struct irq_context *ctx)
{
    switch (ctx->cause)
    {
        case CAUSE_ECALL_U:
        case CAUSE_ECALL_S:
        case CAUSE_ECALL_M:
            ctx->pc += 4;
            break;
    }

    if (ctx->cause < CAUSE_MAX_EXC && _exception_table[ctx->cause])
        ctx = _exception_table[ctx->cause](ctx);
    else
    {
        serial_putstr("Unhandled exception, stopping...\n");
        _exit(-1);
    }
    return ctx;
}
