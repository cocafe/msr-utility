#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <Windows.h>
#include <windowsx.h>

#include <getopt.h>
#include <winring0.h>

#define ARRAY_SIZE(arr)				(sizeof(arr) / sizeof(arr[0]))

static int no_readback;

typedef struct msr_op {
	uint32_t idx;
	const char *arg;
} msr_op;

enum msr_ops_idx {
	MSR_OPS_READ = 0,
	MSR_OPS_WRITE,
	NUM_MSR_OPS,
};

msr_op msr_ops[] = {
	{ MSR_OPS_READ, "read" },
	{ MSR_OPS_WRITE, "write" },
};

enum params_not_opt {
	WRMSR_OP = 0,
	WRMSR_REG,
	WRMSR_EDX,
	WRMSR_EAX,
};

typedef struct configuration {
	uint32_t proc_all;
	uint32_t proc;
	uint32_t nproc;
	uint32_t msr_op;
	uint32_t msr_reg;
	uint32_t edx;
	uint32_t eax;
} config;

uint32_t nproc_get(void)
{
	SYSTEM_INFO sysinfo;

	GetSystemInfo(&sysinfo);

	return sysinfo.dwNumberOfProcessors;
}

void print_help(void)
{
	fprintf_s(stdout, "Usage:\n"
			  "	msr-cmd.exe [options] [read]  [reg]\n"
			  "	msr-cmd.exe [options] [write] [reg] [edx(63 - 32)] [eax(31 - 0)]\n");
	fprintf_s(stdout, "Options:\n");
	fprintf_s(stdout, "	-a		apply to all available processors\n");
	fprintf_s(stdout, "	-s		write only do not read back\n");
	fprintf_s(stdout, "	-p [CPU]	processor (default: CPU0) to apply\n");
}

int parse_opts(config *cfg, int argc, char *argv[])
{
	int index;
	int c;

	opterr = 0;

	while ((c = getopt(argc, argv, "hasp:")) != -1) {
		switch (c) {
			case 'h':
				print_help();
				exit(0);

			case 'a':
				cfg->proc_all = 1;
				break;

			case 'p':
				if (sscanf_s(optarg, "%u", &cfg->proc) != 1) {
					fprintf_s(stderr, "%s(): failed to parse argument for -p\n", __func__);
					return -EINVAL;
				}
				
				if (cfg->proc > (cfg->nproc - 1)) {
					fprintf_s(stderr, "%s(): invalid processor id\n", __func__);
					return -EINVAL;
				}

				break;

			case 's':
				no_readback = 1;
				break;

			case '?':
				if (optopt == 'p')
					fprintf_s(stderr, "%s(): option -p needs an arguemnt\n", __func__);
				else if (isprint(optopt))
					fprintf_s(stderr, "%s(): unknonw option -%c\n", __func__, optopt);
				else
					fprintf_s(stderr, "%s(): failed to parse character: \\x%x\n", __func__, optopt);

				return -EINVAL;

			default:
				break;
		}
	}

	int i = 0;
	for (index = optind; index < argc; index++, i++) {
		switch (i) {
			case WRMSR_OP:
				{
					int f = 0;

					for (int j = 0; j < ARRAY_SIZE(msr_ops); j++) {
						if (!strcmp(argv[index], msr_ops[j].arg)) {
							cfg->msr_op = msr_ops[j].idx;
							f = 1;
						}
					}

					if (!f) {
						fprintf_s(stderr, "%s(): invalid opeartion argument: %s\n", __func__, argv[index]);
						return -EINVAL;
					}
				}
				
				break;

			case WRMSR_REG:
				if (sscanf_s(argv[index], "%x", &cfg->msr_reg) != 1) {
					fprintf_s(stderr, "%s(): invalid msr register\n", __func__);
					return -EINVAL;
				}
				
				break;

			case WRMSR_EDX:
				if (sscanf_s(argv[index], "%x", &cfg->edx) != 1) {
					fprintf_s(stderr, "%s(): invalid edx register\n", __func__);
					return -EINVAL;
				}
				break;

			case WRMSR_EAX:
				if (sscanf_s(argv[index], "%x", &cfg->eax) != 1) {
					fprintf_s(stderr, "%s(): invalid eax register\n", __func__);
					return -EINVAL;
				}
				break;

			default:
				break;
		}
	}

	if ((cfg->msr_op == MSR_OPS_READ && i < 2) || (cfg->msr_op == MSR_OPS_WRITE && i < 4)) {
		fprintf_s(stderr, "%s(): missing arguments\n", __func__);
		return -EINVAL;
	}

	return 0;
}

