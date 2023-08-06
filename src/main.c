#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <windows.h>

#include <libjj/utils.h>
#include <libjj/logging.h>
#include <libjj/opts.h>
#include <libjj/iconv.h>
#include <libjj/ffs.h>

#include <libwinring0/winring0.h>

typedef struct config config_t;

union msr_val {
        struct {
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
                DWORD eax;
                DWORD edx;
#else
                DWORD edx;
                DWORD eax;
#endif
        } u;
        uint64_t ull;
};

enum msr_ops_idx {
        MSR_OPS_READ = 0,
        MSR_OPS_WRITE,
        MSR_OPS_RMW,
        MSR_OPS_RMW_MASKED,
        MSR_OPS_GETBIT,
        MSR_OPS_SETBIT,
        NUM_MSR_OPS,
};

struct msr_op {
        uint32_t idx;
        const char *arg;
} msr_op;

struct msr_op msr_ops[] = {
        { MSR_OPS_READ,       "read"          },
        { MSR_OPS_WRITE,      "write"         },
        { MSR_OPS_RMW,        "rmw"           },
        { MSR_OPS_RMW_MASKED, "rmwmask"       },
        { MSR_OPS_GETBIT,     "getbit"        },
        { MSR_OPS_SETBIT,     "setbit"        },
};

struct config {
        uint32_t proc_group;
        uint32_t proc_all;
        uint32_t group_all;
        uint32_t proc;
        int      op;
        uint32_t reg;
        uint32_t edx;
        uint32_t eax;
        uint64_t rmw_mask;
        uint64_t rmw_val;
        uint32_t rmw_h;
        uint32_t rmw_l;
        uint32_t reg_bit;
};

static int no_readback = 0;
static int no_column_item = 0;
static int dry_run = 0;
static int use_long_value = 0;

static uint32_t nr_pgrp;
static uint32_t *nr_proc_pgrp;

static config_t g_cfg;

int systopo_init(void)
{
        DWORD nr_cpu_grp = GetActiveProcessorGroupCount();
        uint32_t *cpu_grp;

        cpu_grp = calloc(nr_cpu_grp, sizeof(uint32_t));
        if (!cpu_grp)
                return -ENOMEM;

        for (uint32_t i = 0; i < nr_cpu_grp; i++) {
                cpu_grp[i] = (uint32_t)GetMaximumProcessorCount(i);
        }

        nr_pgrp = nr_cpu_grp;
        nr_proc_pgrp = cpu_grp;

        return 0;
}

void systopo_deinit(void)
{
        if (nr_proc_pgrp) {
                free(nr_proc_pgrp);
                nr_proc_pgrp = 0;
        }

        nr_pgrp = 0;
}

void help(void)
{
        pr_raw("Usage:\n"
               "	msr-cmd.exe [options] read <reg>\n"
               "	msr-cmd.exe [options] write <reg> <edx(63~32)> <eax(31~0)>\n"
               "	msr-cmd.exe [options] -l write <reg> <hex_val(63~0)>\n"
               "	msr-cmd.exe [options] rmw <reg> <h bit> <l bit> <unmasked_hex_val>\n"
               "	msr-cmd.exe [options] rmwmask <reg> <mask(63~0)> <unmasked_hex_val>\n"
               "	msr-cmd.exe [options] getbit <reg> <#bit>\n"
               "	msr-cmd.exe [options] setbit <reg> <#bit> <0/1>\n"
        );
        pr_raw("\n");
        pr_raw("Options:\n");
        pr_raw("	-s		write only do not read back\n");
        pr_raw("	-d		data only, not to print column item name\n");
        pr_raw("	-g <GRP>	processor group (default: 0) to apply\n"
               "                        'A' or 'a' to apply to all available processors groups\n"
               "                        by default, a group can contain up to 64 logical processors\n"
        );
        pr_raw("	-p <CPU>	logical processor (default: 0) of specified processor group to apply\n");
        pr_raw("	-a		operate on all available processors in specified processor group\n");
        pr_raw("	-A		operate on all available processors in all available processor groups\n");
        pr_raw("	-l		use long 64 bits value for input/output instead of EDX EAX\n");
        pr_raw("        -t		dry run\n");
        pr_raw("\n");
        pr_raw("Example:\n"
               "	msr-cmd.exe -A read 0xc0010015\n"
               "	msr-cmd.exe -l read 0xc0010015\n"
               "	msr-cmd.exe write 0xc0010296 0x00808080 0x00404040\n"
               "	msr-cmd.exe -l write 0xc0010296 0x0080808000404040\n"
               "        # register 0xc0010294 will be ANDed with NOT of mask 0xff000000 then register will be ORed with 0xee000000\n"
               "	msr-cmd.exe rmw 0xc0010294 31 24 0xee\n"
               "	msr-cmd.exe rmwmask 0xc0010294 0xff000000 0xee\n"
               "	# get value of bit 6 of register 0xc0010296\n"
               "	msr-cmd.exe getbit 0xc0010296 6\n"
               "	msr-cmd.exe setbit 0xc0010296 6 1\n"
        );
}

