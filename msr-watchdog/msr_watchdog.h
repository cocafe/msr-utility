#pragma once

#include <Windows.h>

#include "msr_regs.h"

extern LPCTSTR lpProgramName;

typedef struct configuration {
	msr_regs	regs;
	int		watchdog_interval;
	int		oneshot;
} config;

int load_ini(const char *filepath, config *cfg);
