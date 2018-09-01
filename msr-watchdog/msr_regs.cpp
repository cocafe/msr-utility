#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <Windows.h>

#include "msr_regs.h"
#include "ini_config.h"

#define MSR_REGS_LIST_COUNT_INIT		(10)

int msr_regs_init(msr_regs *list)
{
	if (!list)
		return -EINVAL;

	list->gen_regs = (msr_reg *)calloc(MSR_REGS_LIST_COUNT_INIT, sizeof(msr_reg));
	if (!list->gen_regs)
		return -ENOMEM;
	list->gen_regs_allocated = MSR_REGS_LIST_COUNT_INIT;
	list->gen_regs_count = 0;

	list->mb_regs = (msr_mb *)calloc(MSR_REGS_LIST_COUNT_INIT, sizeof(msr_mb));
	if (!list->mb_regs)
		return -ENOMEM;
	list->mb_regs_allocated = MSR_REGS_LIST_COUNT_INIT;
	list->mb_regs_count = 0;

	return 0;
}

int msr_regs_deinit(msr_regs *list)
{
	if (!list)
		return -EINVAL;

	if (list->gen_regs)
		free(list->gen_regs);

	if (list->mb_regs)
		free(list->mb_regs);

	ZeroMemory(list, sizeof(msr_regs));

	return 0;
}

int msr_regs_is_full(msr_regs *list)
{
	if (list->gen_regs_count >= list->gen_regs_allocated
	    || list->mb_regs_count >= list->mb_regs_allocated)
		return 1;
	else
		return 0;
}

int msr_regs_expand(msr_regs *list, int gen_regs, int mb_regs)
{
	if (!list)
		return -EINVAL;

	if (gen_regs) {
		msr_reg *t;
		size_t t_size;

		t_size = list->gen_regs_allocated + MSR_REGS_LIST_COUNT_INIT;
		t = (msr_reg *)calloc(t_size, sizeof(msr_reg));
		if (!t)
			return -ENOMEM;

		ZeroMemory(t, t_size * sizeof(msr_reg));
		CopyMemory(t, list->gen_regs, list->gen_regs_allocated * sizeof(msr_reg));

		free(list->gen_regs);
		list->gen_regs = t;
		list->gen_regs_allocated = t_size;
	}

	if (mb_regs) {
		msr_mb *t;
		size_t t_size;

		t_size = list->mb_regs_allocated + MSR_REGS_LIST_COUNT_INIT;
		t = (msr_mb *)calloc(t_size, sizeof(msr_mb));
		if (!t)
			return -ENOMEM;

		ZeroMemory(t, t_size * sizeof(msr_mb));
		CopyMemory(t, list->mb_regs, list->mb_regs_allocated * sizeof(msr_mb));

		free(list->mb_regs);
		list->mb_regs = t;
		list->mb_regs_allocated = t_size;
	}

	return 0;
}

int msr_gen_reg_insert(msr_regs *regs,
		       uint32_t proc,
		       DWORD reg, 
		       DWORD eax,
		       DWORD edx,
		       int watch)
{
	if (msr_regs_is_full(regs))
		if (msr_regs_expand(regs, REG_GENERAL, 0))
			return -ENOMEM;

	regs->gen_regs[regs->gen_regs_count].proc = proc;
	regs->gen_regs[regs->gen_regs_count].reg = reg;
	regs->gen_regs[regs->gen_regs_count].eax = eax;
	regs->gen_regs[regs->gen_regs_count].edx = edx;
	regs->gen_regs[regs->gen_regs_count].watch = 1;
	regs->gen_regs_count++;

	return 0;
}

int msr_mb_reg_create(msr_regs *regs, DWORD reg, int watch)
{
	if (msr_regs_is_full(regs))
		if (msr_regs_expand(regs, 0, REG_MAILBOX))
			return -ENOMEM;

	regs->mb_regs[regs->mb_regs_count].reg = reg;
	regs->mb_regs[regs->mb_regs_count].watch = 1;
	regs->mb_regs_count++;

	return 0;
}

int msr_mb_reg_fill(msr_regs *regs,
		    DWORD reg,
		    DWORD *eax_get,
		    DWORD *edx_get,
		    DWORD *eax_set,
		    DWORD *edx_set,
		    DWORD *eax_ret,
		    DWORD *edx_ret)
{
	size_t i;

	for (i = 0; i < regs->mb_regs_count; i++)
	{
		if (regs->mb_regs[i].reg == reg) {
			if (eax_get)
				regs->mb_regs[i].eax_get = *eax_get;
			if (edx_get)
				regs->mb_regs[i].edx_get = *edx_get;
			if (eax_set)
				regs->mb_regs[i].eax_set = *eax_set;
			if (edx_set)
				regs->mb_regs[i].edx_set = *edx_set;
			if (eax_ret)
				regs->mb_regs[i].eax_ret = *eax_ret;
			if (edx_ret)
				regs->mb_regs[i].edx_ret = *edx_ret;

			return 0;
		}
	}

	if (i >= regs->mb_regs_count)
		return -ENOENT;

	return -EFAULT;
}

int msr_regs_dump(msr_regs *regs)
{
	if (!regs)
		return -EINVAL;

	printf_s("%s:\n", __func__);
	printf_s("\nMSR General register:\n");

	for (size_t i = 0; i < regs->gen_regs_count; i++)
	{
		printf_s("%zd CPU%2u reg: %#010x edx: %#010x eax: %#010x\n", 
		       i, 
		       regs->gen_regs[i].proc,
		       regs->gen_regs[i].reg, 
		       regs->gen_regs[i].edx, 
		       regs->gen_regs[i].eax);
	}

	printf_s("\nMSR Mailbox regsiter:\n");

	for (size_t i = 0; i < regs->mb_regs_count; i++)
	{
		printf_s("%zd reg: %#010x\n", i, regs->mb_regs[i].reg);
		printf_s("\tedx_get: %#010x eax_get: %#010x\n", 
		       regs->mb_regs[i].edx_get, 
		       regs->mb_regs[i].eax_get);
		printf_s("\tedx_set: %#010x eax_set: %#010x\n",
		       regs->mb_regs[i].edx_set,
		       regs->mb_regs[i].eax_set);
		printf_s("\tedx_ret: %#010x eax_ret: %#010x\n",
		       regs->mb_regs[i].edx_ret,
		       regs->mb_regs[i].eax_ret);
	}

	return 0;
}
