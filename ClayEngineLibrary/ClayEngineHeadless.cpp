#include "pch.h"
#include "ClayEngineHeadless.h"

#include "AsyncNetworkSystem.h"

namespace ClayEngine
{
	struct ClayEngineHeadlessEntryPoint
	{
		int operator()(FUTURE future, ClayEngineHeadless* context);
	};
}

ClayEngine::ClayEngineHeadless::ClayEngineHeadless(Affinity pRoot)
{
	m_affinity_data.root_thread = pRoot;
	m_thread = THREAD{ ClayEngineHeadlessEntryPoint(), std::move(m_promise.get_future()), this };
}

ClayEngine::ClayEngineHeadless::~ClayEngineHeadless()
{
	m_promise.set_value();
	if (m_thread.joinable())
		m_thread.join();
}

void ClayEngine::ClayEngineHeadless::SetContextAffinity(Affinity affinity)
{
	m_affinity_data.this_thread = affinity;
}

const ClayEngine::AffinityData& ClayEngine::ClayEngineHeadless::GetAffinityData() const
{
	return m_affinity_data;
}

int ClayEngine::ClayEngineHeadlessEntryPoint::operator()(FUTURE future, ClayEngineHeadless* context)
{
    context->SetContextAffinity(std::this_thread::get_id());
    auto _affinity = context->GetAffinityData();

    auto _network = Services::MakeService<AsyncNetworkSystem>(_affinity);

    while (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout)
    {
		//TODO: Put some cin console parsing to quit the server
        continue;
    }

    return 0;

}
