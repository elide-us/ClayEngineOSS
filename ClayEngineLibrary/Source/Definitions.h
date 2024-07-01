#pragma once

#include <mutex>
#include <thread>
#include <future>
#include <chrono>

/// <summary>
/// ClayEngine Type Aliases and Constants
/// </summary>
namespace ClayEngine
{
	constexpr auto c_pi = 3.1415926535F;
	constexpr auto c_2pi = 6.283185307F;

	using Future = ::std::future<void>;
	using FutureStatus = ::std::future_status;
	using Promise = ::std::promise<void>;
	using Thread = ::std::thread;
	using Mutex = ::std::mutex;

	using TimePoint = ::std::chrono::steady_clock::time_point;
	using TimeSpan = ::std::chrono::steady_clock::duration;
	using Clock = ::std::chrono::steady_clock;
	//using TimePoint = std::chrono::high_resolution_clock::time_point;
	//using TimeSpan = std::chrono::high_resolution_clock::duration;
	//using Clock = std::chrono::high_resolution_clock;

	using Seconds = ::std::chrono::seconds;
	using Milliseconds = ::std::chrono::milliseconds; // 1'000
	using Microseconds = ::std::chrono::microseconds; // 1'000'000
	using Nanoseconds = ::std::chrono::nanoseconds;   // 1'000'000'000

	constexpr auto c_target_frame_rate = 60UL;
	constexpr auto c_ticks_per_second = 100'000'000ULL;

	namespace Networking
	{
		using PortType = unsigned short;
	}

	namespace Platform
	{
		constexpr auto c_min_window_width = 320U;
		constexpr auto c_min_window_height = 240U;

		constexpr auto c_default_window_width = 1920U;
		constexpr auto c_default_window_height = 1080U;

	}
}