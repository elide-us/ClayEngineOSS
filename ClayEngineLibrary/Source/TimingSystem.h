#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Timing System Library (C) 2022 Epoch Meridian, LLC.             */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include "ClayEngine.h"
#include "RenderSystem.h"

namespace ClayEngine
{
	using namespace ClayEngine::Graphics;

	inline double TicksToSeconds(uint64_t ticks) noexcept { return static_cast<double>(ticks) / c_ticks_per_second; }

	inline uint64_t SecondsToTicks(double seconds) noexcept { return static_cast<uint64_t>(seconds * c_ticks_per_second); }

	/// <summary>
	/// This functor serves as the entry point for the game ticker's thread
	/// </summary>
	struct TickMachine
	{
		void operator()(Future future);
	};

	/// <summary>
	/// TimingCore class provides interval timing information for Update calls
	/// </summary>
	class TimingCore
	{
		TimePoint m_last_timepoint = {};
		TimeSpan m_frames_timespan = {};
		TimeSpan m_total_timespan = {};
		TimeSpan m_update_timespan = {};

		uint32_t m_frame_count = 0;
		uint32_t m_fps = 0;

		bool m_fixed_update = false;

		uint32_t m_target_frame_rate = c_target_frame_rate;
		TimeSpan m_target_update_time = Seconds(1 / m_target_frame_rate); //TODO: This is suspect, considering the nanosecond math...

	public:
		TimingCore();
		~TimingCore();

		void UpdateTimer();

		double GetElapsedSeconds() { return TicksToSeconds(m_frames_timespan.count()); }
		double GetTotalSeconds() { return TicksToSeconds(m_total_timespan.count()); }
		uint64_t GetElapsedTicks() { return static_cast<uint64_t>(m_frames_timespan.count()); }
		uint64_t GetTotalTicks() { return static_cast<uint64_t>(m_total_timespan.count()); }

		uint32_t GetFrameCount() { return m_frame_count; }
		uint32_t GetFramesPerSecond() { return m_fps; }

		void SetFixedUpdate(bool fixed) noexcept { m_fixed_update = fixed; }
		void SetTargetFramerate(uint32_t framerate) noexcept { m_target_update_time = Seconds{ 1 / framerate }; }

		void ResetTimer();
	};
	using TimingCorePtr = std::unique_ptr<TimingCore>;
	using TimingCoreRaw = TimingCore*;

	/// <summary>
	/// This functor is meant to call the core game loop Update/Draw
	/// when the engine core switches to any kind of scene rendering mode
	/// </summary>
	class TimingSystem
	{
		Thread m_thread;
		Promise m_promise = {};

		RenderSystemRaw m_rs = nullptr;

		TimingCorePtr m_timer = nullptr;
		bool m_timer_running = false;
		double m_last_total_seconds = 0.;

		UpdateCallbacks m_update_callbacks = {};
		std::mutex m_update_callbacks_mtx = {};

		DrawCallbacks m_draw_callbacks = {};
		std::mutex m_draw_callbacks_mtx = {};

	public:
		TimingSystem();
		~TimingSystem();

		void StartTimer();
		void StopTimer();
		void ResetTimer();
		void RunGameTick();
		uint32_t GetFPS();
		double GetTotalSeconds();

		void AddUpdateCallback(UpdateCallback fn);
		void OnUpdateCallback(float elapsedTime);
		void ClearUpdateCallbacks();

		void AddDrawCallback(DrawCallback fn);
		void OnDrawCallback();
		void ClearDrawCallbacks();
	};
	using TimingSystemPtr = std::unique_ptr<TimingSystem>;
	using TimingSystemRaw = TimingSystem*;

	class TimingSystemExtension
	{
	protected:
		TimingSystemRaw ticker = nullptr;
	public:
		TimingSystemExtension()
		{
			ticker = Services::GetService<TimingSystem>(std::this_thread::get_id());
		}
		~TimingSystemExtension()
		{
			ticker = nullptr;
		}
	};
}