int args_parse(int argc, char *argv[])
{
        static int args_cnt_wr = 4;
        int index;
        int c;

        opterr = 0;

        while ((c = getopt(argc, argv, "haAsdg:p:tl")) != -1) {
                switch (c) {
                case 'h':
                        help();
                        exit(0);

                case 'a':
                        g_cfg.proc_all = 1;
                        break;

                case 'A':
                        g_cfg.proc_all = 1;
                        g_cfg.group_all = 1;
                        break;

                case 'g':
                        if (optarg[0] == 'A' || optarg[0] == 'a') {
                                g_cfg.group_all = 1;
                                break;
                        }

                        if (sscanf(optarg, "%u", &g_cfg.proc_group) != 1) {
                                pr_err("failed to parse argument for -p\n");
                                return -EINVAL;
                        }

                        if (g_cfg.proc_group >= nr_pgrp) {
                                pr_err("exceeded maximum processor group count (%u) on system\n", nr_pgrp);
                                return -EINVAL;
                        }

                        break;

                case 'p':
                        if (sscanf(optarg, "%u", &g_cfg.proc) != 1) {
                                pr_err("failed to parse argument for -p\n");
                                return -EINVAL;
                        }

                        // '-g' must be passed first to work properly
                        // incase we have two different processors group?
                        if (g_cfg.proc >= nr_proc_pgrp[g_cfg.proc_group]) {
                                pr_err("exceeded maximum processor count (%u) for processor group %u\n", nr_proc_pgrp[g_cfg.proc_group], g_cfg.proc_group);
                                return -EINVAL;
                        }

                        break;

                case 's':
                        no_readback = 1;
                        break;

                case 'd':
                        no_column_item = 1;
                        break;

                case 't':
                        dry_run = 1;
                        break;

                case 'l':
                        use_long_value = 1;
                        break;

                case '?':
                        if (optopt == 'p')
                                pr_err("option -p needs an arguemnt\n");
                        if (optopt == 'g')
                                pr_err("option -g needs an arguemnt\n");

                        return -EINVAL;

                default:
                        break;
                }
        }

        int i = 0;
        for (index = optind; index < argc; index++, i++) {
                if (i == 0) {
                        int f = 0;

                        for (size_t j = 0; j < ARRAY_SIZE(msr_ops); j++) {
                                if ((strlen(argv[index]) == strlen(msr_ops[j].arg)) &&
                                    !strcmp(argv[index], msr_ops[j].arg)) {
                                        g_cfg.op = msr_ops[j].idx;
                                        f = 1;
                                }
                        }

                        if (!f) {
                                pr_err("invalid operation argument: %s\n", argv[index]);
                                return -EINVAL;
                        }

                        continue;
                }

                if (i == 1) {
                        if (sscanf(argv[index], "%x", &g_cfg.reg) != 1) {
                                pr_err("failed to parse msr register index\n");
                                return -EINVAL;
                        }
                }

                switch (g_cfg.op) {
                case MSR_OPS_READ:
                case MSR_OPS_WRITE:
                        switch (i) {
                        case 2:
                                if (use_long_value) {
                                        uint64_t t;

                                        if (sscanf(argv[index], "%jx", &t) != 1) {
                                                pr_err("invalid register value\n");
                                                return -EINVAL;
                                        }

                                        g_cfg.edx = t >> 32 & UINT32_MAX;
                                        g_cfg.eax = t & UINT32_MAX;

                                }
                                else if (sscanf(argv[index], "%x", &g_cfg.edx) != 1) {
                                        pr_err("invalid edx register\n");
                                        return -EINVAL;
                                }

                                break;

                        case 3:
                                if (use_long_value)
                                        break;

                                if (sscanf(argv[index], "%x", &g_cfg.eax) != 1) {
                                        pr_err("invalid eax register\n");
                                        return -EINVAL;
                                }

                                break;

                        default:
                                break;
                        } // switch(i)

                        break;

                case MSR_OPS_RMW:
                        switch (i) {
                        case 2:
                                if (sscanf(argv[index], "%u", &g_cfg.rmw_h) != 1 || g_cfg.rmw_h > 63) {
                                        pr_err("invalid high mask bit\n");
                                        return -EINVAL;
                                }

                                break;
                        
                        case 3:
                                if (sscanf(argv[index], "%u", &g_cfg.rmw_l) != 1 || g_cfg.rmw_l > 63) {
                                        pr_err("invalid low mask bit\n");
                                        return -EINVAL;
                                }

                                if (g_cfg.rmw_h < g_cfg.rmw_l) {
                                        pr_err("high mask bit (%u) is smaller than low mask bit (%u)\n", g_cfg.rmw_h, g_cfg.rmw_l);
                                        return -EINVAL;
                                }

                                g_cfg.rmw_mask = GENMASK_ULL(g_cfg.rmw_h, g_cfg.rmw_l);

                                break;

                        case 4:
                                if (sscanf(argv[index], "%jx", &g_cfg.rmw_val) != 1) {
                                        pr_err("invalid value\n");
                                        return -EINVAL;
                                }

                                break;

                        default:
                                break;
                        } // switch(i)
                        
                        break;
                
                case MSR_OPS_RMW_MASKED:
                        switch (i) {
                        case 2:
                                if (sscanf(argv[index], "%jx", &g_cfg.rmw_mask) != 1) {
                                        pr_err("invalid mask\n");
                                        return -EINVAL;
                                }

                                break;

                        case 3:
                                if (sscanf(argv[index], "%jx", &g_cfg.rmw_val) != 1) {
                                        pr_err("invalid value\n");
                                        return -EINVAL;
                                }

                                break;

                        default:
                                break;
                        } // switch(i)

                        break;

                case MSR_OPS_GETBIT:
                        switch (i) {
                        case 2:
                                if (sscanf(argv[index], "%u", &g_cfg.reg_bit) != 1 || g_cfg.reg_bit > 63) {
                                        pr_err("invalid bit range to get\n");
                                        return -EINVAL;
                                }

                                break;
                        } // switch(i)

                        break;

                case MSR_OPS_SETBIT:
                        switch (i) {
                        case 2:
                                if (sscanf(argv[index], "%u", &g_cfg.reg_bit) != 1 || g_cfg.reg_bit > 63) {
                                        pr_err("invalid bit range to get\n");
                                        return -EINVAL;
                                }

                                break;

                        case 3:
                                if (sscanf(argv[index], "%ju", &g_cfg.rmw_val) != 1) {
                                        pr_err("invalid value for bit\n");
                                        return -EINVAL;
                                }

                                g_cfg.rmw_val = !!g_cfg.rmw_val;
                                g_cfg.rmw_mask = BIT_ULL(g_cfg.reg_bit);

                                break;
                        } // switch(i)

                        break;

                default:
                        break;
                }


        }

        if (use_long_value) {
                args_cnt_wr -= 1;
        }

        if ((g_cfg.op == MSR_OPS_READ && i != 2) ||
            (g_cfg.op == MSR_OPS_WRITE && i != args_cnt_wr) ||
            (g_cfg.op == MSR_OPS_RMW && i != 5) ||
            (g_cfg.op == MSR_OPS_RMW_MASKED && i != 4) ||
            (g_cfg.op == MSR_OPS_GETBIT && i != 3) ||
            (g_cfg.op == MSR_OPS_SETBIT && i != 4)) {
                pr_err("invalid argument count\n");
                return -EINVAL;
        }

        return 0;
}

