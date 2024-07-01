#include "pch.h"
#include "ClientCore.h"

namespace ClayEngine
{
    /// <summary>
    /// This is the main entry point for a client thread. It can be signaled to shuw down by setting the future.
    /// </summary>
    struct ClayEngineClientEntryPoint
    {
        int operator()(HINSTANCE hInstance, UINT nCmdShow, Unicode className, Unicode windowName, Future future, ClayEngineClientRaw context);
    };
}

ClayEngine::ClayEngineClient::ClayEngineClient(HINSTANCE hInstance, UINT nCmdShow, LPWSTR nCmdLine, Unicode className, Unicode windowName)
    : m_instance_handle(hInstance), m_show_flags(nCmdShow), m_cmd_line(nCmdLine)
{
    m_thread = std::thread{ ClayEngineClientEntryPoint(), hInstance, nCmdShow, className, windowName, std::move(m_promise.get_future()), this };
}

ClayEngine::ClayEngineClient::~ClayEngineClient()
{
    m_promise.set_value();
    m_thread.join();
}

int ClayEngine::ClayEngineClientEntryPoint::operator()(HINSTANCE hInstance, UINT nCmdShow, Unicode className, Unicode windowName, Future future, ClayEngineClientRaw context)
{
    auto _affinity = std::this_thread::get_id();
    context->SetAffinity(_affinity);

    // The following services need to be instantiated for a client:
    // WindowSystem, InputSystem, TimingSystem, ContentSystem, RenderSystem and NetworkSystem

    auto _window = Services::MakeService<WindowSystem>(_affinity, hInstance, nCmdShow, className, windowName);
    //auto _input = Services::MakeService<InputSystem>(_affinity);
    //auto _timing = Services::MakeService<TimingSystem>(_affinity);
    //auto _content = Services::MakeService<ContentSystem>(_affinity);
    //auto _render = Services::MakeService<RenderSystem>(_affinity);
    //auto _network = Services::MakeService<NetworkSystem>(_affinity);

    while (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout)
    {
        MSG msg = {};
        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        return (int)msg.wParam;
    }

    return 0;
}
