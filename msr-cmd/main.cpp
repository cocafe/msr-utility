#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <Windows.h>
#include <windowsx.h>

#include <getopt.h>
#include <winring0.h>

#define ARRAY_SIZE(arr)				(sizeof(arr) / sizeof(arr[0]))

static int no_readback = 0;
static int no_column_item = 0;

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
	uint32_t proc_group;
	uint32_t proc_all;
	uint32_t proc;
	int      msr_op;
	uint32_t msr_reg;
	uint32_t edx;
	uint32_t eax;
} config_t;

typedef struct sysinfo {
	DWORD nr_pgrp;
	DWORD *nr_proc;
} sysinfo_t;

sysinfo_t g_sysinfo;

static BOOL WINAPI RdmsrPGrp(DWORD index, PDWORD eax, PDWORD edx, DWORD grp, DWORD_PTR aff_mask)
{
	BOOL result = FALSE;
	HANDLE hThread = NULL;
	GROUP_AFFINITY old_aff = { 0 }, new_aff = { 0 };

	hThread = GetCurrentThread();

	new_aff.Group = (WORD)grp;
	new_aff.Mask = aff_mask;

	if (SetThreadGroupAffinity(hThread, &new_aff, &old_aff) == 0)
		return FALSE;

	result = Rdmsr(index, eax, edx);

	if (SetThreadGroupAffinity(hThread, &old_aff, NULL) == 0)
		return FALSE;

	return result;
}

static BOOL WINAPI WrmsrPGrp(DWORD index, DWORD eax, DWORD edx, DWORD grp, DWORD_PTR aff_mask)
{
	BOOL result = FALSE;
	HANDLE hThread = NULL;
	GROUP_AFFINITY old_aff = { 0 }, new_aff = { 0 };

	hThread = GetCurrentThread();

	new_aff.Group = (WORD)grp;
	new_aff.Mask = aff_mask;

	if (SetThreadGroupAffinity(hThread, &new_aff, &old_aff) == 0)
		return FALSE;

	result = Wrmsr(index, eax, edx);

	if (SetThreadGroupAffinity(hThread, &old_aff, NULL) == 0)
		return FALSE;

	return result;
}

uint32_t nproc_get(void)
{
	SYSTEM_INFO sysinfo;

	GetSystemInfo(&sysinfo);

	return sysinfo.dwNumberOfProcessors;
}

int sysinfo_init(sysinfo_t *info)
{
	WORD nr_grps = GetActiveProcessorGroupCount();

	memset(info, 0x00, sizeof(*info));

	if (nr_grps == 0) {
		fprintf_s(stderr, "%s(): no processor group found\n", __func__);
		return -1;
	}

	info->nr_pgrp = nr_grps;
	info->nr_proc = (DWORD *)calloc(nr_grps, sizeof(DWORD));
	if (!info->nr_proc) {
		fprintf_s(stderr, "%s(): failed to allocate memory", __func__);
		return -1;
	}

	for (int i = 0; i < nr_grps; i++) {
		DWORD nr_proc = GetActiveProcessorCount(i);

		if (nr_proc == 0) {
			fprintf_s(stderr, "%s(): GetActiveProcessorCount() failed, err = %lu\n", __func__, GetLastError());
			return -1;
		}

		info->nr_proc[i] = nr_proc;

		// fprintf_s(stdout, "%s(): group %d has %d processors\n", __func__, i, nr_proc);
	}

	return 0;
}

void sysinfo_deinit(sysinfo_t* info)
{
	if (info->nr_proc)
		free(info->nr_proc);

	memset(info, 0x00, sizeof(*info));
}

void print_help(void)
{
	fprintf_s(stdout, "Usage:\n"
			  "	msr-cmd.exe [options] [read]  [reg]\n"
			  "	msr-cmd.exe [options] [write] [reg] [edx(63 - 32)] [eax(31 - 0)]\n");
	fprintf_s(stdout, "Options:\n");
	fprintf_s(stdout, "	-s		write only do not read back\n");
	fprintf_s(stdout, "	-d		data only, not to print column item name\n");
	fprintf_s(stdout, "	-g <GRP>	processor group (default: 0) to apply\n");
	fprintf_s(stdout, "	-p <CPU>	logical processor (default: 0) of processor group to apply\n");
	fprintf_s(stdout, "	-a		apply to all available processors in group\n");
}