int wargs_parse(int argc, wchar_t *wargv[])
{
        char **argv;
        int ret = 0;

        // extra null terminated is required for getopt_long()
        argv = calloc(argc + 1, sizeof(char *));
        if (!argv)
                return -ENOMEM;

        for (int i = 0; i < argc; i++) {
                char **v = &argv[i];
                size_t len = wcslen(wargv[i]);
                *v = calloc(len + 2, sizeof(char));
                if (!*v)
                        return -ENOMEM;

                if (iconv_wc2utf8(wargv[i], len * sizeof(wchar_t), *v, len * sizeof(char)))
                        return -EINVAL;
        }

        ret = args_parse(argc, argv);

        for (int i = 0; i < argc; i++) {
                if (argv[i])
                        free(argv[i]);
        }

        free(argv);

        return ret;
}

static void result_print(int grp, size_t cpu, uint32_t reg, uint32_t edx, uint32_t eax)
{
        if (use_long_value) {
                uint64_t val = ((uint64_t)edx << 32) | eax;
                pr_raw("%-8u %-8zu 0x%08x 0x%016jx\n", grp, cpu, reg, val);
        }
        else {
                pr_raw("%-8u %-8zu 0x%08x 0x%08x 0x%08x\n", grp, cpu, reg, edx, eax);
        }
}

