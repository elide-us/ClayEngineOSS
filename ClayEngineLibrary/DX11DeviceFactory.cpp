#include "pch.h"
#include "DX11DeviceFactory.h"

void ClayEngine::DX11DeviceFactory::createFactory()
{
#pragma region Create Device Factory
	// Create the device factory
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(m_factory.ReleaseAndGetAddressOf())), "ClayEngine CRITICAL: Failed to create DXGI factory");
#pragma endregion

}

ClayEngine::DX11DeviceFactory::DX11DeviceFactory(AffinityData affinityData)
{
	m_affinity_data = affinityData;

#pragma region Configure Device Debug Flag
#ifdef _DEBUG
	//Apply debug flags if using debug build
	if (SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_NULL, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, nullptr, nullptr, nullptr)))
		m_creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
#pragma endregion

	createFactory();

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
}

ClayEngine::DX11DeviceFactory::~DX11DeviceFactory()
{
	if (m_factory)
	{
		m_factory.Reset();
		m_factory = nullptr;
	}
}

const ClayEngine::FactoryPtr ClayEngine::DX11DeviceFactory::GetFactory()
{
	if (!m_factory->IsCurrent())
	{
		createFactory();
	}

	return m_factory;
}

