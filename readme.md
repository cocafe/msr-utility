# msr-cmd

------

**msr-cmd** on Windows is something like **wrmsr** from Intel msr-tools in Linux world, which can be used in batch/shell scripts.

It is just a simple cli interface of famous WinRing0 driver.

✔ Systems which have more than 64 cores are supported.

🔶 Supported `rmw`, `{get/set}bit` to make shell scripts easier.

⚠ This tool is intended for power users, if you don't understand fully what is going on, you may shoot your self in the foot.

-----

### ⚠ <u>**ABSOLUTELY NO WARRANTIES, USE ON YOUR OWN RISK**</u> ⚠

------

```shell
Usage:
        msr-cmd.exe [options] read <reg>
        msr-cmd.exe [options] write <reg> <edx(63~32)> <eax(31~0)>
        msr-cmd.exe [options] -l write <reg> <hex_val(63~0)>
        msr-cmd.exe [options] rmw <reg> <h bit> <l bit> <unmasked_hex_val>
        msr-cmd.exe [options] rmwmask <reg> <mask(63~0)> <unmasked_hex_val>
        msr-cmd.exe [options] getbit <reg> <#bit>
        msr-cmd.exe [options] setbit <reg> <#bit> <0/1>

Options:
        -s              write only do not read back
        -d              data only, not to print column item name
        -g <GRP>        processor group (default: 0) to apply
                        'A' or 'a' to apply to all available processors groups
                        by default, a group can contain up to 64 logical processors
        -p <CPU>        logical processor (default: 0) of specified processor group to apply
        -a              operate on all available processors in specified processor group
        -A              operate on all available processors in all available processor groups
        -l              use long 64 bits value for input/output instead of EDX EAX
        -t              dry run

Example:
        msr-cmd.exe -A read 0xc0010015
        msr-cmd.exe -l read 0xc0010015
        msr-cmd.exe write 0xc0010296 0x00808080 0x00404040
        msr-cmd.exe -l write 0xc0010296 0x0080808000404040
        # register 0xc0010294 will be ANDed with NOT of mask 0xff000000 then register will be ORed with 0xee000000
        msr-cmd.exe rmw 0xc0010294 31 24 0xee
        msr-cmd.exe rmwmask 0xc0010294 0xff000000 0xee
        # get value of bit 6 of register 0xc0010296
        msr-cmd.exe getbit 0xc0010296 6
        msr-cmd.exe setbit 0xc0010296 6 1

```

