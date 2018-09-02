#include <stdio.h>
#include <stdint.h>
#include <conio.h>
#include <errno.h>
#include <Windows.h>

#include "msr_regs.h"
#include "mem_regs.h"
#include "ini_config.h"

typedef struct ini_item {
	const char	*name;
	const char	*fmt;
	int		opts;
} ini_item;

enum {
	WatchdogInterval = 0,
	MSRGeneralRegLock,
	MailboxRegLock,
	MailboxRegGet,
	MailboxRegSet,
	MailboxRegRet,
	OneShot,
	PhysicalMemoryLock,
};

ini_item ini_items[] = {
	{ "WatchdogIntervalMS",	"WatchdogIntervalMS=%d",		1 },
	{ "MSRGeneralRegLock",	"MSRGeneralRegLock=%u, %x, %x, %x",	4 },
	{ "MailboxRegLock",	"MailboxRegLock=%x",			1 },
	{ "MailboxRegGet",	"MailboxRegGet=%x, %x, %x",		3 },
	{ "MailboxRegSet",	"MailboxRegSet=%x, %x, %x",		3 },
	{ "MailboxRegRet",	"MailboxRegRet=%x, %x, %x",		3 },
	{ "OneShot",		"OneShot=%d",				1 },
	{ "PhysicalMemoryLock", "PhysicalMemoryLock=%I64x, %x",         2 },
};

void readline(FILE *fp, char *buf, size_t len)
{
	memset(buf, '\0', len);

	for (size_t i = 0; i < len; i++)
	{
		char c;

		c = (char)fgetc(fp);

		if (feof(fp))
			break;

		if (ferror(fp))
			break;

		buf[i] = c;

		if (c == '\n')
			break;
	}
}

int parse_ini(char *buf, size_t len, config *cfg)
{
	if (!strncmp(ini_items[WatchdogInterval].name, buf, strlen(ini_items[WatchdogInterval].name))) {
		if (sscanf_s(buf, ini_items[WatchdogInterval].fmt, &cfg->watchdog_interval)
		    != ini_items[WatchdogInterval].opts)
			return 1;

		return 0;
	}

	if (!strncmp(ini_items[MSRGeneralRegLock].name, buf, strlen(ini_items[MSRGeneralRegLock].name))) {
		DWORD reg;
		DWORD eax;
		DWORD edx;
		uint32_t proc;

		if (sscanf_s(buf, ini_items[MSRGeneralRegLock].fmt, &proc, &reg, &edx, &eax)
		    != ini_items[MSRGeneralRegLock].opts)
			return 1;

		if (msr_gen_reg_insert(&cfg->regs, proc, reg, eax, edx, 1))
			return 1;

		return 0;
	}

	if (!strncmp(ini_items[MailboxRegLock].name, buf, strlen(ini_items[MailboxRegLock].name))) {
		DWORD reg;

		if (sscanf_s(buf, ini_items[MailboxRegLock].fmt, &reg)
		    != ini_items[MailboxRegLock].opts)
			return 1;

		if (msr_mb_reg_create(&cfg->regs, reg, 1))
			return 1;

		return 0;
	}

	if (!strncmp(ini_items[MailboxRegGet].name, buf, strlen(ini_items[MailboxRegGet].name))) {
		DWORD reg;
		DWORD eax;
		DWORD edx;

		if (sscanf_s(buf, ini_items[MailboxRegGet].fmt, &reg, &edx, &eax)
		    != ini_items[MailboxRegGet].opts)
			return 1;

		if (msr_mb_reg_fill(&cfg->regs, reg, &eax, &edx, NULL, NULL, NULL, NULL))
			return 1;

		return 0;
	}

	if (!strncmp(ini_items[MailboxRegSet].name, buf, strlen(ini_items[MailboxRegSet].name))) {
		DWORD reg;
		DWORD eax;
		DWORD edx;

		if (sscanf_s(buf, ini_items[MailboxRegSet].fmt, &reg, &edx, &eax)
		    != ini_items[MailboxRegSet].opts)
			return 1;

		if (msr_mb_reg_fill(&cfg->regs, reg, NULL, NULL, &eax, &edx, NULL, NULL))
			return 1;

		return 0;
	}

	if (!strncmp(ini_items[MailboxRegRet].name, buf, strlen(ini_items[MailboxRegRet].name))) {
		DWORD reg;
		DWORD eax;
		DWORD edx;

		if (sscanf_s(buf, ini_items[MailboxRegRet].fmt, &reg, &edx, &eax)
		    != ini_items[MailboxRegRet].opts)
			return 1;

		if (msr_mb_reg_fill(&cfg->regs, reg, NULL, NULL, NULL, NULL, &eax, &edx))
			return 1;

		return 0;
	}

	if (!strncmp(ini_items[PhysicalMemoryLock].name, buf, strlen(ini_items[PhysicalMemoryLock].name))) {
		uint64_t addr;
		uint32_t data;

		if (sscanf_s(buf, ini_items[PhysicalMemoryLock].fmt, &addr, &data)
			!= ini_items[PhysicalMemoryLock].opts)
			return 1;

		if (mem_regs_insert(&cfg->pmem, addr, data))
			return 1;

		return 0;
	}

	return -ENOENT;
}

int load_ini(const char *filepath, config *cfg)
{
	FILE *fp;
	size_t line;
	char buf[1024];
	errno_t err;

	err = fopen_s(&fp, filepath, "r");
	if (!fp)
		return err;

	line = 0;
	while (1) {
		readline(fp, buf, sizeof(buf));
		line++;

		if (feof(fp))
			break;

		if (ferror(fp))
			break;

		//printf("%d %s", line, buf);

		// XXX: what if we have whitespaces before #
		if (buf[0] == '#' || buf[0] == '\n') {
			continue;
		}

		if (parse_ini(buf, sizeof(buf), cfg)) {
			printf_s("%s: parsing failure at line %zd: %s\n", __func__, line, buf);
			fclose(fp);

			return -EFAULT;
		}
	}

	fclose(fp);

	return 0;
}
