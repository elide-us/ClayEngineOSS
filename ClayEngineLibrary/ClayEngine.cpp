#include "pch.h"
#include "ClayEngine.h"
#include "Services.h"

#include <DirectXMath.h>
#include <exception>

ClayEngine::ClayEngine::ClayEngine(HINSTANCE hInstance, LPWSTR lpCmdLine, UINT nCmdShow, Locale pLocale)
	: m_hInstance(hInstance), m_cmdLine(lpCmdLine), m_cmdShow(nCmdShow)
{
	if (!DirectX::XMVerifyCPUSupport()) throw std::exception("ClayEngine CRITICAL: CPU Unsupported");
	if (FAILED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED))) throw std::exception("ClayEngine CRITICAL: Failed to Initialize COM");

	m_affinity_data.root_thread = m_affinity_data.this_thread = std::this_thread::get_id();

	// Intentionally hard-coded, this is your default startup file
	m_bootstrap = std::make_unique<JsonFile>(c_bootstrap_json);

	auto doc = m_bootstrap->GetDocument();
	auto& startup = doc["startup"];
	for (auto& element : startup)
	{
		auto _type = element["type"].get<std::string>();

		if (_type == "client")
		{
			auto _title = element["title"].get<std::string>();
			auto _class = element["class"].get<std::string>();

			m_clients.emplace(_class, std::make_unique<ClayEngineClient>(m_bootstrap->GetDocument(), m_hInstance, m_affinity_data.root_thread, ToUnicode(_class), ToUnicode(_title)));
		}

		if (_type == "server")
		{
			auto _title = element["title"].get<std::string>();
			auto _class = element["class"].get<std::string>();

			m_servers.emplace(_class, std::make_unique<ClayEngineServer>(m_bootstrap->GetDocument(), m_hInstance, m_affinity_data.root_thread, ToUnicode(_class), ToUnicode(_title)));
		}

		if (_type == "headless")
		{
			auto _class = element["class"].get<std::string>();

			m_headless.emplace(_class, std::make_unique<ClayEngineHeadless>(m_bootstrap->GetDocument(), m_affinity_data.root_thread, ToUnicode(_class)));
		}
	}

	m_device = Services::MakeService<DX11DeviceFactory>(m_affinity_data);
}

ClayEngine::ClayEngine::~ClayEngine()
{
	for (auto& element : m_headless)
	{
		element.second.reset();
	}

	for (auto& element : m_servers)
	{
		element.second.reset();
	}

	for (auto& element : m_clients)
	{
		element.second.reset();
	}

	m_bootstrap.reset();
	CoUninitialize();
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

// S - Diamond - Mythical - Orange - 2.5
// A - Platinum - Legendry - Red - 2.25
// B - Gold - Epic - Purple - 2.0
// C - Silver - Superior - Blue - 1.75
// D - Bronze - Uncommon - Green - 1.5
// E - Iron - Common - White - 1.25
// F - Stone - Junk - Grey - 1.0




