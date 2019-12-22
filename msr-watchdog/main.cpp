#include <io.h>
#include <conio.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <Windows.h>
#include <windowsx.h>
#include <WinUser.h>

#include <winio.h>
#include <winring0.h>

#include "msr_regs.h"
#include "ini_config.h"

#define DEFAULT_CONFIG_INI			"./msr-watchdog.ini"
#define MSR_WATCHDOG_INTERVAL_MS		(3000)
#define SHORT_OPTS				"hdwc:"

static LPCTSTR lpProgramName = L"msr watchdog";
static const char* program_name = "msr-watchdog";

static char* help_text[] = {
	"-h	this help text\n",
	"-d	enable debug output window\n",
	"-c	specified config ini path\n",
	"-w	enable WinIO, disabled by default,\n",
	"	install WinIO driver manually first\n",
};

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
	cfg->winio_enabled = 0;
	cfg->watchdog_interval = MSR_WATCHDOG_INTERVAL_MS;
	strncpy_s(cfg->ini_path, DEFAULT_CONFIG_INI, sizeof(cfg->ini_path));

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

void msgbox_help()
{
	char buf[4096] = { 0 };
	size_t len = 0;

	for (int i = 0; i < ARRAYSIZE(help_text); i++)
		len += snprintf(&buf[len], sizeof(buf) - len, "%s", help_text[i]);

	// ensure properly terminated
	buf[sizeof(buf) - 1] = '\0';

	MessageBoxA(NULL, buf, program_name, MB_OK);
}

int parse_opts(config* cfg, int argc, char* argv[])
{
	int index;
	int c;

	(void)index;

	opterr = 0;

	while ((c = getopt(argc, argv, SHORT_OPTS)) != -1) {
		switch (c) {
		case 'h':
			msgbox_help();
			exit(0);

			break;

		case 'd':
			cfg->debug = 1;
			break;

		case 'c':
			strncpy_s(cfg->ini_path, optarg, sizeof(cfg->ini_path));
			break;

		case 'w':
			cfg->winio_enabled = 1;
			break;

		default:
			break;
		}
	}

	return 0;
}


int WINAPI WinMain(HINSTANCE hInstance, 
		   HINSTANCE hPrevInstance,
		   LPSTR lpCmdLine, 
		   int nCmdShow)
{
	config cfg;
	int ret = 0;

	config_init(&cfg);

	ret = parse_opts(&cfg, __argc, __argv);
	if (ret) {
		msgbox_help();
		goto out;
	}

	if (cfg.debug) {
		console_init();
		console_stdio_redirect();
	}

	if (cfg.winio_enabled) {
		ret = WinIO_init();
		if (ret)
			goto out;
	}

	ret = WinRing0_init();
	if (ret)
		goto out;

	if (!IsMsr()) {
		MessageBox(NULL, L"System platform does not support MSR instruction",
			lpProgramName, MB_ICONINFORMATION | MB_OK);
		goto deinit;
	}

	ret = load_ini(cfg.ini_path, &cfg);
	if (ret) {
		MessageBox(NULL, L"Failed to parse config .ini", lpProgramName, MB_ICONSTOP | MB_OK);
		goto deinit;
	}

	printf("%s(): load config %s successfully\n", __func__, cfg.ini_path);

	msr_regs_dump(&cfg.regs);
	mem_regs_dump(&cfg.pmem);

	printf("\n%s(): start watchdog...\n", __func__);

	while (1) {
		if (msr_gen_reg_deamon(&cfg.regs))
			break;

		if (msr_mailbox_deamon(&cfg.regs))
			break;

		if (cfg.winio_enabled && pmem_deamon(&cfg.pmem))
			break;

		if (cfg.oneshot) {
			printf_s("%s: one shot enabled\n", __func__);
			break;
		}

		Sleep(cfg.watchdog_interval);
	}

deinit:
	if (cfg.debug)
		console_deinit();

	config_deinit(&cfg);
	WinRing0_deinit();

	if (cfg.winio_enabled)
		WinIO_deinit();

out:
	return ret;
}