#include "pch.h"
#include "ClientCore.h"

#include "WindowSystem.h"
#include "InputSystem.h"
//#include "TimingSystem.h"
#include "ContentSystem.h"
#include "RenderSystem.h"
//#include "NetworkSystem.h"

#pragma region ClayEngineClientEntryPoint Declaration
namespace ClayEngine
{
    /// <summary>
    /// This is the main entry point for a client thread. It can be signaled to shut down by setting the future.
    /// </summary>
    struct ClayEngineClientEntryPoint
    {
        int operator()(HINSTANCE hInstance, UINT nCmdShow, Unicode className, Unicode windowName, Future future, ClayEngineClientRaw context);
    };
}
#pragma endregion

#pragma region ClayEngineClient Definitions
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

void ClayEngine::ClayEngineClient::SetAffinity(Affinity affinity)
{
	m_affinity = affinity;
}

const ClayEngine::Affinity& ClayEngine::ClayEngineClient::GetAffinity() const
{
    return m_affinity;
}
#pragma endregion

#pragma region ClayEngineClientEntryPoint Definition
int ClayEngine::ClayEngineClientEntryPoint::operator()(HINSTANCE hInstance, UINT nCmdShow, Unicode className, Unicode windowName, Future future, ClayEngineClientRaw context)
{
    auto _affinity = std::this_thread::get_id();
    context->SetAffinity(_affinity);

    // The following services need to be instantiated for a basic client:
    // WindowSystem, InputSystem, TimingSystem, ContentSystem, RenderSystem

    //TODO: See older code for client state management logic which should go here

    auto _window = Services::MakeService<WindowSystem>(_affinity, hInstance, nCmdShow, className, windowName);
    
    //TODO: The InputSystem may not be functioning fully as the InputHandler is a static class that probably needs to be reworked
    auto _input = Services::MakeService<InputSystem>(_affinity);

    auto _test_dx11resources = std::make_unique<DX11Resources>(_affinity, 0);

    //TODO: The TimingSystem hasn't been tested or refactored yet
    //auto _timing = Services::MakeService<TimingSystem>(_affinity);
    
    //TODO: The RenderSystem and ContentSystem both compile by there are issues in the RenderSystem with device creation
    //auto _render = Services::MakeService<RenderSystem>(_affinity);
    //auto _content = Services::MakeService<ContentSystem>(_affinity);

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
#pragma endregion
