#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* Main engine library header with bootstrap class                            */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include <Windows.h>
#include <memory>

#include "Strings.h" // String and error handling
#include "Storage.h" // Filesystem and JSON parsing

#include "DX11DeviceFactory.h"
#include "ClayEngineClient.h"
//#include "ServerCore.h"
//#include "HeadlessCore.h"

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

		AffinityData m_affinity_data = {};
		DX11DevicePtr m_device = nullptr;

		// These maps contain the kernels for each client and server running within this process
		// There should typically only be one Server running to minimize resource usage since 
		// this process runs a GUI. The headless server should be used for most other processes
		// such as authentication, egress routing, database connectors, and the like. It should be
		// possible to run all of these services on one headless node, and connect to it with the
		// GUI server to view and monitor the game world status. It is strongly recommended to
		// run the game world server on its own dedicated node.

		ClientMap m_clients = {};
		//ServerMap m_servers = {};
		//HeadlessMap m_headless = {};

	public:
		ClayEngine(HINSTANCE hInstance, LPWSTR lpCmdLine, UINT nCmdShow, Locale pLocale);
		~ClayEngine();

		void Run();
	};
	using ClayEnginePtr = std::unique_ptr<ClayEngine>;
	using ClayEngineRaw = ClayEngine*;

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
	// Make/Destroy
	// Add/Remove
	// Set/Get
	
	//constexpr auto c_pi = 3.1415926535F;
	//constexpr auto c_2pi = 6.283185307F;

	//constexpr auto c_default_server_port = 48000;
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
