#include "pch.h"
#include "ClayEngineContext.h"

#include "WindowSystem.h"
//#include "InputSystem.h"
//#include "TimingSystem.h"
#include "ContentSystem.h"
//#include "RenderSystem.h"
#include "AsyncNetworkSystem.h"

namespace ClayEngine
{
    /// <summary>
    /// This is the main entry point for a client thread. It can be signaled to shut down by setting the future.
    /// </summary>
    struct ClayEngineClientEntryPoint
    {
        int operator()(Document document, HINSTANCE hInstance, UINT nCmdShow, Unicode className, Unicode windowName, FUTURE future, ClayEngineClient* context);
    };

    /// <summary>
	/// This is the main entry point for a GUI server thread. It can be signaled to shut down by setting the future.
    /// </summary>
    struct ClayEngineServerEntryPoint
    {
		int operator()(Document document, HINSTANCE hInstance, UINT nCmdShow, Unicode className, Unicode windowName, FUTURE future, ClayEngineServer* context);
    };

    /// <summary>
	/// This is the main entry point for the headless server thread. It can be signaled to shut down by setting the future.
    /// </summary>
    struct ClayEngineHeadlessEntryPoint
    {
		int operator()(Document document, Unicode className, FUTURE future, ClayEngineHeadless* context);
    };
}

#pragma region ClayEngineClient Setup
ClayEngine::ClayEngineClient::ClayEngineClient(Document document, HINSTANCE hInstance, Affinity pRoot, Unicode className, Unicode windowName)
    : m_instance_handle(hInstance)
{
    m_document = document;
    m_affinity_data.root_thread = pRoot;
    m_thread.Thread = THREAD{ ClayEngineClientEntryPoint(), document, hInstance, m_show_flags, className, windowName, std::move(m_thread.Promise.get_future()), this };
}

ClayEngine::ClayEngineClient::~ClayEngineClient()
{
    m_thread.Promise.set_value();
    if (m_thread.Thread.joinable()) m_thread.Thread.join();
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

int ClayEngine::ClayEngineClientEntryPoint::operator()(Document document, HINSTANCE hInstance, UINT nCmdShow, Unicode className, Unicode windowName, FUTURE future, ClayEngineClient* context)
{
    context->SetContextAffinity(std::this_thread::get_id());
    auto _affinity = context->GetAffinityData();

    auto _window = Services::MakeService<WindowSystem>(_affinity, hInstance, nCmdShow, className, windowName);
    
    //auto _input = Services::MakeService<InputSystem>(_affinity);

    auto _resources = Services::MakeService<DX11Resources>(_affinity);

    auto _content = Services::MakeService<ContentSystem>(_affinity);

    auto _network = Services::MakeService<AsyncNetworkSystem>(_affinity, className, document);

    //auto _timing = Services::MakeService<TimingSystem>(_affinity);
    
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

#pragma region ClayEngineServer Setup
ClayEngine::ClayEngineServer::ClayEngineServer(Document document, HINSTANCE hInstance, Affinity pRoot, Unicode className, Unicode windowName)
	: m_instance_handle(hInstance)
{
    m_document = document;
	m_affinity_data.root_thread = pRoot;
	m_thread.Thread = THREAD{ ClayEngineServerEntryPoint(), document, hInstance, m_show_flags, className, windowName, std::move(m_thread.Promise.get_future()), this };
}

ClayEngine::ClayEngineServer::~ClayEngineServer()
{
	m_thread.Promise.set_value();
	if (m_thread.Thread.joinable()) m_thread.Thread.join();
}

void ClayEngine::ClayEngineServer::SetContextAffinity(Affinity affinity)
{
	m_affinity_data.this_thread = affinity;
}

const ClayEngine::AffinityData& ClayEngine::ClayEngineServer::GetAffinityData() const
{
	return m_affinity_data;
}
#pragma endregion

int ClayEngine::ClayEngineServerEntryPoint::operator()(Document document, HINSTANCE hInstance, UINT nCmdShow, Unicode className, Unicode windowName, FUTURE future, ClayEngineServer* context)
{
    context->SetContextAffinity(std::this_thread::get_id());
    auto _affinity = context->GetAffinityData();

    auto _window = Services::MakeService<WindowSystem>(_affinity, hInstance, nCmdShow, className, windowName);

	auto _network = Services::MakeService<AsyncNetworkSystem>(_affinity, className, document);

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

#pragma region ClayEngineHeadless Setup
ClayEngine::ClayEngineHeadless::ClayEngineHeadless(Document document, Affinity pRoot, Unicode className)
{
	m_document = document;
	m_affinity_data.root_thread = pRoot;
	m_thread.Thread = THREAD{ ClayEngineHeadlessEntryPoint(), document, className, std::move(m_thread.Promise.get_future()), this };
}

ClayEngine::ClayEngineHeadless::~ClayEngineHeadless()
{
	m_thread.Promise.set_value();
	if (m_thread.Thread.joinable()) m_thread.Thread.join();
}

void ClayEngine::ClayEngineHeadless::SetContextAffinity(Affinity affinity)
{
	m_affinity_data.this_thread = affinity;
}

const ClayEngine::AffinityData& ClayEngine::ClayEngineHeadless::GetAffinityData() const
{
	return m_affinity_data;
}
#pragma endregion

int ClayEngine::ClayEngineHeadlessEntryPoint::operator()(Document document, Unicode className, FUTURE future, ClayEngineHeadless* context)
{
	context->SetContextAffinity(std::this_thread::get_id());
	auto _affinity = context->GetAffinityData();

    if (AllocConsole())
    {
        FILE* file = nullptr;
        _wfreopen_s(&file, L"CONIN$", L"r", stdin);
        _wfreopen_s(&file, L"CONOUT$", L"w", stdout);
        _wfreopen_s(&file, L"CONOUT$", L"w", stderr);
    }

    auto _network = Services::MakeService<AsyncNetworkSystem>(_affinity, className, document);

    while (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout)
    {
        Unicode input;
        std::wcin >> input;

        if (input == L"quit")
		{
			std::wcout << L"Beginning system shutdown, press ENTER to exit..." << std::endl;
            break;
        }
    }

    return 0;
}
