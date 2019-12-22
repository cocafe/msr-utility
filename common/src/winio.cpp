#include <Windows.h>

#include <winio.h>

#define lpWinIOLoaderTitle L"WinIO Loader"

#pragma comment(lib, "../common/lib/WinIo.lib")

int WinIO_init(void)
{
	if (!InitializeWinIo()) {
		MessageBox(NULL, L"Failed to load WinIO", lpWinIOLoaderTitle, MB_ICONERROR | MB_OK);
		return -EFAULT;
	}

	return 0;
}

void WinIO_deinit(void)
{
	return ShutdownWinIo();
}

int WinIO_install(wchar_t *driver_path)
{
	return InstallWinIoDriver(driver_path, false);
}

int WinIO_remove(void)
{
	return RemoveWinIoDriver();
}
