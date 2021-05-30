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
	{ "MailboxRegLock",	"MailboxRegLock=%x, %d",		2 },
	{ "MailboxRegGet",	"MailboxRegGet=%x, %x, %x",		3 },
	{ "MailboxRegSet",	"MailboxRegSet=%x, %x, %x",		3 },
	{ "MailboxRegRet",	"MailboxRegRet=%x, %x, %x",		3 },
	{ "OneShot",		"OneShot=%d",				1 },
	{ "PhysicalMemoryLock", "PhysicalMemoryLock=%I64x, %x",         2 },
};

#define LINE_BUF_MAX			(4096)
#define CFG_COMMENT_CHAR		('#')

static inline int is_empty_line(char *buf)
{
	if (buf[0] == '\n' || buf[0] == '\r')
		return 1;

	return 0;
}

static inline char *whitespace_skip(char *buf, size_t len)
{
	// ASCII encoding only, horizontal whitespace: ' ', '\t'
	// seek the buffer to first valid char

	for (size_t i = 0; i < len; i++) {
		if (buf[i] != ' ' && buf[i] != '\t')
			return (buf + i);
	}

	return buf;
}

static inline int is_comment(char *buf)
{
	if (buf[0] == CFG_COMMENT_CHAR)
		return 1;

	return 0;
}

size_t readline(FILE *fp, char *buf, size_t len)
{
	size_t i = 0;

	memset(buf, '\0', len);

	while (i < len) {
		char c;

		c = (char)fgetc(fp);

		if (feof(fp) || ferror(fp))
			break;

		buf[i] = c;
		i++;

		if (c == '\n')
			break;
	}

	return i;
}

int parse_ini(char *buf, size_t len, config *cfg)
{
	if (!strncmp(ini_items[OneShot].name, buf, strlen(ini_items[OneShot].name))) {
		if (sscanf(buf, ini_items[OneShot].fmt, &cfg->oneshot) != ini_items[OneShot].opts)
			return 1;

		return 0;
	}

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
		DWORD reg, cpu;

		if (sscanf_s(buf, ini_items[MailboxRegLock].fmt, &reg, &cpu)
		    != ini_items[MailboxRegLock].opts)
			return 1;

		if (msr_mb_reg_create(&cfg->regs, reg, cpu, 1))
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
	char buf[LINE_BUF_MAX];
	char *seek;
	size_t line;
	FILE *fp;
        int err = 0;

	err = fopen_s(&fp, filepath, "r");
        if (!fp)
		return err;

	line = 0;
        while (1) {
		if (readline(fp, buf, sizeof(buf))) {
			line++;

			seek = whitespace_skip(buf, sizeof(buf));

			if (is_empty_line(seek))
				continue;

			if (is_comment(seek))
				continue;

			size_t offset = seek - buf;
			size_t len = sizeof(buf) - offset;

			if (parse_ini(seek, len, cfg)) {
				printf_s("%s(): parsing failure at line %zd: %s\n", __func__, line, buf);
				err = -EINVAL;
				goto out;
			}
		}

		if (feof(fp) || ferror(fp))
			break;
        }

out:
	fclose(fp);

	return 0;
}
