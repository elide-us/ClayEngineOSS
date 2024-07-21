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

using Microsoft::WRL::ComPtr;

namespace ClayEngine
{
	using FactoryPtr = ComPtr<IDXGIFactory2>;
	using AdapterPtr = ComPtr<IDXGIAdapter1>;
	using DevicePtr = ComPtr<ID3D11Device>;
	using DeviceRaw = ID3D11Device*;
	using ContextPtr = ComPtr<ID3D11DeviceContext>;
	using ContextRaw = ID3D11DeviceContext*;
	using SwapChainPtr = ComPtr<IDXGISwapChain1>;
	using SwapChainRaw = IDXGISwapChain1*;
	using RenderTargetPtr = ComPtr<ID3D11Texture2D>;
	using DepthStencilPtr = ComPtr<ID3D11Texture2D>;
	using RenderTargetViewPtr = ComPtr<ID3D11RenderTargetView>;
	using RenderTargetViewRaw = ID3D11RenderTargetView*;
	using DepthStencilViewPtr = ComPtr<ID3D11DepthStencilView>;
	using DepthStencilViewRaw = ID3D11DepthStencilView*;

	constexpr unsigned int c_flip_present = 0x1;
	constexpr unsigned int c_allow_tearing = 0x2;
	constexpr unsigned int c_enable_hdr = 0x4;

	constexpr D3D_FEATURE_LEVEL c_feature_levels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};
	constexpr auto c_feature_level_count = __crt_countof(c_feature_levels);

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
		Affinity m_affinity;

		bool m_sdk_layers = false;
		unsigned int m_device_options = 0x0;

		UINT m_creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
		D3D_FEATURE_LEVEL m_min_feature_level = D3D_FEATURE_LEVEL_11_1;

		D3D_FEATURE_LEVEL m_feature_level = {};

		FactoryPtr m_factory = nullptr;
		DevicePtr m_device = nullptr;
		ContextPtr m_context = nullptr;
		ContextPtr m_annotation = nullptr;

		bool m_device_lost = true;

		SwapChainPtr m_swapchain = nullptr;

		RenderTargetPtr m_rendertarget_buffer = nullptr;
		DepthStencilPtr m_depthstencil_buffer = nullptr;

		RenderTargetViewPtr m_rendertarget = nullptr;
		DepthStencilViewPtr m_depthstencil = nullptr;

	public:
		DX11Resources(Affinity affinityId, UINT flags);
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
