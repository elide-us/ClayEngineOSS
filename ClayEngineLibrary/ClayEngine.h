#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* Main engine library header with bootstrap classes                          */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include <Windows.h>
#include <memory>

#include "Strings.h" // String and error handling
#include "Storage.h" // Filesystem and JSON parsing

#include "DX11DeviceFactory.h"
#include "ClayEngineClient.h"
#include "ClayEngineHeadless.h"


namespace ClayEngine
{
	/// <summary>
	/// ClayEngine is a boot loader class that will read the
	/// configuration defined in clayengine.json.
	/// </summary>
	class ClayEngine
	{
		bool run = true;

		HINSTANCE m_hInstance; // This is the HANDLE to the process
		LPWSTR m_cmdLine; // The command line from the OS
		UINT m_cmdShow; // The window display flags, eg. SW_SHOWDEFAULT

		JsonFilePtr m_bootstrap = {}; // Loads clayengine.json

		AffinityData m_affinity_data = {}; // Universal scope (parent of all) affinity data. this_thread = root_thread
		
		DX11DeviceFactoryPtr m_device = nullptr; // Universal scope hardware device factory, includes feature interrogation

		ClientMap m_clients = {};

	public:
		ClayEngine(HINSTANCE hInstance, LPWSTR lpCmdLine, UINT nCmdShow, Locale pLocale);
		~ClayEngine();

		void Run();
		void SetExit() { run = false; }
	};
	using ClayEnginePtr = std::unique_ptr<ClayEngine>;
	using ClayEngineRaw = ClayEngine*;

	// Might be used for things like authentication, gateway/egress network routing servers, dedicated chat/messenger, etc.
	class ClayHeadless
	{
		JsonFilePtr m_bootstrap = {}; // Loads clayengine.json

		AffinityData m_affinity_data = {}; // Universal scope (parent of all) affinity data. this_thread = root_thread

		HeadlessMap m_headless = {};

	public:
		ClayHeadless()
		{
			m_affinity_data.root_thread = m_affinity_data.this_thread = std::this_thread::get_id();

			m_bootstrap = std::make_unique<JsonFile>(c_bootstrap_headless_json);
		}
		~ClayHeadless()
		{
			FreeConsole();
		}

		void Run()
		{
			auto doc = m_bootstrap->GetDocument();
			auto& startup = doc["startup"];
			for (auto& element : startup)
			{
				auto _type = element["type"].get<std::string>();

				if (_type == "headless")
				{
					auto _class = element["class"].get<std::string>();
					auto _port = element["port"].get<std::string>();
					auto _address = element["address"].get<std::string>();

					m_headless.emplace(_class, std::make_unique<ClayEngineHeadless>(m_affinity_data.root_thread));
				}
			}

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

			for (auto& element : m_headless)
			{
				element.second.reset();
			}
		};
	};
	using ClayHeadlessPtr = std::unique_ptr<ClayHeadless>;

#pragma region Orphaned Code Fragments
	//#define _USE_MATH_DEFINES
	//#include <cmath>
	//#include <future>
	//#include <thread>
	//#include <mutex>
	//#include <chrono>
	//#include <functional>
	//#include <vector>

	// In an effort to standardize terminology, the following prefixes should be 
	// used for most functions, when possible:
	// Create, Read, Update, Delete (CRUD)
	// Make/Erase
	// Add/Remove
	// Set/Get
	
	//constexpr auto c_pi = 3.1415926535F;
	//constexpr auto c_2pi = 6.283185307F;

	//constexpr auto c_default_server_port = 19740;
	//constexpr auto c_default_server_addr = "127.0.0.1";

	
	//using UpdateCallback = std::function<void(float)>;
	//using UpdateCallbacks = std::vector<UpdateCallback>;

	//using DrawCallback = std::function<void()>;
	//using DrawCallbacks = std::vector<DrawCallback>;

	//struct IUpdate
	//{
	//	virtual void Update(float elapsedTime) = 0;
	//};

	//struct IDraw
	//{
	//	virtual void Draw() = 0;
	//};
#pragma endregion
}
