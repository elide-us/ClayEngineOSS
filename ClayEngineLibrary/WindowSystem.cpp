#include "pch.h"

#include "WindowSystem.h"

#pragma region WindowSystem WndProc Implementation
namespace ClayEngine
{
    constexpr auto c_min_window_width = 320U;
    constexpr auto c_min_window_height = 200U;

    LRESULT CALLBACK ClayEngineWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto _ctx = reinterpret_cast<WindowSystem*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (!_ctx)
        {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }

        switch (message)
        {

        case WM_MENUCHAR: // Supress the menu
            return MAKELRESULT(0, MNC_CLOSE);
        case WM_PAINT:
            if (!_ctx->GetInSizeMove())
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
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
            // These messages get sent to all windows in the process on cerntain events, we'll
            // need to make sure we are only procesing the message for windows that are relevant
            _ctx->OnMouesMessage(message, wParam, lParam);
            if (wParam)
            {
                _ctx->OnActivated();
            }
            else
            {
                _ctx->OnDeactivated();
            }
            break;
        case WM_POWERBROADCAST:
            switch (wParam)
            {
            case PBT_APMQUERYSUSPEND:
                if (!_ctx->GetInSuspend())
                    _ctx->OnSuspending();
                _ctx->SetInSuspend(true);
                return TRUE;
            case PBT_APMRESUMESUSPEND:
                if (!_ctx->GetMinimized())
                {
                    if (_ctx->GetInSuspend())
                        _ctx->OnResuming();
                    _ctx->SetInSuspend(false);
                }
                return TRUE;
            }
            break;
        case WM_ENTERSIZEMOVE:
            _ctx->SetInSizeMove(true);
            break;
        case WM_EXITSIZEMOVE:
            _ctx->SetInSizeMove(false);
            {
                _ctx->OnChanged();
            }
            break;
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED)
            {
                if (!_ctx->GetMinimized())
                {
                    _ctx->SetMinimized(true);
                    if (!_ctx->GetInSuspend())
                        _ctx->OnSuspending();
                    _ctx->SetInSuspend(true);
                }
            }
            else if (_ctx->GetMinimized())
            {
                _ctx->SetMinimized(false);
                if (_ctx->GetInSuspend())
                    _ctx->OnResuming();
                _ctx->SetInSuspend(false);
            }
            else if (!_ctx->GetInSizeMove())
            {
                _ctx->OnChanged();
            }
            break;
        case WM_SYSKEYUP:
        case WM_KEYUP:
            _ctx->OnKeyUp(wParam, lParam);
            break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            _ctx->OnKeyDown(wParam, lParam);
            break;
        case WM_CHAR:
            _ctx->OnChar(wParam, lParam);
            break;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_INPUT:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEHOVER:
            _ctx->OnMouesMessage(message, wParam, lParam);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;

#pragma region Older reference code
        //case WM_CREATE:
        //    if (lParam)
        //    {
        //        auto params = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        //        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params->lpCreateParams));
        //    }
        //    break;

        //  // Implements the classic ALT+ENTER fullscreen toggle
        //  case WM_SYSKEYDOWN:
        //  if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
        //  {
        //	    if (s_fullscreen)
        //	    {
        //		    SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        //		    SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);
        //		    int width = 800;
        //		    int height = 600;
        //		    if (game)
        //			    game->GetDefaultSize(width, height);
        //		    ShowWindow(hWnd, SW_SHOWNORMAL);
        //		    SetWindowPos(hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        //	    }
        //	    else
        //	    {
        //		    SetWindowLongPtr(hWnd, GWL_STYLE, 0);
        //		    SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);
        //		    SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        //		    ShowWindow(hWnd, SW_SHOWMAXIMIZED);
        //	    }
        //	    s_fullscreen = !s_fullscreen;
        //	}
#pragma endregion
    
    }
}
#pragma endregion

#pragma region WindowSystem Implementation
ClayEngine::WindowSystem::WindowSystem(AffinityData affinityId, HINSTANCE hInstance, int nCmdShow, Unicode className, Unicode windowName)
    : m_affinity(affinityId), m_instance_handle(hInstance), m_show_flags(nCmdShow), m_class_name(className), m_window_name(windowName)
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
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszClassName = m_class_name.c_str();

    ThrowIfFailed(RegisterClassExW(&wcex));

    // m_window_handle = CreateWindowExW(WS_EX_TOPMOST, ..., WS_POPUP,
    m_window_handle = CreateWindowExW(m_show_flags, m_class_name.c_str(), m_window_name.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, c_default_window_width, c_default_window_height, nullptr, nullptr, m_instance_handle, this);
    if (!m_window_handle) throw std::exception("CreateWindowW() ERROR: Window handle null");

    // m_show_flags = SW_SHOWMAXIMIZED
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

