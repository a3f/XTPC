#include "xtpc.h"

#include <stdlib.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define unused __attribute__((__unused__))

HWND hwnd;
DWORD WINAPI other_thread(void *arg);
__typeof__(*(WNDPROC)0) WndProc;

#define XTPC_NOTIFY notify
int notify(struct xtpc *xtpc) {
    COPYDATASTRUCT data;
    data.dwData = (ULONG_PTR)&notify;
    data.lpData = xtpc;
    data.cbData = sizeof *xtpc;
    SendMessage(hwnd, WM_COPYDATA, 0, (LPARAM)&data);
    return 0;
}


static __thread const char * thread;
const char *getthread() { return thread; }

int WINAPI WinMain(HINSTANCE unused hInstance, HINSTANCE unused hPrevInstance, LPSTR unused lpCmdLine, int unused nCmdShow) {

    MSG         msg;
    WNDCLASS    wndclass = {0};

    wndclass.lpfnWndProc    = WndProc;
    wndclass.hInstance      = hInstance;
    wndclass.lpszClassName  = "XTPC";

    if (RegisterClass (&wndclass) == 0) {
        fprintf(stderr, "RegisterClass: error code: 0x%lX", GetLastError());
        return 1;
    }


    // spawn XTPC thread
    CreateThread( 
            NULL,         // default security attributes
            0,            // use default stack size  
            other_thread, // thread function name
            hwnd,          // argument to thread function 
            0,            // use default creation flags 
            NULL
            );            // returns the thread identifier 

    hwnd = CreateWindow ("XTPC",
            "XTPC Example",
            WS_OVERLAPPEDWINDOW | WS_CAPTION, /* Window style */
            CW_USEDEFAULT, 0, /* default pos. */
            200, 200, /* default width, height */
            NULL,	/* No parent */
            NULL, 	/* Window class menu */
            hInstance, NULL);
    if (hwnd == NULL) {
        return 1;
    }

    ShowWindow (hwnd, nCmdShow);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;

}
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
        case WM_CHAR:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_LBUTTONUP:
        case WM_COMMAND:
        case WM_CLOSE:
        case WM_PAINT:
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_COPYDATA:;
            COPYDATASTRUCT* data = (COPYDATASTRUCT*)lParam;
            if (data->dwData == (WPARAM)&notify) {
                XTPC_SERVE((struct xtpc*)(data->lpData));
            }
            return 0;

    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}


DWORD WINAPI other_thread(void *hwnd) {
    SendMessage(hwnd, WM_PAINT, 0, 0);

    char line[128];
    while(fgets(line, sizeof line, stdin)) {
#define XTPC_RETURN (void)
        XTPC(puts)("Hello World");
    }

    return 0;
}

