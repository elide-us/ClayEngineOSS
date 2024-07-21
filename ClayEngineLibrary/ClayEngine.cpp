#include "pch.h"
#include "ClayEngine.h"

#include <DirectXMath.h>
#include <exception>

ClayEngine::ClayEngine::ClayEngine(HINSTANCE hInstance, LPWSTR lpCmdLine, UINT nCmdShow, Locale pLocale)
	: m_hInstance(hInstance), m_cmdLine(lpCmdLine), m_cmdShow(nCmdShow)
{
	if (!DirectX::XMVerifyCPUSupport()) throw std::exception("ClayEngine CRITICAL: CPU Unsupported");
	if (FAILED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED))) throw std::exception("ClayEngine CRITICAL: Failed to Initialize COM");

#ifdef _DEBUG
	if (AllocConsole())
	{
		FILE* file = nullptr;
		_wfreopen_s(&file, L"CONIN$", L"r", stdin);
		_wfreopen_s(&file, L"CONOUT$", L"w", stdout);
		_wfreopen_s(&file, L"CONOUT$", L"w", stderr);
		WriteLine("ClayEngine INFO: Allocated default console");
	}
	else WriteLine("ClayEngine WARNING: Failed to allocate default console");
#endif

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
	auto doc = m_bootstrap->GetDocument();
	auto& startup = doc["startup"];
	for (auto& element : startup)
	{
		auto _type = element["type"].get<std::string>();

		if (_type == "client")
		{
			auto _title = element["title"].get<std::string>();
			auto _class = element["class"].get<std::string>();
			
			m_clients.emplace(_class, std::make_unique<ClayEngineClient>(m_hInstance, m_cmdShow, m_cmdLine, ToUnicode(_class), ToUnicode(_title)));
		}

		if (_type == "server")
		{
			WriteLine("Implement ClayEngine GUI Server here");
		}

		if (_type == "headless")
		{
			WriteLine("Implement ClayEngine Service Server here");
		}
	}

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

	for (auto& element : m_clients)
	{
		element.second.reset();
	}
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