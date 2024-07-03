#include "pch.h"
#include "RenderSystem.h"

#include "WindowSystem.h"

using namespace DirectX;

ClayEngine::RenderSystem::RenderSystem(Affinity affinityId)
	: m_affinity(affinityId)
{
	StartRenderSystem();
}

ClayEngine::RenderSystem::~RenderSystem()
{
	StopRenderSystem();
}

void ClayEngine::RenderSystem::StartRenderSystem()
{
	// This is the graphics device resource, context and object factory
	m_resources = Services::MakeService<DX11Resources>(m_affinity, 0); // D3D11_CREATE_DEVICE_BGRA_SUPPORT

	// This SpriteBatch can be used for 2D games and User Interface elements
	m_spritebatch = Services::MakeDxService<SpriteBatch>(m_affinity, m_resources->GetContext());

	// This primitive pipeline is designed to render basic 3D vertex buffer objects
	m_primitive = Services::MakeService<PrimitivePipeline>(m_affinity, m_resources->GetContext());
}

void ClayEngine::RenderSystem::StopRenderSystem()
{
	if (m_primitive)
	{
		Services::RemoveService<PrimitivePipeline>(m_affinity);
		m_primitive.reset();
		m_primitive = nullptr;
	}

	if (m_spritebatch)
	{
		Services::RemoveService<SpriteBatch>(m_affinity);
		m_spritebatch.reset();
		m_spritebatch = nullptr;
	}

	if (m_resources)
	{
		Services::RemoveService<DX11Resources>(m_affinity);
		m_resources.reset();
		m_resources = nullptr;
	}
}

//IDEA: Consider a "restart" count tracker to keep this from restarting forever...
void ClayEngine::RenderSystem::RestartRenderSystem()
{
	StopRenderSystem();

	//TODO: Services::GetService<ContentSystem>()->StopContentSystem();

	StartRenderSystem();

	//TODO: Service::GetService<ContentSystem>()->StartContentSystem();
}

void ClayEngine::RenderSystem::Clear()
{
	//TODO: Check if I need to clear the SpriteBatch

	auto ctx = m_resources->GetContext();

	ctx->ClearRenderTargetView(m_resources->GetRTV(), Colors::DarkSlateGray);
	ctx->ClearDepthStencilView(m_resources->GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	ctx->OMSetRenderTargets(1, m_resources->GetRTVPtr().GetAddressOf(), m_resources->GetDSV());

	auto w = float(Services::GetService<WindowSystem>(m_affinity)->GetWindowWidth());
	auto h = float(Services::GetService<WindowSystem>(m_affinity)->GetWindowHeight());

	auto viewport = CD3D11_VIEWPORT{ 0.0f, 0.0f, w, h };
	//CD3D11_VIEWPORT viewport();
	ctx->RSSetViewports(1, &viewport);
}

void ClayEngine::RenderSystem::Present()
{
	auto hr = m_resources->GetSwapChain()->Present(1, 0);

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		m_device_lost = true; //TODO: OnDeviceLost()
	}
	else
	{
		ThrowIfFailed(hr); //IDEA: This can be handled by a sentinel service and caught instead of thrown out to the OS
	}
}

void ClayEngine::RenderSystem::OnDeviceLost()
{

}

ClayEngine::DX11ResourcesRaw ClayEngine::RenderSystem::GetDeviceResources()
{
	if (m_resources)
		return m_resources.get();
	else
		return nullptr;
}

ClayEngine::SpriteBatchRaw ClayEngine::RenderSystem::GetSpriteBatch()
{
	if (m_spritebatch)
		return m_spritebatch.get();
	else
		return nullptr;
}

ClayEngine::PrimitivePipelineRaw ClayEngine::RenderSystem::GetPrimitivePipeline()
{
	if (m_primitive)
		return m_primitive.get();
	else
		return nullptr;

}
