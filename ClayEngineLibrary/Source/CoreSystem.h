#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Client Core Class Library (C) 2022 Epoch Meridian, LLC.         */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include "ClayEngine.h"
#include "WindowSystem.h"
#include "InputSystem.h"
#include "TimingSystem.h"
#include "RenderSystem.h"
#include "ContentSystem.h"
#include "NetworkSystem.h"

#include "Sensorium.h"

//#include "SquareChase.h"
//#include "VoxelFarmThread.h"
//#include "VoxelFarmCellRender.h"
//#include "PrimitivePlayground.h"

#include "Mouse.h"

using namespace ClayEngine;
using namespace ClayEngine::Game;
using namespace ClayEngine::Platform;
using namespace ClayEngine::Graphics;
using namespace ClayEngine::Networking;

namespace
{
	DebugConsolePtr g_debug_console = nullptr;
	ClientWindowPtr g_client_window = nullptr;
	ServerWindowPtr g_server_window = nullptr;
	ServerConsolePtr g_server_console = nullptr;
}

namespace ClayEngine
{
	class CoreSystem
	{
		// The instance handle for the parent window (wWinMain)
		HINSTANCE m_instance;
		int m_flags;

		// DOM class for the specified configuration file in clayengine.json
		JsonFilePtr m_settings = {};

		// Not sure if I need this here or somewhere else yet...
		InputSystemPtr m_input = nullptr;

		void SetLocale()
		{
			auto locale = std::make_unique<JsonFile>("clayengine.json");
			std::locale::global(std::locale(locale->GetValue<std::string>("locale")));
		}
		JsonFilePtr LoadSettings()
		{
			auto startup = std::make_unique<JsonFile>("clayengine.json");
			return std::make_unique<JsonFile>(startup->GetValue<std::string>("startup"));
		}
		void Initialize()
		{
			//SetLocale();

			if (FAILED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED))) throw std::exception("Core CRITICAL: Failed to Initialize COM");
			if (!DirectX::XMVerifyCPUSupport()) throw std::exception("Core CRITICAL: CPU Unsupported");

			InputHandler::Initialize();

