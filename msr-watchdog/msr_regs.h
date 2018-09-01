#pragma once

#include <stdint.h>
#include <Windows.h>

#define CPU0				(0)

enum {
	NOT_REG = 0,
	REG_GENERAL,
	REG_MAILBOX,
};

typedef struct msr_reg {
	uint32_t	proc;
	DWORD		reg;
	DWORD		eax;	// low 32bit
	DWORD		edx;	// high 32bit
	int		watch;
} msr_reg;

typedef struct msr_mailbox {
	DWORD	reg;

	DWORD	eax_get;
	DWORD	edx_get;

	DWORD	eax_set;
	DWORD	edx_set;

	DWORD	eax_ret;
	DWORD	edx_ret;

	int	watch;
} msr_mb;

typedef struct msr_reg_list {
	msr_reg	*gen_regs;
	size_t	gen_regs_allocated;
	size_t	gen_regs_count;

	msr_mb	*mb_regs;
	size_t	mb_regs_allocated;
	size_t	mb_regs_count;
} msr_regs;

int msr_regs_init(msr_regs *list);
int msr_regs_deinit(msr_regs *list);

int msr_gen_reg_insert(msr_regs *regs,
		       uint32_t proc,
		       DWORD reg,
		       DWORD eax,
		       DWORD edx,
		       int watch);
int msr_mb_reg_create(msr_regs *regs, DWORD reg, int watch);
int msr_mb_reg_fill(msr_regs *regs,
		    DWORD reg,
		    DWORD *eax_get,
		    DWORD *edx_get,
		    DWORD *eax_set,
		    DWORD *edx_set,
		    DWORD *eax_ret,
		    DWORD *edx_ret);
int msr_regs_dump(msr_regs *regs);
