#include <io.h>
#include <conio.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <Windows.h>
#include <windowsx.h>

#include <winio.h>
#include <winring0.h>

#include "msr_regs.h"
#include "ini_config.h"

LPCTSTR lpProgramName = L"msr watchdog";

#define MSR_WATCHDOG_INTERVAL_MS		(3000)

int msr_gen_reg_deamon(msr_regs *regs)
{
	for (size_t i = 0; i < regs->gen_regs_count; i++)
	{
		DWORD eax;
		DWORD edx;
		DWORD_PTR thread_mask = 0;

		if (!regs->gen_regs[i].watch)
			continue;

		thread_mask = 1ULL << regs->gen_regs[i].proc;

		if (!RdmsrTx(regs->gen_regs[i].reg, &eax, &edx, thread_mask)) {
			printf_s("%s(): RdmsrTx() failure\n", __func__);
			return -EIO;
		}

		printf_s("%s(): RdmsrTx(): CPU%2u reg: %#010x edx: %#010x eax: %#010x\n",
		       __func__, regs->gen_regs[i].proc, regs->gen_regs[i].reg, edx, eax);

		if (eax != regs->gen_regs[i].eax || edx != regs->gen_regs[i].edx) {
			printf_s("%s(): Register value differs, invoke WrmsrTx()\n", __func__);
			if (!WrmsrTx(regs->gen_regs[i].reg,
				     regs->gen_regs[i].eax,
				     regs->gen_regs[i].edx,
				     thread_mask)) {
				int ret_msgbox = MessageBox(NULL, L"WinRing0 driver failed to write register, continue?", 
							    lpProgramName, MB_ICONWARNING | MB_YESNO);
				if (ret_msgbox == IDNO)
					return -EIO;
			}
		}
	}

	return 0;
}

int msr_mailbox_deamon(msr_regs *regs)
{
	for (size_t i = 0; i < regs->mb_regs_count; i++)
	{
		DWORD eax;
		DWORD edx;

		if (!regs->mb_regs[i].watch)
			continue;

		if (!Wrmsr(regs->mb_regs[i].reg,
			   regs->mb_regs[i].eax_get,
			   regs->mb_regs[i].edx_get)) {
			printf_s("%s(): Wrmsr() failure\n", __func__);
			return -EIO;
		}
		if (!Rdmsr(regs->mb_regs[i].reg, &eax, &edx)) {
			printf_s("%s(): Rdmsr() failure\n", __func__);
			return -EIO;
		}

		printf_s("%s(): Rdmsr(): reg: %#010x edx: %#010x eax: %#010x\n",
		       __func__, regs->mb_regs[i].reg, edx, eax);

		if (regs->mb_regs[i].eax_ret != eax || regs->mb_regs[i].edx_ret != edx) {
			printf_s("%s(): Register value differs, call Wrmsr()\n", __func__);
			if (!Wrmsr(regs->mb_regs[i].reg,
				   regs->mb_regs[i].eax_set,
				   regs->mb_regs[i].edx_set)) {
				int ret_msgbox = MessageBox(NULL, L"WinRing0 driver failed to write register, continue?",
							    lpProgramName, MB_ICONWARNING | MB_YESNO);
				if (ret_msgbox == IDNO)
					return -EIO;
			}
		}
	}

	return 0;
}

int pmem_deamon(mem_regs *regs)
{
	for (size_t i = 0; i < regs->regs_count; i++) {
		DWORD data_r = 0;

		if (GetPhysLong((PBYTE)regs->regs[i].addr, &data_r) == FALSE) {
			printf_s("%s(): GetPhysLong() failure\n", __func__);
			return -EFAULT;
		}

		if (data_r == regs->regs[i].data)
			continue;

		if (SetPhysLong((PBYTE)regs->regs[i].addr, regs->regs[i].data) == FALSE) {
			printf_s("%s(): GetPhysLong() failure\n", __func__);
			return -EFAULT;
		}
	}

	return 0;
}

void config_init(config *cfg)
{
	memset(cfg, 0x00, sizeof(config));

	cfg->oneshot = 0;
	cfg->watchdog_interval = MSR_WATCHDOG_INTERVAL_MS;

	msr_regs_init(&cfg->regs);
	mem_regs_init(&cfg->pmem);
}

void config_deinit(config *cfg)
{
	msr_regs_deinit(&cfg->regs);
	mem_regs_deinit(&cfg->pmem);
}

void console_init(void)
{
	AllocConsole();
}

void console_deinit(void)
{
	FreeConsole();
}

void console_stdio_redirect(void)
{
	HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
	FILE *COutputHandle = _fdopen(SystemOutput, "w");

	HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
	int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
	FILE *CErrorHandle = _fdopen(SystemError, "w");

	HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
	int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
	FILE *CInputHandle = _fdopen(SystemInput, "r");

	freopen_s(&CInputHandle, "CONIN$", "r", stdin);
	freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
	freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);
}

int WINAPI WinMain(HINSTANCE hInstance, 
		   HINSTANCE hPrevInstance,
		   LPSTR lpCmdLine, 
		   int nCmdShow)
{
	config cfg;
	int ret = 0;

	ret = WinIO_init();
	if (ret)
		goto out;

	ret = WinRing0_init();
	if (ret)
		goto out;

	if (!IsMsr()) {
		MessageBox(NULL, L"System platform does not support MSR instruction",
			lpProgramName, MB_ICONINFORMATION | MB_OK);
		goto deinit;
	}

	if (!strncmp(lpCmdLine, "-one", sizeof(char) * 4))
		cfg.oneshot = 1;

	config_init(&cfg);

	ret = load_ini("./msr-watchdog.ini", &cfg);
	if (ret) {
		MessageBox(NULL, L"Failed to parse config .ini", lpProgramName, MB_ICONSTOP | MB_OK);
		goto deinit;
	}

	printf("%s(): load config .ini successfully\n", __func__);

	msr_regs_dump(&cfg.regs);
	mem_regs_dump(&cfg.pmem);

	while (1) {
		if (msr_gen_reg_deamon(&cfg.regs))
			break;

		if (msr_mailbox_deamon(&cfg.regs))
			break;

		if (pmem_deamon(&cfg.pmem))
			break;

		if (cfg.oneshot) {
			printf_s("%s: one shot enabled\n", __func__);
			break;
		}

		Sleep(cfg.watchdog_interval);
	}

deinit:
	config_deinit(&cfg);
	WinRing0_deinit();
	WinIO_deinit();

out:
	return ret;
}