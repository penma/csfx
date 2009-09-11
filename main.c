#define _WIN32_IE	0x0300
#include <windows.h>
#include <commctrl.h>

#include <stdio.h>
#include <fcntl.h>

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
		"",
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
	
	/* Load the compressed data */
	char *compressed_data = LockResource(LoadResource(NULL, FindResource(NULL, "SFXDATA", RT_RCDATA)));
	
	char *data = compressed_data;
	int data_len = 30341632;
	int data_pos = 0;
	int in_file = -1;
	int in_file_rest = 0;
	
	chdir("C:\\extract");
	
	while (1) {
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
					perror(fn);
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
		//Sleep(30);
	}
}