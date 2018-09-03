#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <memory.h>

#include <Windows.h>

#include "mem_regs.h"

#define MEM_REGS_LIST_COUNT_INTT		(10)

int mem_regs_init(mem_regs *list)
{
	if (!list)
		return -EINVAL;

	list->regs = (mem_reg *)calloc(MEM_REGS_LIST_COUNT_INTT, sizeof(mem_reg));
	if (!list->regs)
		return -ENOMEM;
	list->regs_allocated = MEM_REGS_LIST_COUNT_INTT;
	list->regs_count = 0;

	return 0;
}

int mem_regs_deinit(mem_regs *list)
{

	if (!list)
		return -EINVAL;

	if (list->regs)
		free(list->regs);

	ZeroMemory(list, sizeof(mem_regs));

	return 0;
}

int mem_regs_is_full(mem_regs *list)
{
	if (list->regs_count >= list->regs_allocated)
		return 1;
	else
		return 0;
}

int mem_regs_expand(mem_regs *list)
{
	if (!list)
		return -EINVAL;

	if (list->regs) {
		mem_reg *t;
		size_t count;

		count = list->regs_allocated + MEM_REGS_LIST_COUNT_INTT;
		t = (mem_reg *)calloc(count, sizeof(mem_reg));
		if (!t)
			return -ENOMEM;

		ZeroMemory(t, count * sizeof(mem_reg));
		CopyMemory(t, list->regs, list->regs_allocated * sizeof(mem_reg));

		free(list->regs);
		list->regs = t;
		list->regs_allocated = count;
	}

	return 0;
}

int mem_regs_insert(mem_regs *list, uint64_t addr, uint32_t data)
{
	if (!list)
		return -EINVAL;

	if (mem_regs_is_full(list))
		if (mem_regs_expand(list))
			return -ENOMEM;

	list->regs[list->regs_count].addr = addr;
	list->regs[list->regs_count].data = data;
	list->regs_count++;

	return 0;
}

int mem_regs_dump(mem_regs *list)
{
	if (!list)
		return -EINVAL;

	printf_s("\nPhysical Memory Watches:\n");

	for (size_t i = 0; i < list->regs_count; i++) {
		printf_s("Address: %#018I64x Data: %#010x\n", list->regs[i].addr, list->regs[i].data);
	}

	return 0;
}
