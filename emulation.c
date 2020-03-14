#include "serial.h"
#include "csr.h"
#include "exception.h"
#include "assert.h"
#include "emulation.h"

extern void isr_vector(void);

//-----------------------------------------------------------------
// Defines:
//-----------------------------------------------------------------
#define max(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })


#define min(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })

//-----------------------------------------------------------------
// Locals
//-----------------------------------------------------------------
static uint32_t load_reservation = 0;

//-----------------------------------------------------------------
// emulation_read_word: Read word with fault catching
//-----------------------------------------------------------------
static int32_t emulation_read_word(uint32_t address, int32_t *data)
{
    int32_t result, tmp;
    int32_t fail;
    __asm__ __volatile__ (
        "   li       %[tmp],  0x00020000\n" \
        "   csrs     mstatus,  %[tmp]\n" \
        "   la       %[tmp],  1f\n" \
        "   csrw     mtvec,  %[tmp]\n" \
        "   li       %[fail], 1\n" \
        "   lw       %[result], 0(%[address])\n"
        "   li       %[fail], 0\n" \
        "1:\n" \
        "   li       %[tmp],  0x00020000\n" \
        "   csrc     mstatus,  %[tmp]\n" \
        : [result]"=&r" (result), [fail]"=&r" (fail), [tmp]"=&r" (tmp)
        : [address]"r" (address)
        : "memory"
    );

    *data = result;
    return fail;
}
//-----------------------------------------------------------------
// emulation_write_word: Write word with fault catching
//-----------------------------------------------------------------
static int32_t emulation_write_word(uint32_t address, int32_t data)
{
    int32_t tmp;
    int32_t fail;
    __asm__ __volatile__ (
        "   li       %[tmp],  0x00020000\n" \
        "   csrs     mstatus,  %[tmp]\n" \
        "   la       %[tmp],  1f\n" \
        "   csrw     mtvec,  %[tmp]\n" \
        "   li       %[fail], 1\n" \
        "   sw       %[data], 0(%[address])\n"
        "   li       %[fail], 0\n" \
        "1:\n" \
        "   li       %[tmp],  0x00020000\n" \
        "   csrc     mstatus,  %[tmp]\n" \
        : [fail]"=&r" (fail), [tmp]"=&r" (tmp)
        : [address]"r" (address), [data]"r" (data)
        : "memory"
    );
    return fail;
}
//-----------------------------------------------------------------
// emulation_trap_to_supervisor: Divert fault to supervisor mode
//-----------------------------------------------------------------
static void emulation_trap_to_supervisor(struct irq_context *ctx, uint32_t sepc, uint32_t mstatus)
{
    csr_write(mtvec,    isr_vector);
    csr_write(0x143,    csr_read(0x343)); // mbadaddr/mtval -> sbadaddr/stval 
    csr_write(scause,   csr_read(mcause));
    csr_write(sepc,     sepc);

    // Return to supervisor trap address
    ctx->pc     = csr_read(stvec);
    ctx->status = (mstatus & ~(MSTATUS_SPP | MSTATUS_MPP | MSTATUS_SIE | MSTATUS_SPIE))
                  | ((mstatus >> 3) & MSTATUS_SPP)
                  | (0x0800 | MSTATUS_MPIE)
                  | ((mstatus & MSTATUS_SIE) << 4);
}
//-----------------------------------------------------------------
// trap_invalid_inst: Invalid instruction handler
//-----------------------------------------------------------------
static struct irq_context *trap_invalid_inst(struct irq_context *ctx)
{
    uint32_t mepc    = ctx->pc;
    uint32_t mstatus = ctx->status;
    uint32_t instr   = csr_read(0x343); // mbadaddr or mtval

    uint32_t opcode = instr & 0x7f;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t sel    = (instr >> 27);
    uint32_t rd     = (instr >> 7)  & 0x1f;
    uint32_t rs1    = (instr >> 15) & 0x1f;
    uint32_t rs2    = (instr >> 20) & 0x1f;

