#include "pch.h"
#include "ClayEngine.h"

#include <DirectXMath.h>
#include <exception>

#include "ClientCore.h"
//#include "ServerCore.h"
//#include "HeadlessCore.h"

namespace
{
	//INFO: This is the hidden flag to enable the debug console
	constexpr auto g_debug = true;
}

ClayEngine::ClayEngine::ClayEngine(HINSTANCE hInstance, LPWSTR lpCmdLine, UINT nCmdShow, Locale pLocale)
	: m_hInstance(hInstance), m_cmdLine(lpCmdLine), m_cmdShow(nCmdShow)
{
	if (FAILED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED))) throw std::exception("ClayEngine CRITICAL: Failed to Initialize COM");
	if (!DirectX::XMVerifyCPUSupport()) throw std::exception("ClayEngine CRITICAL: CPU Unsupported");

	if (g_debug)
	{
		if (AllocConsole())
		{
			FILE* file = nullptr;
			_wfreopen_s(&file, L"CONIN$", L"r", stdin);
			_wfreopen_s(&file, L"CONOUT$", L"w", stdout);
			_wfreopen_s(&file, L"CONOUT$", L"w", stderr);
			WriteLine("ClayEngine INFO: Allocated default console");
		}
		else WriteLine("ClayEngine WARNING: Failed to allocate default console");
	}

	// Intentionally hard-coded, this is your default startup file
	m_bootstrap = std::make_unique<JsonFile>(c_bootstrap_json);
}

ClayEngine::ClayEngine::~ClayEngine()
{
	m_bootstrap.reset();

	FreeConsole();
	CoUninitialize();
}

void ClayEngine::ClayEngine::Run()
{
	//TODO: Much work to do here...

	// Inside the above file, define the startup key to script the startup process for your game
	//m_startup = std::make_unique<JsonFile>(m_bootstrap->GetValue<std::string>("startup"));

	//std::for_each( m_startup.begin(), m_startup.end(), [&]() { /* */ } );
	// "type" == "client", "server", "headless"

	auto _cec = std::make_unique<ClayEngineClient>(m_hInstance, m_cmdShow, m_cmdLine, L"client01", L"ClayEngine Test Client");

	// Start the std::cin parser for flow control
	bool _run = true;
	while (_run)
	{
		Unicode _input;
		std::wcin >> _input;

		if (_input == L"quit")
		{
			_run = false;
			std::cout << "Beginning system shutdown, press ENTER to exit..." << std::endl;
		}
	}

	_cec.reset();
}

#pragma region Orphaned Code Fragments
//bool quit = true;
//while (quit == true)
//{
//	if (g_debug_console->GetInputSet())
//	{
//		auto words = SplitString(g_debug_console->GetInput());
//		for (size_t i = 0; i < words.size(); ++i)
//		{
//			if (words[i] == "start")
//			{
//				if (words.size() > i + 1ULL)
//				{
//					if (words[i + 1ULL] == "window")
//					{
//						if (words.size() > i + 2ULL)
//						{
//							if (words[i + 2ULL] == "client")
//							{
//								m_client_window = std::make_unique<ClientWindow>(hInstance, nShowCmd, RECT{ 0, 0, settings->GetValue<int>("video", "width"), settings->GetValue<int>("video", "height") }, settings->GetValue<std::string>("title"));
//								break;
//							}
//							if (words[i + 2ULL] == "server")
//							{
//								m_server_window = std::make_unique<ServerWindow>(hInstance, nShowCmd, RECT{ 0, 0, settings->GetValue<int>("video", "width"), settings->GetValue<int>("video", "height") }, settings->GetValue<std::string>("title"));
//								break;
//							}
//						}
//						break;
//					}
//				}
//			}
//			if (words[i] == "stop")
//			{
//				if (words.size() > i + 1ULL)
//				{
//					if (words[i + 1ULL] == "window")
//					{
//						if (words.size() > i + 2ULL)
//						{
//							if (words[i + 2ULL] == "client")
//							{
//								m_client_window.reset();
//								m_client_window = nullptr;
//								break;
//							}
//							if (words[i + 2ULL] == "server")
//							{
//								m_server_window.reset();
//								m_server_window = nullptr;
//								break;
//							}
//						}
//						break;
//					}
//				}
//			}
//			if (words[i] == "quit")
//			{
//				quit = false;
//				WriteLine("Beginning system shutdown, press ENTER to exit...");
//				break;
//			}
//		}
//	}
#pragma endregion