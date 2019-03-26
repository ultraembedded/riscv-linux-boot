#if __riscv_xlen == 64
# define RISCV_PGLEVEL_BITS 9
#else
# define RISCV_PGLEVEL_BITS 10
#endif
#define RISCV_PGSHIFT 12
#define RISCV_PGSIZE (1 << RISCV_PGSHIFT)

##############################################################
# Payload Binary
##############################################################
  .section ".payload","a",@progbits
  .align RISCV_PGSHIFT + RISCV_PGLEVEL_BITS

  .globl _payload_start, _payload_end
_payload_start:
  .incbin PAYLOAD_BINARY
_payload_end:

##############################################################
# DTB Binary
##############################################################
.section ".dtb","a",@progbits
.align RISCV_PGLEVEL_BITS

.globl _dtb_start, _dtb_end
_dtb_start:
.incbin DTB_BINARY
_dtb_end: