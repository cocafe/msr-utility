#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <wchar.h>

#include <Windows.h>
#include <windowsx.h>

#include <getopt.h>
#include <winio.h>

static wchar_t *drv_file = L"WinIo64.sys";

void print_help(void)
{
	fprintf_s(stdout, "Usage:\n");
	fprintf_s(stdout, "      winio-drv.exe [options]\n");
	fprintf_s(stdout, "Options:\n");
	fprintf_s(stdout, "      -i            install and configure WinIO to boot with system\n");
	fprintf_s(stdout, "      -r            remove WinIO driver installation\n");
}

int parse_opts(int argc, char *argv[], wchar_t *drv_path)
{
	int ret;
	int c;

	opterr = 0;

	while ((c = getopt(argc, argv, "hir:")) != -1) {
		switch (c) {
			case 'i':
				ret = WinIO_install(drv_path);
				if (ret == false) {
					printf_s("failed to install WinIO driver\n");
				}

				exit(ret);

			case 'r':
				ret = WinIO_remove();
				if (ret == false) {
					printf_s("failed to remove WinIO driver");
				}

				exit(ret);

			case 'h':
				print_help();
				exit(0);

			default:
				break;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	wchar_t drv_path[1024] = { 0 };

	if (argc <= 1) {
		print_help();
		exit(0);
	}

	_wgetcwd(drv_path, sizeof(drv_path) / sizeof(wchar_t));
	_swprintf(drv_path, L"%s\\%s", drv_path, drv_file);

	parse_opts(argc, argv, drv_path);

	return 0;
}
