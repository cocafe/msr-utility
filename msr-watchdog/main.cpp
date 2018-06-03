#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <Windows.h>
#include <windowsx.h>

#include <winring0.h>

#include "msr_regs.h"
#include "msr_watchdog.h"

LPCTSTR lpProgramName = L"msr watchdog";

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

		printf_s("%s: RdmsrTx(): CPU%2u reg: %#010x edx: %#010x eax: %#010x\n",
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

		printf_s("%s: Rdmsr(): reg: %#010x edx: %#010x eax: %#010x\n",
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

void config_init(config *cfg)
{
	memset(cfg, 0x00, sizeof(config));
}

int WINAPI WinMain(HINSTANCE hInstance, 
		   HINSTANCE hPrevInstance,
		   LPSTR lpCmdLine, 
		   int nCmdShow)
{
	int ret = 0;
	config cfg;

	config_init(&cfg);

	ret = WinRing0_init();
	if (ret)
		goto out;

	if (!IsMsr()) {
		MessageBox(NULL, L"System platform does not support MSR instruction", lpProgramName, MB_ICONINFORMATION | MB_OK);
		goto deinit;
	}

	if (!strncmp(lpCmdLine, "-one", sizeof(char) * 4))
		cfg.oneshot = 1;

	msr_regs_init(&cfg.regs);

	ret = load_ini("./msr-regs.ini", &cfg);
	if (ret) {
		MessageBox(NULL, L"Failed to load registers config: msr-regs.ini", lpProgramName, MB_ICONSTOP | MB_OK);
		goto deinit;
	}

	printf_s("%s: load msr-regs.ini successfully\n", __func__);

	msr_regs_dump(&cfg.regs);

	while (1) {
		if (msr_gen_reg_deamon(&cfg.regs))
			break;

		if (msr_mailbox_deamon(&cfg.regs))
			break;

		if (cfg.oneshot) {
			printf_s("%s: one shot enabled\n", __func__);
			break;
		}

		Sleep(cfg.watchdog_interval);
	}

deinit:
	msr_regs_deinit(&cfg.regs);
	WinRing0_deinit();

out:
	return ret;
}