    // LR
    if (opcode == 0x2f && funct3 == 0x2 && sel == 0x2)
    {
        uint32_t addr       = ctx->reg[rs1];
        int32_t  data_read  = 0;

        // Load
        if (emulation_read_word(addr, &data_read))
        {
            // Load fault - stop and redirect to supervisor
            emulation_trap_to_supervisor(ctx, mepc, mstatus);
            return ctx;
        }

        // TODO: This should be on the physical address or take into account ctx switches...
        load_reservation = addr;

        ctx->reg[rd] = data_read;
    }
    // SC
    else if (opcode == 0x2f && funct3 == 0x2 && sel == 0x3)
    {
        uint32_t addr        = (rs1 != 0) ? ctx->reg[rs1] : 0;
        int32_t  data_write  = (rs2 != 0) ? ctx->reg[rs2] : 0;

        if (load_reservation == addr)
        {
            // Store
            if (emulation_write_word(addr, data_write))
            {
                // Store fault - stop and redirect to supervisor
                emulation_trap_to_supervisor(ctx, mepc, mstatus);
                return ctx;
            }

            load_reservation = 0;
            ctx->reg[rd] = 0;
        }
        else
            ctx->reg[rd] = 1;
    }
    // Atomics
    else if (opcode == 0x2f)
    {
        switch(funct3)
        {
            case 0x2:
            {
                uint32_t addr = (rs1 != 0) ? ctx->reg[rs1] : 0;
                int32_t  src  = (rs2 != 0) ? ctx->reg[rs2] : 0;
                int32_t  data_read  = 0;
                int32_t  data_write = 0;

                // Load
                if (emulation_read_word(addr, &data_read))
                {
                    // Load fault - stop and redirect to supervisor
                    emulation_trap_to_supervisor(ctx, mepc, mstatus);
                    return ctx;
                }

                switch(sel)
                {
                    case 0x0:  data_write = src + data_read; break; // amoadd.w
                    case 0x1:  data_write = src;             break; // amoswap.w
                    case 0x4:  data_write = src ^ data_read; break; // amoxor.w
                    case 0xC:  data_write = src & data_read; break; // amoand.w
                    case 0x8:  data_write = src | data_read; break; // amoor.w
                    case 0x10: data_write = min((int32_t)src, (int32_t)data_read);   break; // amomin.w
                    case 0x14: data_write = max((int32_t)src, (int32_t)data_read);   break; // amomax.w
                    case 0x18: data_write = min((uint32_t)src, (uint32_t)data_read); break; // amominu.w
                    case 0x1C: data_write = max((uint32_t)src, (uint32_t)data_read); break; // amomaxu.w
                    default: assert(!"error"); break;
                }

                // Store
                if (emulation_write_word(addr, data_write))
                {
                    // Store fault - stop and redirect to supervisor
                    emulation_trap_to_supervisor(ctx, mepc, mstatus);
                    return ctx;
                }
                ctx->reg[rd] = data_read;
            } break;
            default: assert(!"error"); break;
        } 
    }
    else
    {
        serial_putstr_hex("ERROR: Invalid opcode: ", instr);
        serial_putstr_hex("       at PC: ", ctx->pc);
        assert(!"error"); 
    }

    // Skip faulting instruction
    ctx->pc += 4;

    // Force MTVEC back to default handler
    csr_write(mtvec, isr_vector);
    return ctx;
}
//-----------------------------------------------------------------
// emulation_init: Configure emulation
//-----------------------------------------------------------------
void emulation_init(void)
{
    exception_set_handler(CAUSE_ILLEGAL_INSTRUCTION, trap_invalid_inst);
}
//-----------------------------------------------------------------
// emulation_take_irq: On interrupt, clear load reservation
//-----------------------------------------------------------------
void emulation_take_irq(void)
{
    load_reservation = 0;
}
