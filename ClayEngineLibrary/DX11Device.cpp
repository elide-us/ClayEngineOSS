#include "pch.h"
#include "DX11Device.h"

ClayEngine::DX11Device::DX11Device()
{
#pragma region Configure Device Debug Flag
#ifdef _DEBUG
	//Apply debug flags if using debug build
	if (SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_NULL, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, nullptr, nullptr, nullptr)))
		m_creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
#pragma endregion

#pragma region Create Device Factory
	// Create the device factory
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(m_factory.ReleaseAndGetAddressOf())), "ClayEngine CRITICAL: Failed to create DXGI factory");
#pragma endregion

#pragma region Check Device Options
	// Determines whether tearing support is available for fullscreen borderless windows.
	if (m_device_options & c_allow_tearing)
	{
		bool _tearing = false;

		ComPtr<IDXGIFactory5> factory5;
		HRESULT hr = m_factory.As(&factory5);
		if (SUCCEEDED(hr))
		{
			hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &_tearing, sizeof(_tearing));
		}

		if (FAILED(hr) || !_tearing)
		{
			m_device_options &= ~c_allow_tearing;
		}
	}

	// Disable HDR if we are on an OS that can't support FLIP swap effects
	if (m_device_options & c_enable_hdr)
	{
		ComPtr<IDXGIFactory5> factory5;
		if (FAILED(m_factory.As(&factory5)))
		{
			m_device_options &= ~c_enable_hdr;
		}
	}

	// Disable FLIP if not on a supporting OS
	if (m_device_options & c_flip_present)
	{
		ComPtr<IDXGIFactory4> factory4;
		if (FAILED(m_factory.As(&factory4)))
		{
			m_device_options &= ~c_flip_present;
		}
	}
#pragma endregion

	//ComPtr<IDXGIAdapter1> _adapter; // Our adapter, we'll discard this after device and context creation

#pragma region Enumerate Adapters
	//Try to cast the factory to version 6 and test for high performance GPU
	ComPtr<IDXGIFactory6> _factory6;
	if (SUCCEEDED(m_factory.As(&_factory6)))
	{
		for (UINT index = 0;
			SUCCEEDED(_factory6->EnumAdapterByGpuPreference(index,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				IID_PPV_ARGS(m_adapter.ReleaseAndGetAddressOf())));
				index++)
		{
			DXGI_ADAPTER_DESC1 _desc;
			ThrowIfFailed(m_adapter->GetDesc1(&_desc), "ClayEngine ERROR: Failed to get DESC from adapter");

			if (_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue; // Skip software adapter

			break;
		}
	}

	if (!m_adapter) // If we fail to cast to version 6, we'll use the legacy method
	{
		for (UINT index = 0;
			SUCCEEDED(m_factory->EnumAdapters1(index, m_adapter.ReleaseAndGetAddressOf()));
			index++)
		{
			DXGI_ADAPTER_DESC1 _desc;
			ThrowIfFailed(m_adapter->GetDesc1(&_desc), "ClayEngine ERROR: Failed to get DESC from adapter");

			if (_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue; // Skip software adapter

			break;
		}
	}
#pragma endregion
}

ClayEngine::DX11Device::~DX11Device()
{
	if (m_adapter)
	{
		m_adapter.Reset();
		m_adapter = nullptr;
	}

	if (m_factory)
	{
		m_factory.Reset();
		m_factory = nullptr;
	}
}

const ClayEngine::FactoryRaw ClayEngine::DX11Device::GetFactory() const
{
	if (m_factory)
		return m_factory.Get();
	else
		return nullptr;
}

const ClayEngine::AdapterPtr ClayEngine::DX11Device::GetAdapter() const
{
	if (m_adapter)
		return m_adapter.Get();
	else
		return nullptr;
}
