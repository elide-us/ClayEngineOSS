#include "pch.h"

#include "WindowSystem.h"

#pragma region ClayEngineClientCore Implementation
//ClayEngine::Platform::ClayEngineClientCore::ClayEngineClientCore(HINSTANCE hInstance, int nCmdShow) {}
//ClayEngine::Platform::ClayEngineClientCore::~ClayEngineClientCore() {}
//HWND ClayEngine::Platform::ClayEngineClientCore::GetWindowHandle() {}
//int ClayEngine::Platform::ClayEngineClientCore::Run() {}
#pragma endregion

#pragma region WindowSystem Implementation
ClayEngine::Platform::WindowSystem::WindowSystem(HINSTANCE hInstance, int nCmdShow, Unicode className, Unicode windowName)
    : m_instance_handle(hInstance), m_show_flags(nCmdShow), m_class_name(className), m_window_name(windowName)
{
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = m_instance_handle;
    wcex.hIcon = NULL;
    wcex.hIconSm = NULL;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = m_class_name.c_str();

    ThrowIfFailed(RegisterClassExW(&wcex));

    m_window_handle = CreateWindowW(m_class_name.c_str(), m_window_name.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, m_instance_handle, nullptr);

    if (!m_window_handle) throw;
    ShowWindow(m_window_handle, m_show_flags);
    //ShowCursor(true);
    //SetWindowPos(m_window_handle, etc...);
    //SetWindowTextW(m_window_handle, L"");

    SetWindowLongPtrW(m_window_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    UpdateWindow(m_window_handle);
}

ClayEngine::Platform::WindowSystem::~WindowSystem()
{
    DestroyWindow(m_window_handle);
    UnregisterClassW(m_class_name.c_str(), m_instance_handle);
}

//int ClayEngine::Platform::WindowSystem::Run()
//{
//    MSG msg = {};
//    while (msg.message != WM_QUIT)
//    {
//        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
//        {
//            TranslateMessage(&msg);
//            DispatchMessage(&msg);
//        }
//    }
//
//    return (int)msg.wParam;
//}
#pragma endregion

#pragma region ClayEngineClientCore WndProc Implementation
LRESULT CALLBACK ClayEngine::Platform::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto context = reinterpret_cast<WindowSystem*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (!context)
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_MENUCHAR: // Supress the menu
        return MAKELRESULT(0, MNC_CLOSE);
    case WM_PAINT:
        if (!context->GetInSizeMove())
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_GETMINMAXINFO:
        if (lParam)
        {
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = c_min_window_width;
            info->ptMinTrackSize.y = c_min_window_height;
        }
        break;
    case WM_ACTIVATEAPP:
        // These messages get sent to all windows in the process on cerntain events, we'll
        // need to make sure we are only procesing the message for windows that are relevant
        if (wParam)
        {
            context->OnActivated();
        }
        else
        {
            context->OnDeactivated();
        }
        break;
    case WM_POWERBROADCAST:
        switch (wParam)
        {
        case PBT_APMQUERYSUSPEND:
            if (!context->GetInSuspend())
                context->OnSuspending();
            context->SetInSuspend(true);
            return TRUE;
        case PBT_APMRESUMESUSPEND:
            if (!context->GetMinimized())
            {
                if (context->GetInSuspend())
                    context->OnResuming();
                context->SetInSuspend(false);
            }
            return TRUE;
        }
        break;
    case WM_ENTERSIZEMOVE:
        context->SetInSizeMove(true);
        break;
    case WM_EXITSIZEMOVE:
        context->SetInSizeMove(false);
        {
            context->UpdateWindowSize();
        }
        break;
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
        {
            if (!context->GetMinimized())
            {
                context->SetMinimized(true);
                if (!context->GetInSuspend())
                    context->OnSuspending();
                context->SetInSuspend(true);
            }
        }
        else if (context->GetMinimized())
        {
            context->SetMinimized(false);
            if (context->GetInSuspend())
                context->OnResuming();
            context->SetInSuspend(false);
        }
        else if (!context->GetInSizeMove())
        {
            context->UpdateWindowSize();
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
#pragma endregion
