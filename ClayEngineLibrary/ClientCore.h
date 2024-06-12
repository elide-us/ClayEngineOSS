#pragma once

#include "ClayEngine.h"
#include "WindowSystem.h"
#include "TimingSystem.h"
#include "InputSystem.h"

using namespace ClayEngine::Platform;

namespace ClayEngine
{
    // This is the true entry point for a client instance, everything under this is just plumbing
    // This entry point runs in its own thread, but uses the static Services object, which keeps track of
    // services used by that specific thread ID.
    struct ClayEngineClientEntryPoint
    {
		// Add className and windowName to the parameters when your brain works again...
        int operator()(HINSTANCE hInstance, int nCmdShow, Unicode className, Unicode windowName, Future future)
        {
            // First we make a window
            auto wnd = Services::MakeService<WindowSystem>(std::this_thread::get_id(), hInstance, nCmdShow, className, windowName);

            // Next fill out the services starting with DirectX graphics systems, input, and content

			auto timing = Services::MakeService<TimingSystem>(std::this_thread::get_id());
			auto input = Services::MakeService<InputSystem>(std::this_thread::get_id());

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
    };

    // Should instantiate a thread for this particular game client
    class ClayEngineClientCore
        : public ThreadExtension
    {
        HINSTANCE m_instance_handle;
		UINT m_show_flags;

    public:
		ClayEngineClientCore(HINSTANCE hInstance, int nCmdShow = SW_SHOWDEFAULT, Unicode className = L"default", Unicode windowName = L"ClayEngine") : m_instance_handle(hInstance), m_show_flags(nCmdShow)
        {
            m_thread = std::thread{ ClayEngineClientEntryPoint(), hInstance, nCmdShow, className, windowName, std::move(m_promise.get_future()) };
        }

        ~ClayEngineClientCore()
        {
            m_promise.set_value();
            m_thread.join();
        }

        int Run()
        {
            while (true)
            {
                // Do application control logic here
            }
        }
    };
    using ClayEngineClientCorePtr = std::unique_ptr<ClayEngineClientCore>;

}