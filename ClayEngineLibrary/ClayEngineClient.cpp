#include "pch.h"
#include "ClayEngineClient.h"

#include "WindowSystem.h"
#include "InputSystem.h"
//#include "TimingSystem.h"
#include "ContentSystem.h"
//#include "RenderSystem.h"
#include "AsyncNetworkSystem.h"

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
ClayEngine::ClayEngineClient::ClayEngineClient(HINSTANCE hInstance, Affinity pRoot, Unicode className, Unicode windowName)
    : m_instance_handle(hInstance)
{
    m_affinity_data.root_thread = pRoot;
    m_thread = std::thread{ ClayEngineClientEntryPoint(), hInstance, m_show_flags, className, windowName, std::move(m_promise.get_future()), this };
}

ClayEngine::ClayEngineClient::~ClayEngineClient()
{
    m_promise.set_value();
    m_thread.join();
}

void ClayEngine::ClayEngineClient::SetContextAffinity(Affinity affinity)
{
	m_affinity_data.this_thread = affinity;
}

const ClayEngine::AffinityData& ClayEngine::ClayEngineClient::GetAffinityData() const
{
	return m_affinity_data;
}
#pragma endregion

#pragma region ClayEngineClientEntryPoint Definition
int ClayEngine::ClayEngineClientEntryPoint::operator()(HINSTANCE hInstance, UINT nCmdShow, Unicode className, Unicode windowName, Future future, ClayEngineClientRaw context)
{
    context->SetContextAffinity(std::this_thread::get_id());
    auto _affinity = context->GetAffinityData();

    //TODO: See older code for client state management logic which should go here

    auto _window = Services::MakeService<WindowSystem>(_affinity, hInstance, nCmdShow, className, windowName);
    
    //TODO: The InputSystem may not be functioning fully as the InputHandler is a static class that probably needs to be reworked
    auto _input = Services::MakeService<InputSystem>(_affinity);


    auto _resources = Services::MakeService<DX11Resources>(_affinity);
    auto _content = Services::MakeService<ContentSystem>(_affinity);

    auto _network = std::make_unique<Experimental::AsyncNetworkSystem>(L"127.0.0.1", L"19740");

    //TODO: The TimingSystem hasn't been tested or refactored yet
    //auto _timing = Services::MakeService<TimingSystem>(_affinity);
    
    //TODO: The RenderSystem and ContentSystem both compile by there are issues in the RenderSystem with device creation
    //auto _render = Services::MakeService<RenderSystem>(_affinity);
    
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
