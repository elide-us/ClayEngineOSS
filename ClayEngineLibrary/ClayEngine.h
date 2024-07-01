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

namespace ClayEngine
{
	constexpr auto c_bootstrap_json = "clayengine.json";

	/// <summary>
	/// ClayEngine is a boot loader class that will read the
	/// configuration defined in clayengine.json.
	/// </summary>
	class ClayEngine
	{
		HINSTANCE m_hInstance;
		LPWSTR m_cmdLine;
		UINT m_cmdShow;

		JsonFilePtr m_bootstrap = {};

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
	// Create, Read, Update, Delete
	// Make/Destroy
	// Add/Remove
	// Set/Get

	//constexpr auto c_pi = 3.1415926535F;
	//constexpr auto c_2pi = 6.283185307F;

	//constexpr auto c_default_window_title = "ClayEngine";
	//constexpr auto c_default_window_class = "ClayEngineWindowClass";
	//constexpr auto c_default_server_port = 48000;
	//constexpr auto c_default_server_addr = "127.0.0.1";

	//constexpr auto c_settings_debug_json = "clayengine_debug.json"; // Debug scenario with console, server and multiple clients
	//constexpr auto c_settings_client_json = "clayengine_client.json"; // Graphical client
	//constexpr auto c_settings_server_json = "clayengine_server.json"; // Graphical server
	//constexpr auto c_settings_headless_json = "clayengine_headless.json"; // Console server

	//constexpr auto c_max_stringbuffer_length = 1024LL;
	//constexpr auto c_max_scrollback_length = 20LL;
	//constexpr auto c_max_displaybuffer_length = 256LL;
	//constexpr auto c_max_stringarray_length = 2048LL;

	//using UpdateCallback = std::function<void(float)>;
	//using UpdateCallbacks = std::vector<UpdateCallback>;

	//using DrawCallback = std::function<void()>;
	//using DrawCallbacks = std::vector<DrawCallback>;

	//struct ILayoutChanged
	//{
	//	virtual void OnLayoutChanged(float x, float y, float width, float height) = 0;
	//};

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
