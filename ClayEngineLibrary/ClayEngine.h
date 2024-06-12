#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <future>
#include <thread>
#include <mutex>
#include <chrono>
#include <functional>
#include <vector>

#include "Services.h" // Shared Services
#include "Storage.h" // File System

// In an effort to standardize terminology, the following prefixes should be 
// used for most functions, when possible.
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
	//constexpr auto c_settings_json = "clayengine.json";

	constexpr auto c_settings_debug_json = "clayengine_debug.json"; // Debug scenario with console, server and multiple clients
	constexpr auto c_settings_client_json = "clayengine_client.json"; // Graphical client
	constexpr auto c_settings_server_json = "clayengine_server.json"; // Graphical server
	constexpr auto c_settings_headless_json = "clayengine_headless.json"; // Console server

	using PortType = unsigned short;

	using Future = std::future<void>;
	using FutureStatus = std::future_status;
	using Promise = std::promise<void>;
	using Thread = std::thread;
	using Mutex = std::mutex;

	constexpr auto c_pi = 3.1415926535F;
	constexpr auto c_2pi = 6.283185307F;

	constexpr auto c_max_stringbuffer_length = 1024LL;
	constexpr auto c_max_scrollback_length = 20LL;
	constexpr auto c_max_displaybuffer_length = 256LL;
	constexpr auto c_max_stringarray_length = 2048LL;

	constexpr auto c_target_frame_rate = 60UL;
	constexpr auto c_ticks_per_second = 100'000'000ULL;

	using TimePoint = std::chrono::steady_clock::time_point;
	using TimeSpan = std::chrono::steady_clock::duration;
	using Clock = std::chrono::steady_clock;
	//using TimePoint = std::chrono::high_resolution_clock::time_point;
	//using TimeSpan = std::chrono::high_resolution_clock::duration;
	//using Clock = std::chrono::high_resolution_clock;

	using Seconds = std::chrono::seconds;
	using Milliseconds = std::chrono::milliseconds; // 1'000
	using Microseconds = std::chrono::microseconds; // 1'000'000
	using Nanoseconds = std::chrono::nanoseconds;   // 1'000'000'000

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