int parse_opts(config_t *cfg, int argc, char *argv[])
{
	int index;
	int c;

	opterr = 0;

	while ((c = getopt(argc, argv, "hasdg:p:")) != -1) {
		switch (c) {
			case 'h':
				print_help();
				exit(0);

			case 'a':
				cfg->proc_all = 1;
				break;

			case 'g':
				if (sscanf_s(optarg, "%u", &cfg->proc_group) != 1) {
					fprintf_s(stderr, "%s(): failed to parse argument for -p\n", __func__);
					return -EINVAL;
				}

				if (cfg->proc_group >= g_sysinfo.nr_pgrp) {
					fprintf_s(stderr, "%s(): exceeded maximum processor group count %u on system\n", __func__, g_sysinfo.nr_pgrp);
					return -EINVAL;
				}

				break;

			case 'p':
				if (sscanf_s(optarg, "%u", &cfg->proc) != 1) {
					fprintf_s(stderr, "%s(): failed to parse argument for -p\n", __func__);
					return -EINVAL;
				}
				
				// '-g' must be passed first to work properly
				// incase we have two different processors group?
				if (cfg->proc >= g_sysinfo.nr_proc[cfg->proc_group]) {
					fprintf_s(stderr, "%s(): invalid processor id for processor gruop %u\n", __func__, cfg->proc_group);
					return -EINVAL;
				}

				break;

			case 's':
				no_readback = 1;
				break;

			case 'd':
				no_column_item = 1;
				break;

			case '?':
				if (optopt == 'p')
					fprintf_s(stderr, "%s(): option -p needs an arguemnt\n", __func__);
				if (optopt == 'g')
					fprintf_s(stderr, "%s(): option -g needs an arguemnt\n", __func__);
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

int config_init(config_t *cfg)
{
	memset(cfg, 0x00, sizeof(config_t));

	return 0;
}

int msr_read(config_t *cfg)
{
	uint32_t min, max;
	uint32_t col_printed = 0;

	if (cfg->proc_all) {
		min = 0;
		max = g_sysinfo.nr_proc[cfg->proc_group] - 1;
	} else {
		min = cfg->proc;
		max = cfg->proc;
	}

	for (size_t i = min; i <= max; i++) {
		DWORD eax = 0, edx = 0;
		DWORD_PTR thread_mask = 0;

		thread_mask = 1ULL << i;
		if (!RdmsrPGrp(cfg->msr_reg, &eax, &edx, cfg->proc_group, thread_mask)) {
			fprintf_s(stderr, "%s(): CPU%zu RdmsrTx() failed\n", __func__, i);
			return -EIO;
		}

		if (!col_printed && !no_column_item) {
			fprintf_s(stdout, "%-8s %-10s %-10s %-10s\n", "CPU", "REG", "EDX", "EAX");
			col_printed = 1;
		}

		fprintf_s(stdout, "%-8zu 0x%08x 0x%08x 0x%08x\n", i, cfg->msr_reg, edx, eax);
	}

	return 0;
}

int msr_write(config_t *cfg)
{
	uint32_t min, max;
	uint32_t col_printed = 0;

	if (cfg->proc_all) {
		min = 0;
		max = g_sysinfo.nr_proc[cfg->proc_group] - 1;
	} else {
		min = cfg->proc;
		max = cfg->proc;
	}

	for (size_t i = min; i <= max; i++) {
		DWORD eax = 0, edx = 0;
		DWORD_PTR thread_mask = 0;

		thread_mask = 1ULL << i;
		if (!WrmsrPGrp(cfg->msr_reg, cfg->eax, cfg->edx, cfg->proc_group, thread_mask)) {
			fprintf_s(stderr, "%s(): CPU%zu WrmsrTx() failed\n", __func__, i);
			return -EIO;
		}

		if (no_readback)
			break;

		eax = edx = 0;

		if (!RdmsrPGrp(cfg->msr_reg, &eax, &edx, cfg->proc_group, thread_mask)) {
			fprintf_s(stderr, "%s(): CPU%zu RdmsrTx() failed\n", __func__, i);
			return -EIO;
		}

		if (!col_printed && !no_column_item) {
			fprintf_s(stdout, "%-8s %-10s %-10s %-10s\n", "CPU", "REG", "EDX", "EAX");
			col_printed = 1;
		}

		fprintf_s(stdout, "%-8zu 0x%08x 0x%08x 0x%08x\n", i, cfg->msr_reg, edx, eax);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	config_t cfg;

	config_init(&cfg);
	if (sysinfo_init(&g_sysinfo))
		return -EFAULT;

	if (parse_opts(&cfg, argc, argv)) {
		// print_help();
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
			ret = msr_read(&cfg);
			break;

		case MSR_OPS_WRITE:
			ret = msr_write(&cfg);
			break;

		default:
			break;
	}
	
deinit:
	WinRing0_deinit();
	sysinfo_deinit(&g_sysinfo);

	return ret;
}