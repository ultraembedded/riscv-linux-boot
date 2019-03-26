#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "exception.h"

struct irq_context *handle_syscall(struct irq_context *ctx);

#endif