int msr_read(void)
{
        union msr_val v;
        uint32_t min, max;

        if (g_cfg.proc_all) {
                min = 0;
                max = nr_proc_pgrp[g_cfg.proc_group] - 1;
        } else {
                min = g_cfg.proc;
                max = g_cfg.proc;
        }

        for (size_t i = min; i <= max; i++) {
                DWORD_PTR thread_mask = BIT_ULL(i);
                v.ull = 0;

                if (dry_run) {
                        pr_raw("%-8u %-8zu 0x%08x\n", g_cfg.proc_group, i, g_cfg.reg);
                        continue;
                }

                if (!RdmsrPGrp(g_cfg.reg, (DWORD *)&v.u.eax, (DWORD *)&v.u.edx, g_cfg.proc_group, thread_mask)) {
                        pr_err("CPU%zu read msr failed\n", i);
                        return -EIO;
                }

                result_print(g_cfg.proc_group, i, g_cfg.reg, v.u.edx, v.u.eax);
        }

        return 0;
}

int msr_write(void)
{
        union msr_val v;
        uint32_t min, max;

        if (g_cfg.proc_all) {
                min = 0;
                max = nr_proc_pgrp[g_cfg.proc_group] - 1;
        } else {
                min = g_cfg.proc;
                max = g_cfg.proc;
        }

        for (size_t i = min; i <= max; i++) {
                DWORD_PTR thread_mask = BIT_ULL(i);

                if (dry_run) {
                        result_print(g_cfg.proc_group, i, g_cfg.reg, g_cfg.edx, g_cfg.eax);
                        continue;
                }

                if (!WrmsrPGrp(g_cfg.reg, g_cfg.eax, g_cfg.edx, g_cfg.proc_group, thread_mask)) {
                        pr_err("CPU%zu write msr failed\n", i);
                        return -EIO;
                }

                if (no_readback)
                        continue;

                v.ull = 0;

                if (!RdmsrPGrp(g_cfg.reg, (DWORD *)&v.u.eax, (DWORD *)&v.u.edx, g_cfg.proc_group, thread_mask)) {
                        pr_err("CPU%zu read msr failed\n", i);
                        return -EIO;
                }

                result_print(g_cfg.proc_group, i, g_cfg.reg, v.u.edx, v.u.eax);
        }

        return 0;
}

int msr_rmw(void)
{
        union msr_val v;
        uint32_t min, max;
        uint64_t val = 0;

        if (g_cfg.proc_all) {
                min = 0;
                max = nr_proc_pgrp[g_cfg.proc_group] - 1;
        } else {
                min = g_cfg.proc;
                max = g_cfg.proc;
        }

        if (g_cfg.rmw_mask != 0) {
                unsigned long first = find_first_bit_u64(&g_cfg.rmw_mask);
                val = (g_cfg.rmw_val << first) & g_cfg.rmw_mask;
        }

        for (size_t i = min; i <= max; i++) {
                DWORD_PTR thread_mask = BIT_ULL(i);

                if (dry_run) {
                        pr_raw("%-8u %-8zu 0x%08x\n", g_cfg.proc_group, i, g_cfg.reg);
                        continue;
                }

                v.ull = 0;
                if (!RdmsrPGrp(g_cfg.reg, (DWORD *)&v.u.eax, (DWORD *)&v.u.edx, g_cfg.proc_group, thread_mask)) {
                        pr_err("CPU%zu read msr failed\n", i);
                        return -EIO;
                }

                v.ull &= ~(g_cfg.rmw_mask);
                v.ull |= val;

                if (!WrmsrPGrp(g_cfg.reg, v.u.eax, v.u.edx, g_cfg.proc_group, thread_mask)) {
                        pr_err("CPU%zu write msr failed\n", i);
                        return -EIO;
                }

                if (no_readback)
                        continue;

                v.ull = 0;
                if (!RdmsrPGrp(g_cfg.reg, (DWORD *)&v.u.eax, (DWORD *)&v.u.edx, g_cfg.proc_group, thread_mask)) {
                        pr_err("CPU%zu read msr failed\n", i);
                        return -EIO;
                }

                if (g_cfg.op == MSR_OPS_SETBIT) {
                        v.ull &= BIT_ULL(g_cfg.reg_bit);
                        v.ull = v.ull >> g_cfg.reg_bit;
                        pr_raw("%-8u %-8zu 0x%08x %-10u %-10ju\n", g_cfg.proc_group, i, g_cfg.reg, g_cfg.reg_bit, v.ull);
                } else {
                        result_print(g_cfg.proc_group, i, g_cfg.reg, v.u.edx, v.u.eax);
                }
        }

        return 0;
}

