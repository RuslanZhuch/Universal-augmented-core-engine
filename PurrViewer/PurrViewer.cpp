#include "AppHeaders.h"

#include "imgui/backends/imgui_impl_win32.h"

import RenderCore;
import hfog.Core;
import hfog.Alloc;

#include <vector>

HINSTANCE hInst;

using namespace hfog::MemoryUtils::Literals;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    [[maybe_unused]] _In_opt_ HINSTANCE hPrevInstance,
    [[maybe_unused]] _In_ LPSTR  lpCmdLine,
    _In_ int  nCmdShow
)
{

    const auto windowsClass{ "DesktopApp" };

    WNDCLASSEX wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "DesktopApp";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    hInst = hInstance;
    //constexpr UINT needWidth{ 1366 };
    //constexpr UINT needHeight{ 768 };
    constexpr UINT needWidth{ 800 - 4 };
    constexpr UINT needHeight{ 800 + 20 };

    RECT dims{ 0, 0, needWidth, needHeight };
    AdjustWindowRect(&dims, WS_OVERLAPPEDWINDOW, false);

    HWND hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        windowsClass,
        "Direct3d 12 Test app",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        needWidth, needHeight,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindow failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    RECT cRc;
    GetClientRect(hWnd, &cRc);

    const auto width{ cRc.right - cRc.left };
    const auto height{ cRc.bottom - cRc.top };

    std::vector<byte_t> mainBuffer(10_MB);

    using alloc_t = hfog::Alloc::UnifiedExt<512_B, 8_MB>;
    alloc_t mainAllocator(hfog::MemoryBlock(mainBuffer.data(), mainBuffer.size()));
    Purr::Renderer renderer(&mainAllocator);

    if (!renderer.init(hWnd, width, height))
    {
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (TRUE)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                break;
        }
        else
        {
            renderer.draw(width, height);
        }
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}