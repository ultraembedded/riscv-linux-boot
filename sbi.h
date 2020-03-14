#ifndef SBI_H
#define SBI_H

#include "exception.h"

struct irq_context *sbi_syscall(struct irq_context *ctx);

#endif