int config_init(config *cfg)
{
	memset(cfg, 0x00, sizeof(config));

	cfg->proc = 0;
	cfg->nproc = nproc_get();

	return 0;
}

int msr_read(config *cfg)
{
	uint32_t min, max;

	if (cfg->proc_all) {
		min = 0;
		max = cfg->nproc - 1;
	} else {
		min = cfg->proc;
		max = cfg->proc;
	}

	for (size_t i = min; i <= max; i++) {
		DWORD eax, edx = 0;
		DWORD_PTR thread_mask = 0;

		thread_mask = 1ULL << i;
		if (!RdmsrTx(cfg->msr_reg, &eax, &edx, thread_mask)) {
			fprintf_s(stderr, "%s(): CPU%zu RdmsrTx() failed\n", __func__, i);
		}

		fprintf_s(stdout, "%s(): CPU%2zu reg: 0x%08x edx: 0x%08x eax: 0x%08x\n", 
			  __func__, i, cfg->msr_reg, edx, eax);
	}

	return 0;
}

int msr_write(config *cfg)
{
	uint32_t min, max;

	if (cfg->proc_all) {
		min = 0;
		max = cfg->nproc - 1;
	} else {
		min = cfg->proc;
		max = cfg->proc;
	}

	for (size_t i = min; i <= max; i++) {
		DWORD eax, edx = 0;
		DWORD_PTR thread_mask = 0;

		thread_mask = 1ULL << i;
		if (!WrmsrTx(cfg->msr_reg, cfg->eax, cfg->edx, thread_mask)) {
			fprintf_s(stderr, "%s(): CPU%zu WrmsrTx() failed\n", __func__, i);
		}

		if (no_readback)
			break;

		if (!RdmsrTx(cfg->msr_reg, &eax, &edx, thread_mask)) {
			fprintf_s(stderr, "%s(): CPU%zu RdmsrTx() failed\n", __func__, i);
		}

		fprintf_s(stdout, "%s(): ret: CPU%2zu reg: 0x%08x edx: 0x%08x eax: 0x%08x\n",
			  __func__, i, cfg->msr_reg, edx, eax);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	config cfg;

	config_init(&cfg);

	if (parse_opts(&cfg, argc, argv)) {
		print_help();
		return -EINVAL;
	}

	//printf_s("-a: %u -p: %u op: %s reg: %#010x edx: %#010x eax: %010x\n",
	//	 cfg.proc_all,
	//	 cfg.proc,
	//	 msr_ops[cfg.msr_op].arg,
	//	 cfg.msr_reg,
	//	 cfg.edx,
	//	 cfg.eax);

	if (WinRing0_init()) {
		fprintf_s(stderr, "%s(): failed to init WinRing0 driver\n", __func__);
		return -EFAULT;
	}

	if (!IsMsr()) {
		fprintf_s(stderr, "%s(): system platform does not support MSR instruction\n", __func__);
		goto deinit;
	}

	switch (cfg.msr_op) {
		case MSR_OPS_READ:
			msr_read(&cfg);
			break;

		case MSR_OPS_WRITE:
			msr_write(&cfg);
			break;

		default:
			break;
	}
	
deinit:
	WinRing0_deinit();

	return 0;
}