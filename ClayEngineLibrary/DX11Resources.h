#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine DirectX 11 Device Resources (C) 2022 Epoch Meridian, LLC.       */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include "ClayEngine.h"

namespace ClayEngine
{
	namespace Graphics
	{
		using Microsoft::WRL::ComPtr;

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

		/// <summary>
		/// This represents the COM device resources that provide device and context services to the pipeline.
		/// </summary>
		class DX11Resources
		{
			UINT m_creation_flags = 0;
			D3D_FEATURE_LEVEL m_feature_level = {};

			DevicePtr m_device = nullptr;
			ContextPtr m_context = nullptr;

			bool m_device_lost = true;

			SwapChainPtr m_swapchain = nullptr;

			RenderTargetPtr m_rendertarget_buffer = {};
			DepthStencilPtr m_depthstencil_buffer = {};

			RenderTargetViewPtr m_rendertarget = nullptr;
			DepthStencilViewPtr m_depthstencil = nullptr;

		public:
			DX11Resources(UINT flags = 0);
			~DX11Resources();

			void StartResources(UINT flags = 0);
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

		class DX11DeviceExtension
		{
		protected:
			DeviceRaw m_dx11device = nullptr;
		public:
			DX11DeviceExtension()
			{
				m_dx11device = Services::GetService<DX11Resources>(std::this_thread::get_id())->GetDevice();
			}
			~DX11DeviceExtension()
			{
				m_dx11device = nullptr;
			}
		};

		class DX11ContextExtension
		{
		protected:
			ContextRaw m_dx11context = nullptr;
		public:
			DX11ContextExtension()
			{
				m_dx11context = Services::GetService<DX11Resources>(std::this_thread::get_id())->GetContext();
			}
			~DX11ContextExtension()
			{
				m_dx11context = nullptr;
			}
		};
	}
}
