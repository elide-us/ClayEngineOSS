#include "pch.h"

#include "WindowSystem.h"

#pragma region WindowSystem WndProc Implementation
namespace ClayEngine
{
    constexpr auto c_min_window_width = 320U;
    constexpr auto c_min_window_height = 200U;

    LRESULT CALLBACK ClayEngineWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
                context->OnChanged();
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
                context->OnChanged();
            }
            break;
        case WM_CHAR:
            context->OnChar(wParam, lParam);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }
}
#pragma endregion

#pragma region WindowSystem Implementation
ClayEngine::WindowSystem::WindowSystem(HINSTANCE hInstance, int nCmdShow, Unicode className, Unicode windowName)
    : m_instance_handle(hInstance), m_show_flags(nCmdShow), m_class_name(className), m_window_name(windowName)
{
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = ClayEngine::ClayEngineWndProc;
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
    if (!m_window_handle) throw std::exception("CreateWindowW() ERROR: Window handle null");

    ShowWindow(m_window_handle, m_show_flags);
    ShowCursor(true);
    SetWindowTextW(m_window_handle, m_window_name.c_str());
    SetWindowLongPtrW(m_window_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    UpdateWindow(m_window_handle);
}

ClayEngine::WindowSystem::~WindowSystem()
{
    DestroyWindow(m_window_handle);
    UnregisterClassW(m_class_name.c_str(), m_instance_handle);
}
#pragma endregion

