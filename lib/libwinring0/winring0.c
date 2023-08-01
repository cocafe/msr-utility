#include <stdint.h>
#include <errno.h>

#include <windows.h>
#include <processthreadsapi.h>

#include "winring0.h"

DWORD WINAPI (*GetDllStatus)();
DWORD WINAPI (*GetDllVersion)(PBYTE major, PBYTE minor, PBYTE revision, PBYTE release);
DWORD WINAPI (*GetDriverVersion)(PBYTE major, PBYTE minor, PBYTE revision, PBYTE release);
DWORD WINAPI (*GetDriverType)();
BOOL WINAPI (*InitializeOls)();
VOID WINAPI (*DeinitializeOls)();
BOOL WINAPI (*IsCpuid)();
BOOL WINAPI (*IsMsr)();
BOOL WINAPI (*IsTsc)();
BOOL WINAPI (*Rdmsr)(DWORD index, PDWORD eax, PDWORD edx);
BOOL WINAPI (*RdmsrTx)(DWORD index, PDWORD eax, PDWORD edx, DWORD_PTR threadAffinityMask);
BOOL WINAPI (*RdmsrPx)(DWORD index, PDWORD eax, PDWORD edx, DWORD_PTR processAffinityMask);
BOOL WINAPI (*Wrmsr)(DWORD index, DWORD eax, DWORD edx);
BOOL WINAPI (*WrmsrTx)(DWORD index, DWORD eax, DWORD edx, DWORD_PTR threadAffinityMask);
BOOL WINAPI (*WrmsrPx)(DWORD index, DWORD eax, DWORD edx, DWORD_PTR processAffinityMask);
BOOL WINAPI (*Rdpmc)(DWORD index, PDWORD eax, PDWORD edx);
BOOL WINAPI (*RdpmcTx)(DWORD index, PDWORD eax, PDWORD edx, DWORD_PTR threadAffinityMask);
BOOL WINAPI (*RdpmcPx)(DWORD index, PDWORD eax, PDWORD edx, DWORD_PTR processAffinityMask);
BOOL WINAPI (*Cpuid)(DWORD index, PDWORD eax, PDWORD ebx, PDWORD ecx, PDWORD edx);
BOOL WINAPI (*CpuidTx)(DWORD index, PDWORD eax, PDWORD ebx, PDWORD ecx, PDWORD edx, DWORD_PTR threadAffinityMask);
BOOL WINAPI (*CpuidPx)(DWORD index, PDWORD eax, PDWORD ebx, PDWORD ecx, PDWORD edx, DWORD_PTR processAffinityMask);
BOOL WINAPI (*Rdtsc)(PDWORD eax, PDWORD edx);
BOOL WINAPI (*RdtscTx)(PDWORD eax, PDWORD edx, DWORD_PTR threadAffinityMask);
BOOL WINAPI (*RdtscPx)(PDWORD eax, PDWORD edx, DWORD_PTR processAffinityMask);
BOOL WINAPI (*Hlt)();
BOOL WINAPI (*HltTx)(DWORD_PTR threadAffinityMask);
BOOL WINAPI (*HltPx)(DWORD_PTR processAffinityMask);
BYTE WINAPI (*ReadIoPortByte)(WORD port);
WORD WINAPI (*ReadIoPortWord)(WORD port);
DWORD WINAPI (*ReadIoPortDword)(WORD port);
BOOL WINAPI (*ReadIoPortByteEx)(WORD port, PBYTE value);
BOOL WINAPI (*ReadIoPortWordEx)(WORD port, PWORD value);
BOOL WINAPI (*ReadIoPortDwordEx)(WORD port, PDWORD value);
VOID WINAPI (*WriteIoPortByte)(WORD port, BYTE value);
VOID WINAPI (*WriteIoPortDword)(WORD port, DWORD value);
VOID WINAPI (*WriteIoPortWord)(WORD port, WORD value);
BOOL WINAPI (*WriteIoPortByteEx)(WORD port, BYTE value);
BOOL WINAPI (*WriteIoPortWordEx)(WORD port, WORD value);
BOOL WINAPI (*WriteIoPortDwordEx)(WORD port, DWORD value);
VOID WINAPI (*SetPciMaxBusIndex)(BYTE max);
BYTE WINAPI (*ReadPciConfigByte)(DWORD pciAddress, BYTE regAddress);
WORD WINAPI (*ReadPciConfigWord)(DWORD pciAddress, BYTE regAddress);
DWORD WINAPI (*ReadPciConfigDword)(DWORD pciAddress, BYTE regAddress);
BOOL WINAPI (*ReadPciConfigByteEx)(DWORD pciAddress, DWORD regAddress, PBYTE value);
BOOL WINAPI (*ReadPciConfigWordEx)(DWORD pciAddress, DWORD regAddress, PWORD value);
BOOL WINAPI (*ReadPciConfigDwordEx)(DWORD pciAddress, DWORD regAddress, PDWORD value);
VOID WINAPI (*WritePciConfigByte)(DWORD pciAddress, BYTE regAddress, BYTE value);
VOID WINAPI (*WritePciConfigWord)(DWORD pciAddress, BYTE regAddress, WORD value);
VOID WINAPI (*WritePciConfigDword)(DWORD pciAddress, BYTE regAddress, DWORD value);
BOOL WINAPI (*WritePciConfigByteEx)(DWORD pciAddress, DWORD regAddress, BYTE value);
BOOL WINAPI (*WritePciConfigWordEx)(DWORD pciAddress, DWORD regAddress, WORD value);
BOOL WINAPI (*WritePciConfigDwordEx)(DWORD pciAddress, DWORD regAddress, DWORD value);
DWORD WINAPI (*FindPciDeviceById)(WORD vendorId, WORD deviceId, BYTE index);
DWORD WINAPI (*FindPciDeviceByClass)(BYTE baseClass, BYTE subClass, BYTE programIf, BYTE index);

