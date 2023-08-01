#ifndef __LIB_WINRING0_H__
#define __LIB_WINRING0_H__

#include <windows.h>

#include "OlsDef.h"

extern DWORD WINAPI (*GetDllStatus)();
extern DWORD WINAPI (*GetDllVersion)(PBYTE major, PBYTE minor, PBYTE revision, PBYTE release);
extern DWORD WINAPI (*GetDriverVersion)(PBYTE major, PBYTE minor, PBYTE revision, PBYTE release);
extern DWORD WINAPI (*GetDriverType)();
extern BOOL WINAPI (*InitializeOls)();
extern VOID WINAPI (*DeinitializeOls)();
extern BOOL WINAPI (*IsCpuid)();
extern BOOL WINAPI (*IsMsr)();
extern BOOL WINAPI (*IsTsc)();
extern BOOL WINAPI (*Rdmsr)(DWORD index, PDWORD eax, PDWORD edx);
extern BOOL WINAPI (*RdmsrTx)(DWORD index, PDWORD eax, PDWORD edx, DWORD_PTR threadAffinityMask);
extern BOOL WINAPI (*RdmsrPx)(DWORD index, PDWORD eax, PDWORD edx, DWORD_PTR processAffinityMask);
extern BOOL WINAPI (*Wrmsr)(DWORD index, DWORD eax, DWORD edx);
extern BOOL WINAPI (*WrmsrTx)(DWORD index, DWORD eax, DWORD edx, DWORD_PTR threadAffinityMask);
extern BOOL WINAPI (*WrmsrPx)(DWORD index, DWORD eax, DWORD edx, DWORD_PTR processAffinityMask);
extern BOOL WINAPI (*Rdpmc)(DWORD index, PDWORD eax, PDWORD edx);
extern BOOL WINAPI (*RdpmcTx)(DWORD index, PDWORD eax, PDWORD edx, DWORD_PTR threadAffinityMask);
extern BOOL WINAPI (*RdpmcPx)(DWORD index, PDWORD eax, PDWORD edx, DWORD_PTR processAffinityMask);
extern BOOL WINAPI (*Cpuid)(DWORD index, PDWORD eax, PDWORD ebx, PDWORD ecx, PDWORD edx);
extern BOOL WINAPI (*CpuidTx)(DWORD index, PDWORD eax, PDWORD ebx, PDWORD ecx, PDWORD edx, DWORD_PTR threadAffinityMask);
extern BOOL WINAPI (*CpuidPx)(DWORD index, PDWORD eax, PDWORD ebx, PDWORD ecx, PDWORD edx, DWORD_PTR processAffinityMask);
extern BOOL WINAPI (*Rdtsc)(PDWORD eax, PDWORD edx);
extern BOOL WINAPI (*RdtscTx)(PDWORD eax, PDWORD edx, DWORD_PTR threadAffinityMask);
extern BOOL WINAPI (*RdtscPx)(PDWORD eax, PDWORD edx, DWORD_PTR processAffinityMask);
extern BOOL WINAPI (*Hlt)();
extern BOOL WINAPI (*HltTx)(DWORD_PTR threadAffinityMask);
extern BOOL WINAPI (*HltPx)(DWORD_PTR processAffinityMask);
extern BYTE WINAPI (*ReadIoPortByte)(WORD port);
extern WORD WINAPI (*ReadIoPortWord)(WORD port);
extern DWORD WINAPI (*ReadIoPortDword)(WORD port);
extern BOOL WINAPI (*ReadIoPortByteEx)(WORD port, PBYTE value);
extern BOOL WINAPI (*ReadIoPortWordEx)(WORD port, PWORD value);
extern BOOL WINAPI (*ReadIoPortDwordEx)(WORD port, PDWORD value);
extern VOID WINAPI (*WriteIoPortByte)(WORD port, BYTE value);
extern VOID WINAPI (*WriteIoPortDword)(WORD port, DWORD value);
extern VOID WINAPI (*WriteIoPortWord)(WORD port, WORD value);
extern BOOL WINAPI (*WriteIoPortByteEx)(WORD port, BYTE value);
extern BOOL WINAPI (*WriteIoPortWordEx)(WORD port, WORD value);
extern BOOL WINAPI (*WriteIoPortDwordEx)(WORD port, DWORD value);
extern VOID WINAPI (*SetPciMaxBusIndex)(BYTE max);
extern BYTE WINAPI (*ReadPciConfigByte)(DWORD pciAddress, BYTE regAddress);
extern WORD WINAPI (*ReadPciConfigWord)(DWORD pciAddress, BYTE regAddress);
extern DWORD WINAPI (*ReadPciConfigDword)(DWORD pciAddress, BYTE regAddress);
extern BOOL WINAPI (*ReadPciConfigByteEx)(DWORD pciAddress, DWORD regAddress, PBYTE value);
extern BOOL WINAPI (*ReadPciConfigWordEx)(DWORD pciAddress, DWORD regAddress, PWORD value);
extern BOOL WINAPI (*ReadPciConfigDwordEx)(DWORD pciAddress, DWORD regAddress, PDWORD value);
extern VOID WINAPI (*WritePciConfigByte)(DWORD pciAddress, BYTE regAddress, BYTE value);
extern VOID WINAPI (*WritePciConfigWord)(DWORD pciAddress, BYTE regAddress, WORD value);
extern VOID WINAPI (*WritePciConfigDword)(DWORD pciAddress, BYTE regAddress, DWORD value);
extern BOOL WINAPI (*WritePciConfigByteEx)(DWORD pciAddress, DWORD regAddress, BYTE value);
extern BOOL WINAPI (*WritePciConfigWordEx)(DWORD pciAddress, DWORD regAddress, WORD value);
extern BOOL WINAPI (*WritePciConfigDwordEx)(DWORD pciAddress, DWORD regAddress, DWORD value);
extern DWORD WINAPI (*FindPciDeviceById)(WORD vendorId, WORD deviceId, BYTE index);
extern DWORD WINAPI (*FindPciDeviceByClass)(BYTE baseClass, BYTE subClass, BYTE programIf, BYTE index);

BOOL WINAPI RdmsrPGrp(DWORD index, PDWORD eax, PDWORD edx, DWORD grp, DWORD_PTR aff_mask);
BOOL WINAPI WrmsrPGrp(DWORD index, DWORD eax, DWORD edx, DWORD grp, DWORD_PTR aff_mask);

int winring0_init(void);
void winring0_deinit(void);

#endif // __LIB_WINRING0_H__