			m_settings = LoadSettings();
		}

	public:
		CoreSystem(HINSTANCE hInstance, int flags) : m_instance(hInstance), m_flags(flags)
		{
			Initialize();

			//m_input = Services::MakeService<InputSystem>();

			auto dom = m_settings->GetDocument();
			for (auto& element : dom["appmode"])
			{
				if (element == "debug")
				{
					//g_debug_console = ClayEngine::Services::MakeService<DebugConsole>();
					//g_debug_console->StartConsoleInputHandler();
				}
				if (element == "client")
				{
					auto cw = dom["client"]["width"];
					auto ch = dom["client"]["height"];
					auto cz = RECT{ 0, 0, cw, ch };

					auto ct = dom["client"]["title"].get<std::string>();
					auto cc = dom["client"]["class"].get<std::string>();

					//g_client_window = ClayEngine::Services::MakeService<ClientWindow>(hInstance, flags, cz, ct, cc);
				}
				if (element == "guiserver")
				{
					auto sw = dom["guiserver"]["width"];
					auto sh = dom["guiserver"]["height"];
					auto sz = RECT{ 0, 0, sw, sh };

					auto st = dom["guiserver"]["title"].get<std::string>();
					auto sc = dom["guiserver"]["class"].get<std::string>();

					//g_server_window = ClayEngine::Services::MakeService<ServerWindow>(hInstance, flags, sz, st, sc);
				}
				if (element == "conserver")
				{
					if (!g_debug_console)
					{
						g_debug_console = ClayEngine::Services::MakeService<DebugConsole>();
						//g_debug_console->StartConsoleInputHandler();
					}
					g_server_console = ClayEngine::Services::MakeService<ServerConsole>();
					g_server_console->Run();
				}
			}
		}
		~CoreSystem()
		{
			if (m_input)
			{
				Services::RemoveService<InputSystem>();
				m_input.reset();
				m_input = nullptr;
			}

			if (g_client_window)
			{
				Services::RemoveService<ClientWindow>();
				g_client_window.reset();
				g_client_window = nullptr;
			}

			m_settings.reset();

			CoUninitialize();
		}

		void Run()
		{
			auto quit = true;
			while (quit == true)
			{
				if (g_debug_console->GetInputSet())
				{
					auto words = SplitString(g_debug_console->GetInput());

					for (size_t i = 0; i < words.size(); ++i)
					{
						if (words[i] == "quit")
						{
							quit = false;
							WriteLine("Beginning system shutdown, press ENTER to exit...");
							break;
						}
						if (words[i] == "start")
						{
							if (words.size() > i + 1)
							{
								if (words[i + 1] == "client")
								{
									auto dom = m_settings->GetDocument();

									auto cw = dom["client"]["width"];
									auto ch = dom["client"]["height"];
									auto cz = RECT{ 0, 0, cw, ch };

									auto ct = dom["client"]["title"].get<std::string>();
									auto cc = dom["client"]["class"].get<std::string>();

									//g_client_window = ClayEngine::Services::MakeService<ClientWindow>(m_instance, m_flags, cz, ct, cc);
								}
								if (words[i + 1] == "conserver")
								{
									if (!g_debug_console)
									{
										//g_debug_console = ClayEngine::Services::MakeService<DebugConsole>();
										//g_debug_console->StartConsoleInputHandler();
									}
									//g_server_console = ClayEngine::Services::MakeService<ServerConsole>();
									//g_server_console->Run();
								}
							}
						}
						if (words[i] == "stop")
						{
							if (words.size() > i + 1)
							{
								if (words[i + 1] == "client")
								{
									if (g_client_window)
									{
										//Services::RemoveService<ClientWindow>();
										//g_client_window.reset();
										//g_client_window = nullptr;
									}
								}
								if (words[i + 1] == "guiserver")
								{
									if (g_server_window)
									{
										//Services::RemoveService<ServerWindow>();
										//g_server_window.reset();
										//g_server_window = nullptr;
									}
								}
							}
						}
					}
				}
			}
		}
	};
	using CoreSystemPtr = std::unique_ptr<CoreSystem>;
	using CoreSystemRaw = CoreSystem*;

	enum class ClientCoreState// : unsigned int
	{
		Default,
		Initializing,
		DebugRunning,
	};

	class ClientCoreSystem
	{
		ClientCoreState m_state = ClientCoreState::Default;
		bool m_state_changed = true; // This flag effectively acts as a state gate to stop the MSG loop pump.

		//ClientWindowPtr m_client = nullptr;
		TimingSystemPtr m_timer = nullptr;
		RenderSystemPtr m_render = nullptr;
		ContentSystemPtr m_content = nullptr;
		NetworkSystemPtr m_network = nullptr;

		SensoriumPtr m_sensorium = nullptr;

		//SquareChasePtr m_chase = nullptr;

		//VoxelFarmCellRenderPtr m_vfr = nullptr;
		//VoxelFarmThreadPtr m_vf = nullptr;

		//RenderPrimitivePtr m_primitive = nullptr;
		//RenderGridPtr m_grid = nullptr;
		//RenderShapePtr m_shape = nullptr;
		//RenderModelPtr m_model = nullptr;

	public:
		ClientCoreSystem();
		~ClientCoreSystem();

		void StopServices();

		void Update(float elapsedTime) {};

		void SetState(ClientCoreState state);

		bool GetStateChanged();

		void OnStateChanged();
	};
	using ClientCoreSystemPtr = std::unique_ptr<ClientCoreSystem>;
	using ClientCoreSystemRaw = ClientCoreSystem*;

	enum class ServerCoreState
	{
		Default,
		Initializing,
		DebugRunning,
		Shutdown,
	};

	class ServerCoreSystem
	{
		ServerCoreState m_state = ServerCoreState::Default;
		bool m_state_changed = true;
		bool m_shutdown = false;

		NetworkSystemPtr m_network = nullptr;

	public:
		ServerCoreSystem()
		{

		}
		~ServerCoreSystem()
		{
			StopServices();
		}

		void StopServices()
		{
			if (m_network)
			{
				Services::RemoveService<NetworkSystem>();

				m_network.reset();
				m_network = nullptr;
			}
		}

		void SetState(ServerCoreState state)
		{
			m_state_changed = true;
			m_state = state;
		}

		bool GetShutdown()
		{
			return m_shutdown;
		}

		bool GetStateChanged()
		{
			return m_state_changed;
		}

		void OnStateChanged()
		{
			m_state_changed = false;
			switch (m_state)
			{
			case ServerCoreState::Default:
			{
				WriteLine("OnStateChanged INFO: ServerCoreState::Default");

				// Do nothing, just move to the next state...

				m_state = ServerCoreState::Initializing;
				m_state_changed = true;
			}
			break;
			case ServerCoreState::Initializing:
			{
				WriteLine("OnStateChanged INFO: ServerCoreState::Initializing");

				// Set up the listen server, this runs in a separate thread and creates socket objects
				m_network = Services::MakeService<NetworkSystem>();
				m_network->SetListenServerHints(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				m_network->SetListenServerPort(48000);
				m_network->SetListenServerTimeout(125);
				m_network->StartListenServer();

				// Switch to the next state
				m_state = ServerCoreState::DebugRunning;
				m_state_changed = true;
			}
			break;
			case ServerCoreState::DebugRunning:
			{
				WriteLine("OnStateChanged INFO: ServerCoreState::DebugRunning");

				// This is a breakable blocking loop, any (non-blank) input will escape the block
				m_network->Run();

				// Switch to the next state
				m_state = ServerCoreState::Shutdown;
				m_state_changed = true;
			}
			break;
			case ServerCoreState::Shutdown:
			{
				WriteLine("OnStateChanged INFO: ServerCoreState::Shutdown");

				// Gracefully shut down the network objects before breaking out of the main server loop
				StopServices();

				// This causes the main server loop to break and continue with the application shutdown
				m_shutdown = true;
			}
			break;
			default:
				break;
			}

		}
	};
	using ServerCoreSystemPtr = std::unique_ptr<ServerCoreSystem>;
	using ServerCoreSystemRaw = ServerCoreSystem*;
}