HMODULE winring0_dll;

struct symbol {
        char *name;
        void *ptr;
};

struct symbol symbol_list[] = {
        { "GetDllStatus",              &GetDllStatus },
        { "GetDllVersion",             &GetDllVersion },
        { "GetDriverVersion",          &GetDriverVersion },
        { "GetDriverType",             &GetDriverType },
        { "InitializeOls",             &InitializeOls },
        { "DeinitializeOls",           &DeinitializeOls },
        { "IsCpuid",                   &IsCpuid },
        { "IsMsr",                     &IsMsr },
        { "IsTsc",                     &IsTsc },
        { "Rdmsr",                     &Rdmsr },
        { "RdmsrTx",                   &RdmsrTx },
        { "RdmsrPx",                   &RdmsrPx },
        { "Wrmsr",                     &Wrmsr },
        { "WrmsrTx",                   &WrmsrTx },
        { "WrmsrPx",                   &WrmsrPx },
        { "Rdpmc",                     &Rdpmc },
        { "RdpmcTx",                   &RdpmcTx },
        { "RdpmcPx",                   &RdpmcPx },
        { "Cpuid",                     &Cpuid },
        { "CpuidTx",                   &CpuidTx },
        { "CpuidPx",                   &CpuidPx },
        { "Rdtsc",                     &Rdtsc },
        { "RdtscTx",                   &RdtscTx },
        { "RdtscPx",                   &RdtscPx },
        { "Hlt",                       &Hlt },
        { "HltTx",                     &HltTx },
        { "HltPx",                     &HltPx },
        { "ReadIoPortByte",            &ReadIoPortByte },
        { "ReadIoPortWord",            &ReadIoPortWord },
        { "ReadIoPortDword",           &ReadIoPortDword },
        { "ReadIoPortByteEx",          &ReadIoPortByteEx },
        { "ReadIoPortWordEx",          &ReadIoPortWordEx },
        { "ReadIoPortDwordEx",         &ReadIoPortDwordEx },
        { "WriteIoPortByte",           &WriteIoPortByte },
        { "WriteIoPortDword",          &WriteIoPortDword },
        { "WriteIoPortWord",           &WriteIoPortWord },
        { "WriteIoPortByteEx",         &WriteIoPortByteEx },
        { "WriteIoPortWordEx",         &WriteIoPortWordEx },
        { "WriteIoPortDwordEx",        &WriteIoPortDwordEx },
        { "SetPciMaxBusIndex",         &SetPciMaxBusIndex },
        { "ReadPciConfigByte",         &ReadPciConfigByte },
        { "ReadPciConfigWord",         &ReadPciConfigWord },
        { "ReadPciConfigDword",        &ReadPciConfigDword },
        { "ReadPciConfigByteEx",       &ReadPciConfigByteEx },
        { "ReadPciConfigWordEx",       &ReadPciConfigWordEx },
        { "ReadPciConfigDwordEx",      &ReadPciConfigDwordEx },
        { "WritePciConfigByte",        &WritePciConfigByte },
        { "WritePciConfigWord",        &WritePciConfigWord },
        { "WritePciConfigDword",       &WritePciConfigDword },
        { "WritePciConfigByteEx",      &WritePciConfigByteEx },
        { "WritePciConfigWordEx",      &WritePciConfigWordEx },
        { "WritePciConfigDwordEx",     &WritePciConfigDwordEx },
        { "FindPciDeviceById",         &FindPciDeviceById },
        { "FindPciDeviceByClass",      &FindPciDeviceByClass },
};

BOOL WINAPI RdmsrPGrp(DWORD index, PDWORD eax, PDWORD edx, DWORD grp, DWORD_PTR aff_mask)
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

BOOL WINAPI WrmsrPGrp(DWORD index, DWORD eax, DWORD edx, DWORD grp, DWORD_PTR aff_mask)
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

int winring0_load(void)
{
        int err = 0;

        winring0_dll = LoadLibrary(L"WinRing0x64.dll");
        if (winring0_dll == NULL) {

                return -ENODATA;
        }

        for (size_t i = 0; i < sizeof(symbol_list) / sizeof(symbol_list[0]); i++) {
                struct symbol *sym = &symbol_list[i];

                *((intptr_t *)(sym->ptr)) = (intptr_t)GetProcAddress(winring0_dll, sym->name);

                if (*((intptr_t *)(sym->ptr)) == (intptr_t)NULL) {
                        err = -ENOENT;
                        goto err;
                }
        }

        return err;

err:
        FreeLibrary(winring0_dll);

        return err;
}

int winring0_init(void)
{
        DWORD ret;
        int err = 0;

        if ((err = winring0_load()))
                return err;

        if (FALSE == InitializeOls())
                return -EFAULT;

        ret = GetDllStatus();
        if (ret != OLS_DLL_NO_ERROR) {
                err = -EFAULT;
                goto dll_deinit;
        }

        return err;

dll_deinit:
        DeinitializeOls();

        return err;
}

void winring0_deinit(void)
{
        DeinitializeOls();

        if (winring0_dll)
                FreeLibrary(winring0_dll);
}