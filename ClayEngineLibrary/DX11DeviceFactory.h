#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* DirectX 11 Adapter Interrogator and DXGI Factory                           */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include <memory>

#include "Strings.h"
#include "Services.h"

using Microsoft::WRL::ComPtr;

namespace ClayEngine
{
	using FactoryPtr = ComPtr<IDXGIFactory2>;
	using FactoryRaw = IDXGIFactory2*;

	// Used by the swap chain SwapEffect property
	constexpr UINT c_flip_present = 0x1;
	constexpr UINT c_allow_tearing = 0x2;
	constexpr UINT c_enable_hdr = 0x4;

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

	/// <summary>
	/// This class interrogates the hardware in the system and creates a DXGI
	/// factory on an appropriate adapter, capabilities are determined and 
	/// flags are accessable to external sources for instantiating swap chains.
	/// This class should use Affinity 0, it is a process scope service.
	/// </summary>
	class DX11DeviceFactory
	{
		AffinityData m_affinity_data = {};

		FactoryPtr m_factory = nullptr;

		bool m_sdk_layers = false;

		const UINT c_flip_present = 0x1;
		const UINT c_allow_tearing = 0x2;
		const UINT c_enable_hdr = 0x4;
		
		UINT m_device_options = c_flip_present | c_allow_tearing | c_enable_hdr;
		UINT m_creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

		D3D_FEATURE_LEVEL m_min_feature_level = D3D_FEATURE_LEVEL_10_0;
		D3D_FEATURE_LEVEL m_feature_level = {};

	public:
		DX11DeviceFactory(AffinityData affinityData);
		~DX11DeviceFactory();

		const FactoryPtr GetFactory() const { return m_factory; }

		const bool GetSDKLayers() const { return m_sdk_layers; }
		const D3D_FEATURE_LEVEL& GetFeatureLevel() const { return m_feature_level; }
		const UINT GetCreationFlags() const { return m_creation_flags; }
		const UINT GetDeviceOptions() const { return m_device_options; }

		// Handle OnDeviceLost Events in this class and pass to WindowSystem, RenderSystem, etc.
		// Consider how to handle callbacks, the style we wrote works, but it's kinda a PITA.
	};
	using DX11DeviceFactoryPtr = std::unique_ptr<DX11DeviceFactory>;
	using DX11DeviceFactoryRaw = DX11DeviceFactory*;
}