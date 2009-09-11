#define _WIN32_IE	0x0300
#include <windows.h>
#include <commctrl.h>

#include <stdio.h>
#include <fcntl.h>
#include <time.h>

void enter_temp_dir() {
	char temppath[1024];

	do {
		GetTempPath(1024, temppath);

		char tempname[64];
		snprintf(tempname, 63, "sfx%03o_%08x_%d",
			GetCurrentProcessId(),
			time(NULL),
			rand()
		);

		strcat(temppath, tempname);
	} while (!CreateDirectory(temppath, NULL));

	chdir(temppath);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX wc;
	HWND hwnd, hprogress, hdesktop;

	/* find desktop dimensions, to center the window on screen */
	hdesktop = GetDesktopWindow();
	RECT rdesktop;
	GetWindowRect(hdesktop, &rdesktop);

	/* prepare window dimensions */
	int pb_left, pb_top, pb_width, pb_height;
	pb_width = 400;
	pb_height = 20;
	pb_left = (rdesktop.right - pb_width) / 2;
	pb_top = (rdesktop.bottom - pb_height) / 2;

	/* Create application window */
	wc.cbSize            = sizeof(WNDCLASSEX);
	wc.style             = 0;
	wc.lpfnWndProc       = DefWindowProc;
	wc.cbClsExtra        = 0;
	wc.cbWndExtra        = 0;
	wc.hInstance         = hInstance;
	wc.hIcon             = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor           = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground     = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName      = NULL;
	wc.lpszClassName     = "SfxProgressWindow";
	wc.hIconSm           = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window registration failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(
		0,
		"SfxProgressWindow",
		"Sfx Archive Extracting",
		WS_POPUP | WS_VISIBLE,
		pb_left, pb_top, pb_width, pb_height,
		NULL, NULL, hInstance, NULL
	);

	if (hwnd == NULL) {
		MessageBox(NULL, "Window creation failed", "err", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	/* Initialize commctrl */
	INITCOMMONCONTROLSEX InitCtrlEx;
	InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCtrlEx.dwICC  = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&InitCtrlEx);

	/* Create the progress bar */
	hprogress = CreateWindowEx(
		0,
		PROGRESS_CLASS,
		NULL,
		WS_CHILD | WS_VISIBLE,
		0, 0, pb_width, pb_height,
		hwnd, NULL, hInstance, NULL
	);
	UpdateWindow(hprogress);

	/* Load the data archive */
	HRSRC res = FindResource(NULL, "SFXDATA", RT_RCDATA);
	char *data = LockResource(LoadResource(NULL, res));
	int data_len = SizeofResource(NULL, res);
	int data_pos = 0;
	int in_file = -1;
	int in_file_rest = 0;

	/* make a temporary directory and chdir to it */
	enter_temp_dir();

	while (data_pos < data_len) {
		if (in_file == -1) {
			/* collect file name and size */
			char *fn = data + data_pos;
			int fsize = strtol(data + data_pos + 124, NULL, 8);
			int ftype = data[data_pos + 156];
			printf("%s %c %d\n", fn, ftype, fsize);
			fflush(stdout);

			/* do stuff depending on the type */
			switch (ftype) {
			case '0': /* file */
				in_file = open(fn, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY);

				if (in_file == -1) {
					char errstr[1024];
					strncpy(errstr, strerror(errno), 1023);
					char message[2048] = "";
					strcat(message, "Extracting ");
					strcat(message, fn);
					strcat(message, " failed: ");
					strcat(message, errstr);
					MessageBox(NULL, message, "Sfx: Error while extracting", MB_ICONERROR | MB_OK);
					exit(1);
				}
				in_file_rest = fsize;

				/* properly handle empty files */
				if (in_file_rest == 0) {
					close(in_file);
					in_file = -1;
				}
				break;
			case '5': /* directory */
				mkdir(fn);
				break;
			default:
				fprintf(stderr, "Unimplemented file type %c for %s\n", ftype, fn);
			}
		} else {
			/* write more data to output file */
			if (in_file_rest > 512) {
				write(in_file, data + data_pos, 512);
				in_file_rest -= 512;
			} else {
				write(in_file, data + data_pos, in_file_rest);
				in_file_rest = 0;
				close(in_file);
				in_file = -1;
			}
		}

		data_pos += 512;
		SendMessage(hprogress, PBM_SETPOS, (int)(((double)data_pos / data_len) * 100.0), 0);
		UpdateWindow(hprogress);
	}
}
