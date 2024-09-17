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
#include "ClayEngineContext.h"

namespace ClayEngine
{
	/// <summary>
	/// ClayEngine is a boot loader class that will read the
	/// configuration defined in clayengine.json.
	/// </summary>
	class ClayEngine
	{
		HINSTANCE m_hInstance; // This is the HANDLE to the process
		LPWSTR m_cmdLine; // The command line from the OS
		UINT m_cmdShow; // The window display flags, eg. SW_SHOWDEFAULT

		JsonFilePtr m_bootstrap = {}; // Loads clayengine.json

		AffinityData m_affinity_data = {}; // Universal scope (parent of all) affinity data. this_thread = root_thread
		
		DX11DeviceFactoryPtr m_device = nullptr; // Universal scope hardware device factory, includes feature interrogation

		ClientMap m_clients = {};
		ServerMap m_servers = {};
		HeadlessMap m_headless = {};

	public:
		ClayEngine(HINSTANCE hInstance, LPWSTR lpCmdLine, UINT nCmdShow, Locale pLocale);
		~ClayEngine();

		void Run()
		{
			while (!m_clients.empty() || !m_servers.empty() || !m_headless.empty())
			{
				bool unreferencedParameter = false;
				UNREFERENCED_PARAMETER(unreferencedParameter);
			}
		}
	};
	using ClayEnginePtr = std::unique_ptr<ClayEngine>;

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
