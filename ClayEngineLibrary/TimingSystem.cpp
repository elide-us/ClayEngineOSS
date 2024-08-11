#include "pch.h"
#include "TimingSystem.h"

void ClayEngine::TickMachine::operator()(Future future)
{
    auto timing = Services::GetService<TimingSystem>(std::this_thread::get_id());

    while (future.wait_for(Nanoseconds(0)) == std::future_status::timeout)
    {
        timing->RunGameTick();
    }
}

ClayEngine::TimingCore::TimingCore()
{
    m_total_timespan = TimeSpan::zero();

    ResetTimer();
}

ClayEngine::TimingCore::~TimingCore()
{

}

void ClayEngine::TimingCore::ResetTimer()
{
    m_last_timepoint = Clock::now();

    m_fps = 0;
    m_frame_count = 0;

    m_frames_timespan = TimeSpan::zero();
    m_update_timespan = TimeSpan::zero();
}

void ClayEngine::TimingCore::UpdateTimer()
{
    auto now_timepoint = Clock::now();
    auto delta_timespan = now_timepoint - m_last_timepoint;
    m_last_timepoint = now_timepoint;

    m_total_timespan += delta_timespan;
    m_update_timespan += delta_timespan;
    m_frames_timespan += delta_timespan;
    ++m_frame_count;

    if (m_frames_timespan >= Seconds(1))
    {
        m_fps = m_frame_count;
        m_frame_count = 0;
        m_frames_timespan %= Seconds(1);
    }

    if (m_fixed_update)
    {
        if (m_update_timespan >= m_target_update_time)
        {
            m_update_timespan %= m_target_update_time;
            return;
        }
    }
    else
    {
        m_update_timespan = TimeSpan::zero();
        return;
    }
}

ClayEngine::TimingSystem::TimingSystem(AffinityData affinityData)
    : m_affinity_data(affinityData)
{

}

ClayEngine::TimingSystem::~TimingSystem()
{
    StopTimer();
}

void ClayEngine::TimingSystem::StartTimer()
{
    m_rs = Services::GetService<RenderSystem>(std::this_thread::get_id());

    m_timer = std::make_unique<TimingCore>();

    if (!m_timer_running)
    {
        m_thread = std::thread{ TickMachine(), std::move(m_promise.get_future()) };

        m_timer_running = true;
    }
}

void ClayEngine::TimingSystem::ResetTimer()
{
    TimingSystem::StopTimer();
    TimingSystem::StartTimer();
}

void ClayEngine::TimingSystem::StopTimer()
{
    if (m_timer_running)
    {
        m_promise.set_value();
        if (m_thread.joinable()) m_thread.join();

        m_timer_running = false;
    }

    if (m_timer) m_timer.reset();
    m_timer = nullptr;
    
    m_rs = nullptr;
}

void ClayEngine::TimingSystem::RunGameTick()
{
    m_timer->UpdateTimer();
    
    auto elapsed = static_cast<float>(m_timer->GetElapsedSeconds());
    OnUpdateCallback(elapsed);

    m_rs->Clear();
    m_rs->GetSpriteBatch()->Begin();

    OnDrawCallback();

    m_rs->GetSpriteBatch()->End();
    m_rs->Present();
}

uint32_t ClayEngine::TimingSystem::GetFPS()
{
    return m_timer->GetFramesPerSecond();
}

double ClayEngine::TimingSystem::GetTotalSeconds()
{
    if (m_timer)
    {
        return m_last_total_seconds = m_timer->GetTotalSeconds();
    }
    else
    {
        return m_last_total_seconds;
    }
}

void ClayEngine::TimingSystem::AddUpdateCallback(UpdateCallback fn)
{
    m_update_callbacks_mtx.lock();
    m_update_callbacks.push_back(fn);
    m_update_callbacks_mtx.unlock();
}

void ClayEngine::TimingSystem::OnUpdateCallback(float elapsedTime)
{
    m_update_callbacks_mtx.lock();
    std::for_each(m_update_callbacks.begin(), m_update_callbacks.end(), [&](UpdateCallback element) { element(elapsedTime); });
    m_update_callbacks_mtx.unlock();
}

void ClayEngine::TimingSystem::ClearUpdateCallbacks()
{
    m_update_callbacks_mtx.lock();
    //TODO: Consider adding CRITICAL_SECTION to this loop
    m_update_callbacks.clear();
    m_update_callbacks_mtx.unlock();
}

void ClayEngine::TimingSystem::AddDrawCallback(DrawCallback fn)
{
    m_draw_callbacks_mtx.lock();
    m_draw_callbacks.push_back(fn);
    m_draw_callbacks_mtx.unlock();
}

void ClayEngine::TimingSystem::OnDrawCallback()
{
    m_draw_callbacks_mtx.lock();
    std::for_each(m_draw_callbacks.begin(), m_draw_callbacks.end(), [&](DrawCallback element) { element(); });
    m_draw_callbacks_mtx.unlock();
}

void ClayEngine::TimingSystem::ClearDrawCallbacks()
{
    m_draw_callbacks_mtx.lock();
    m_draw_callbacks.clear();
    m_draw_callbacks_mtx.unlock();
}
