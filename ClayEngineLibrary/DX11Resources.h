#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* DirectX 11 Device Resources                                                */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include "Strings.h"
#include "Services.h"

#include "DX11DeviceFactory.h"

using Microsoft::WRL::ComPtr;

namespace ClayEngine
{
	using FactoryPtr = ComPtr<IDXGIFactory2>;
	using DevicePtr = ComPtr<ID3D11Device1>;
	using DeviceRaw = ID3D11Device1*;
	using ContextPtr = ComPtr<ID3D11DeviceContext1>;
	using ContextRaw = ID3D11DeviceContext1*;
	using SwapChainPtr = ComPtr<IDXGISwapChain1>;
	using SwapChainRaw = IDXGISwapChain1*;

	using RenderTargetPtr = ComPtr<ID3D11Texture2D>;
	using DepthStencilPtr = ComPtr<ID3D11Texture2D>;
	using RenderTargetViewPtr = ComPtr<ID3D11RenderTargetView>;
	using RenderTargetViewRaw = ID3D11RenderTargetView*;
	using DepthStencilViewPtr = ComPtr<ID3D11DepthStencilView>;
	using DepthStencilViewRaw = ID3D11DepthStencilView*;

	inline DXGI_FORMAT NoSRGB(DXGI_FORMAT format) noexcept
	{
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
		default:                                return format;
		}
	}

	inline long ComputeIntersectionArea(long ax1, long ay1, long ax2, long ay2, long bx1, long by1, long bx2, long by2) noexcept
	{
		return std::max(0l, std::min(ax2, bx2) - std::max(ax1, bx1)) * std::max(0l, std::min(ay2, by2) - std::max(ay1, by1));
	}

	/// <summary>
	/// This represents the COM device resources that provide device and context services to the pipeline.
	/// </summary>
	class DX11Resources
	{
		AffinityData m_affinity_data;

		D3D_FEATURE_LEVEL m_feature_level = {};

		DevicePtr m_device = nullptr;
		ContextPtr m_device_context = nullptr;
		SwapChainPtr m_swapchain = nullptr;
		ContextPtr m_annotation = nullptr;

		D3D11_VIEWPORT m_viewport = {};
		DXGI_COLOR_SPACE_TYPE m_colorspace = {};

		DXGI_FORMAT m_backbuffer_format = DXGI_FORMAT_B8G8R8A8_UNORM;
		DXGI_FORMAT m_depthbuffer_format = DXGI_FORMAT_D32_FLOAT;
		UINT m_backbuffer_count = 2;

		// Old
		bool m_device_lost = true;
	
		RenderTargetPtr m_rendertarget_context = nullptr;
		DepthStencilPtr m_depthstencil_context = nullptr;

		RenderTargetViewPtr m_rendertarget = nullptr;
		DepthStencilViewPtr m_depthstencil = nullptr;

	public:
		DX11Resources(AffinityData affinityData);
		~DX11Resources();

		void StartResources();
		void StopResources();
		void RestartResources();

		void StartPipeline();
		void StopPipeline();
		void RestartPipeline();

		void OnDeviceLost();

		DeviceRaw GetDevice();
		ContextRaw GetContext();
		SwapChainRaw GetSwapChain();
		RenderTargetViewRaw GetRTV();
		DepthStencilViewRaw GetDSV();
		RenderTargetViewPtr GetRTVPtr();
		DepthStencilViewPtr GetDSVPtr();
	};
	using DX11ResourcesPtr = std::unique_ptr<DX11Resources>;
	using DX11ResourcesRaw = DX11Resources*;

	//TODO: These extensions may not function correctly due to Affinity
	//class DX11DeviceExtension
	//{
	//protected:
	//	DeviceRaw m_dx11device = nullptr;
	//public:
	//	DX11DeviceExtension()
	//	{
	//		m_dx11device = Services::GetService<DX11Resources>(std::this_thread::get_id())->GetDevice();
	//	}
	//	~DX11DeviceExtension()
	//	{
	//		m_dx11device = nullptr;
	//	}
	//};

	//class DX11ContextExtension
	//{
	//protected:
	//	ContextRaw m_dx11context = nullptr;
	//public:
	//	DX11ContextExtension()
	//	{
	//		m_dx11context = Services::GetService<DX11Resources>(std::this_thread::get_id())->GetContext();
	//	}
	//	~DX11ContextExtension()
	//	{
	//		m_dx11context = nullptr;
	//	}
	//};
}
