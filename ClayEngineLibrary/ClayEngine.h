#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
//#include <future>
//#include <thread>
//#include <mutex>
//#include <chrono>
#include <functional>
#include <vector>

#include "Services.h" // Shared Services
#include "Storage.h" // File System
#include "Definitions.h" // Engine Constants

// In an effort to standardize terminology, the following prefixes should be 
// used for most functions, when possible:
// Create, Read, Update, Delete
// Make/Destroy
// Add/Remove
// Set/Get

namespace ClayEngine
{
	//constexpr auto c_default_window_title = "ClayEngine";
	//constexpr auto c_default_window_class = "ClayEngineWindowClass";
	//constexpr auto c_default_server_port = 48000;
	//constexpr auto c_default_server_addr = "127.0.0.1";

	constexpr auto c_settings_json = "clayengine.json";

	//constexpr auto c_settings_debug_json = "clayengine_debug.json"; // Debug scenario with console, server and multiple clients
	//constexpr auto c_settings_client_json = "clayengine_client.json"; // Graphical client
	//constexpr auto c_settings_server_json = "clayengine_server.json"; // Graphical server
	//constexpr auto c_settings_headless_json = "clayengine_headless.json"; // Console server

	constexpr auto c_max_stringbuffer_length = 1024LL;
	constexpr auto c_max_scrollback_length = 20LL;
	constexpr auto c_max_displaybuffer_length = 256LL;
	constexpr auto c_max_stringarray_length = 2048LL;

	using UpdateCallback = std::function<void(float)>;
	using UpdateCallbacks = std::vector<UpdateCallback>;

	using DrawCallback = std::function<void()>;
	using DrawCallbacks = std::vector<DrawCallback>;

	struct ILayoutChanged
	{
		virtual void OnLayoutChanged(float x, float y, float width, float height) = 0;
	};

	struct IUpdate
	{
		virtual void Update(float elapsedTime) = 0;
	};

	struct IDraw
	{
		virtual void Draw() = 0;
	};

	/// <summary>
	/// This extension provides the members for a threaded class, but you 
	/// still need to provide the functor and create the thread object
	/// </summary>
	class ThreadExtension
	{
	protected:
		Promise m_promise = {};
		Thread m_thread;
	public:
		~ThreadExtension()
		{
			if (m_thread.joinable())
			{
				m_promise.set_value();
				m_thread.join();
			}
		}
	};

}
