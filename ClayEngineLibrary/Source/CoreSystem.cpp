#include "pch.h"
#include "CoreSystem.h"

using namespace ClayEngine;

ClientCoreSystem::ClientCoreSystem()
{
	m_timer = Services::MakeService<TimingSystem>();
	m_timer->AddUpdateCallback([&](float elapsedTime) { Update(elapsedTime); });
}

ClientCoreSystem::~ClientCoreSystem()
{
	m_timer->StopTimer();

	StopServices();

	if (m_timer)
	{
		Services::RemoveService<TimingSystem>();
		m_timer.reset();
		m_timer = nullptr;
	}
}

void ClientCoreSystem::StopServices()
{
	//if (m_model) m_model.reset();
	//m_model = nullptr;

	//if (m_shape) m_shape.reset();
	//m_shape = nullptr;

	//if (m_grid) m_grid.reset();
	//m_grid = nullptr;

	//if (m_primitive) m_primitive.reset();
	//m_primitive = nullptr;

	//if (m_vf) m_vf.reset();
	//m_vf = nullptr;

	//if (m_chase) m_chase.reset();
	//m_chase = nullptr;

	if (m_sensorium)
	{
		Services::RemoveService<Sensorium>();
		m_sensorium.reset();
		m_sensorium = nullptr;
	}

	if (m_network)
	{
		Services::RemoveService<NetworkSystem>();
		m_network->StopClientConnection();
		m_network.reset();
		m_network = nullptr;
	}

	if (m_content)
	{
		Services::RemoveService<ContentSystem>();
		m_content.reset();
		m_content = nullptr;
	}

	if (m_render)
	{
		Services::RemoveService<RenderSystem>();
		m_render.reset();
		m_render = nullptr;
	}
		
	m_state = ClientCoreState::Default;
}

void ClientCoreSystem::SetState(ClientCoreState state)
{
	m_state_changed = true;
	m_state = state;
}

bool ClientCoreSystem::GetStateChanged()
{
	return m_state_changed;
}

void ClientCoreSystem::OnStateChanged()
{
	std::wstringstream wss;
	
	m_state_changed = false;

	switch (m_state)
	{
	case ClientCoreState::Default: // The platform has been initialized, begin loading rendering and content systems
		{
			wss << __func__ << L"() INFO: ClientCoreState::Default";
			WriteLine(wss.str());

			// Initialize the API for the rendering system. This class creates and destroys resources related
			// to the graphics device, and provides graphics related objects for use by the engine.
			m_render = Services::MakeService<RenderSystem>();

			// In the future, this is meant to be templatized and used by defining which rendering system to use.
			// Currently it just loads simple DX11 modules
			m_render->StartRenderSystem();

			// Initialize the API for the content system. This class will load resources for rendering from disk.
			m_content = Services::MakeService<ContentSystem>();

			// Shift to the next state.
			m_state = ClientCoreState::Initializing;
			m_state_changed = true;
		}
		break;
	case ClientCoreState::Initializing:
		{
			wss << __func__ << L"() INFO: ClientCoreState::Initializing";
			WriteLine(wss.str());

			// Initialize the user interface root class and hook it up to the ticker
			m_sensorium = Services::MakeService<Sensorium>();

			//m_primitive = std::make_unique<RenderPrimitive>();
			//m_grid = std::make_unique<RenderGrid>();
			//m_shape = std::make_unique<RenderShape>();
			//m_model = std::make_unique<RenderModel>();

			//m_chase = std::make_unique<SquareChase>();
			
			//m_vfr = Services::MakeService<VoxelFarmCellRender>();
			//m_vf = Services::MakeService<VoxelFarmThread>();

			// Temporary code testing network client connection. Server has to be running for this to work.
			m_network = Services::MakeService<ClayEngine::Networking::NetworkSystem>();
			m_network->SetClientConnectionHints(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			m_network->SetClientConnectionPort(48000);
			//m_network->StartClientConnection("73.210.118.242");

			// Shift to the next state, which is just our prototype debug run state
			m_state = ClientCoreState::DebugRunning;
			m_state_changed = true;
		}
		break;
	case ClientCoreState::DebugRunning:
		{
			wss << __func__ << L"() INFO: ClientCoreState::DebugRunning";
			WriteLine(wss.str());

			m_timer->StartTimer();
		}
		break;
	default:
		break;
	}
}
