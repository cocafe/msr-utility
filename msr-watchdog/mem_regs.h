#pragma once

#include <stdint.h>

typedef struct mem_reg {
	uint64_t addr;
	uint32_t data;
} mem_reg;

typedef struct mem_regs {
	mem_reg		*regs;
	size_t		regs_allocated;
	size_t		regs_count;
} mem_regs;

int mem_regs_init(mem_regs *list);
int mem_regs_deinit(mem_regs *list);

int mem_regs_insert(mem_regs *list, uint64_t addr, uint32_t data);
int mem_regs_dump(mem_regs *list);
