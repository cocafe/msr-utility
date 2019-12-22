#pragma once

#include "../lib/WinIO/Source/Dll/winio.h"

int WinIO_init(void);
void WinIO_deinit(void);

int WinIO_install(wchar_t *driver_path);
int WinIO_remove(void);
