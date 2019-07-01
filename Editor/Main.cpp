#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINDOWS 0x0501 // Enable mouse wheel scrolling.

#pragma comment(lib, "deps/DXSDK/lib/d3dx8.lib")
#pragma comment(lib, "deps/DXSDK/lib/d3d8.lib")
#pragma comment(lib, "deps/Assimp/lib/assimp-vc140-mt.lib")

#include <tchar.h>
#include "handlers/Window.h"

const int windowWidth = 800;
const int windowHeight = 600;
const TCHAR szWindowClass[] = APP_NAME;
const TCHAR szTitle[] = _T("Loading");

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    if (LoadLibrary("SciLexer.dll") == NULL)
    {
        MessageBox(NULL, "Could not load SciLexer.dll", "Error", NULL);
        return 1;
    }

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = UltraEd::WindowHandler;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN_MENU);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, "Call to RegisterClassEx failed!", "Error", NULL);
        return 1;
    }

    // Create the main window which we'll add the toolbar and renderer to.
    UltraEd::parentWindow = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        (GetSystemMetrics(SM_CXSCREEN) / 2) - (windowWidth / 2),
        (GetSystemMetrics(SM_CYSCREEN) / 2) - (windowHeight / 2),
        windowWidth, windowHeight, NULL, NULL, hInstance, NULL);

    if (!UltraEd::parentWindow)
    {
        MessageBox(NULL, "Could not create parent window.", "Error", NULL);
        return 1;
    }

    UltraEd::toolbarWindow = UltraEd::CreateToolbar(UltraEd::parentWindow, hInstance);

    if (!UltraEd::toolbarWindow)
    {
        MessageBox(NULL, "Could not create toolbar.", "Error", NULL);
        return 1;
    }

    UltraEd::statusBar = CreateStatusWindow(WS_VISIBLE | WS_CHILD, "Welcome to UltraEd", 
        UltraEd::parentWindow, IDM_STATUS_BAR);

    if (!UltraEd::statusBar)
    {
        MessageBox(NULL, "Could not create status bar.", "Error", NULL);
        return 1;
    }

    ShowWindow(UltraEd::parentWindow, nCmdShow);

    // Create treeview that shows objects in scene.   
    RECT parentRect;
    GetClientRect(UltraEd::parentWindow, &parentRect);
    RECT toolbarRect;
    GetClientRect(UltraEd::toolbarWindow, &toolbarRect);
    RECT statusRect;
    GetClientRect(UltraEd::statusBar, &statusRect);
    UltraEd::treeview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, TEXT("Tree View"),
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES,
        0, toolbarRect.bottom + UltraEd::treeviewBorder,
        UltraEd::treeviewWidth, parentRect.bottom - (toolbarRect.bottom + statusRect.bottom + UltraEd::treeviewBorder),
        UltraEd::parentWindow, (HMENU)IDM_TREEVIEW, hInstance, NULL);

    if (!UltraEd::treeview)
    {
        MessageBox(NULL, "Could not create treeview.", "Error", NULL);
        return 1;
    }

    // Create the window for rendering the scene.
    RECT treeviewRect;
    GetClientRect(UltraEd::treeview, &treeviewRect);
    UltraEd::renderWindow = CreateWindow(szWindowClass, szTitle, WS_CLIPSIBLINGS | WS_CHILD,
        treeviewRect.right + UltraEd::treeviewBorder, 0,
        parentRect.right - treeviewRect.right, parentRect.bottom - statusRect.bottom,
        UltraEd::parentWindow, NULL, hInstance, NULL);

    if (!UltraEd::renderWindow)
    {
        MessageBox(NULL, "Could not create render child window.", "Error", NULL);
        return 1;
    }

    ShowWindow(UltraEd::renderWindow, nCmdShow);

    UltraEd::CScene scene;
    if (!scene.Create(UltraEd::renderWindow))
    {
        MessageBox(NULL, "Could not create Direct3D device.", "Error", NULL);
        return 1;
    }

    // Pass scene pointers to handles that need it during event handling.
    SetWindowLongPtr(UltraEd::renderWindow, GWLP_USERDATA, (LPARAM)&scene);
    SetWindowLongPtr(UltraEd::parentWindow, GWLP_USERDATA, (LPARAM)&scene);

    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            scene.Render();
        }
    }

    return msg.wParam;
}