#include "VulkanApplication.h"

#include "framework.h"

#include <locale>
#include "app.h"

#include "error.h"

constexpr int MAX_LOADSTRING = 100;

static HINSTANCE g_hInst;

ATOM MyRegisterClass(LPCWSTR szWindowClass);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    WCHAR szTitle[MAX_LOADSTRING];
    WCHAR szWindowClass[MAX_LOADSTRING];

    g_hInst = hInstance;

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_VULKANAPPLICATION, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(szWindowClass);

    // constexpr DWORD WINDOW_STYLE = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
    constexpr DWORD WINDOW_STYLE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

    RECT windowRect{};
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = 768;
    windowRect.bottom = 768;
    AdjustWindowRect(&windowRect, WINDOW_STYLE, TRUE);

    // Perform application initialization:
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WINDOW_STYLE, CW_USEDEFAULT, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
                              nullptr, nullptr, g_hInst, nullptr);
    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VULKANAPPLICATION));

    try {
        initApp(hInstance, hWnd);
    }
    catch (const Error& error) {
        MessageBoxA(hWnd, error.what(), "Initialisation Error!", MB_OK | MB_ICONERROR);
        return 0;
    }

    // creates a new game loop thread
    startGameLoop();

    MSG msg{};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // vulkan context is destroyed just before WM_QUIT message is posted

    return static_cast<int>(msg.wParam);
}

ATOM MyRegisterClass(LPCWSTR szWindowClass)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = g_hInst;
    wcex.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_VULKANAPPLICATION));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_VULKANAPPLICATION);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId) {
                case IDM_ABOUT:
                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;
                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
        } break;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            [[maybe_unused]] HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            // SetPixel(hdc, 10, 10, static_cast<COLORREF>(0xFFFFFFFF));
            EndPaint(hWnd, &ps);
        } break;
        case WM_DESTROY:
            // signals the game loop thread to stop and joins it
            endLoopAndShutdown();
            PostQuitMessage(0);
            break;
        case WM_SIZE:
            // window resized
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}
