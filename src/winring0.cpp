#include <Windows.h>

#include <winring0.h>

#define lpLoaderName L"WinRing0 Loader"

#pragma comment(lib, "../lib/WinRing0x64.lib")

int WinRing0_init(void)
{
	if (!InitializeOls()) {
		MessageBox(NULL,
			   L"Failed to load DLL",
			   lpLoaderName,
			   MB_ICONERROR | MB_OK);
		return -EFAULT;
	}

	switch (GetDllStatus())
	{
		case OLS_DLL_NO_ERROR:
			break;

		case OLS_DLL_UNSUPPORTED_PLATFORM:
			MessageBox(NULL,
				   L"DLL Status Error!! UNSUPPORTED_PLATFORM",
				   lpLoaderName,
				   MB_ICONERROR | MB_OK);
			goto dll_deinit;

		case OLS_DLL_DRIVER_NOT_LOADED:
			MessageBox(NULL,
				   L"DLL Status Error!! DRIVER_NOT_LOADED",
				   lpLoaderName,
				   MB_ICONERROR | MB_OK);
			goto dll_deinit;

		case OLS_DLL_DRIVER_NOT_FOUND:
			MessageBox(NULL,
				   L"DLL Status Error!! DRIVER_NOT_FOUND",
				   lpLoaderName,
				   MB_ICONERROR | MB_OK);
			goto dll_deinit;

		case OLS_DLL_DRIVER_UNLOADED:
			MessageBox(NULL,
				   L"DLL Status Error!! DRIVER_UNLOADED",
				   lpLoaderName,
				   MB_ICONERROR | MB_OK);
			goto dll_deinit;

		case OLS_DLL_DRIVER_NOT_LOADED_ON_NETWORK:
			MessageBox(NULL,
				   L"DLL Status Error!! DRIVER_NOT_LOADED_ON_NETWORK",
				   lpLoaderName,
				   MB_ICONERROR | MB_OK);
			goto dll_deinit;

		case OLS_DLL_UNKNOWN_ERROR:
		default:
			MessageBox(NULL,
				   L"DLL Status Error!! UNKNOWN_ERROR",
				   lpLoaderName,
				   MB_ICONERROR | MB_OK);
			goto dll_deinit;
	}

	return 0;

dll_deinit:
	DeinitializeOls();

	return -EFAULT;
}

void WinRing0_deinit(void)
{
	DeinitializeOls();
}