int msr_getbit(void)
{
        union msr_val v;
        uint32_t min, max;

        if (g_cfg.proc_all) {
                min = 0;
                max = nr_proc_pgrp[g_cfg.proc_group] - 1;
        } else {
                min = g_cfg.proc;
                max = g_cfg.proc;
        }

        for (size_t i = min; i <= max; i++) {
                DWORD_PTR thread_mask = BIT_ULL(i);

                if (dry_run) {
                        pr_raw("%-8u %-8zu 0x%08x %10u\n", g_cfg.proc_group, i, g_cfg.reg, g_cfg.reg_bit);
                        continue;
                }

                v.ull = 0;
                if (!RdmsrPGrp(g_cfg.reg, (DWORD *)&v.u.eax, (DWORD *)&v.u.edx, g_cfg.proc_group, thread_mask)) {
                        pr_err("CPU%zu read msr failed\n", i);
                        return -EIO;
                }

                v.ull &= BIT_ULL(g_cfg.reg_bit);
                v.ull = v.ull >> g_cfg.reg_bit;

                pr_raw("%-8u %-8zu 0x%08x %-10u %-10ju\n", g_cfg.proc_group, i, g_cfg.reg, g_cfg.reg_bit, v.ull);
        }

        return 0;
}

int rwmsr(void)
{
        int err = -EINVAL;

        switch (g_cfg.op) {
        case MSR_OPS_READ:
                err = msr_read();
                break;

        case MSR_OPS_WRITE:
                err = msr_write();
                break;

        case MSR_OPS_RMW:
        case MSR_OPS_RMW_MASKED:
                err = msr_rmw();
                break;

        case MSR_OPS_GETBIT:
                err = msr_getbit();
                break;

        case MSR_OPS_SETBIT:
                err = msr_rmw();
                break;

        default:
                break;
        }

        return err;
}

int wmain(int argc, wchar_t *wargv[])
{
        int err;

        setbuf(stdout, NULL);

        g_logprint_colored = 0;

        systopo_init();

        if ((err = wargs_parse(argc, wargv)))
                goto sys_deinit;

        if ((err = winring0_init())) {
                pr_err("failed to load winring0 driver\n");
                goto sys_deinit;
        }

        if (!IsMsr()) {
                pr_err("This CPU does not support MSR\n");
                goto ring_deinit;
        }

        if (!no_column_item) {
                if (g_cfg.op == MSR_OPS_GETBIT || g_cfg.op == MSR_OPS_SETBIT)
                        pr_raw("%-8s %-8s %-10s %-10s %-10s\n", "GROUP", "CPU", "REG", "BIT", "VAL");
                else if (use_long_value)
                        pr_raw("%-8s %-8s %-10s %-10s\n", "GROUP", "CPU", "REG", "VAL");
                else
                        pr_raw("%-8s %-8s %-10s %-10s %-10s\n", "GROUP", "CPU", "REG", "EDX", "EAX");
        }

        if (g_cfg.group_all) {
                for (uint32_t i = 0; i < nr_pgrp; i++) {
                        g_cfg.proc_group = i;
                        err = rwmsr();
                }
        } else {
                err = rwmsr();
        }

ring_deinit:
        winring0_deinit();

sys_deinit:
        systopo_deinit();

        return err;
}
