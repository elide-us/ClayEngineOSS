#include "pch.h"
#include "DX11Resources.h"

#include "Strings.h"
#include "WindowSystem.h"

ClayEngine::DX11Resources::DX11Resources(Affinity affinityId, UINT flags)
	: m_affinity(affinityId), m_creation_flags(flags)
{
	StartResources();

	StartPipeline();
}

ClayEngine::DX11Resources::~DX11Resources()
{
	StopPipeline();

	StopResources();
}

//TODO:  DX11Resources needs to be reworked, this D3D11CreateDevice call is not working
void ClayEngine::DX11Resources::StartResources()
{
#pragma region Configure Device Debug Flag
	//Apply debug flags if using debug build
#ifdef _DEBUG
	m_sdk_layers = SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_NULL, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, nullptr, nullptr, nullptr));
#endif
	if (m_sdk_layers) m_creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
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

	AdapterPtr _adapter; // Our adapter, we'll discard this after device and context creation

#pragma region Enumerate Adapters
	//Try to cast the factory to version 6 and test for high performance GPU
	ComPtr<IDXGIFactory6> _factory6;
	if (SUCCEEDED(m_factory.As(&_factory6)))
	{
		for (UINT index = 0;
			SUCCEEDED(_factory6->EnumAdapterByGpuPreference(index,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				IID_PPV_ARGS(_adapter.ReleaseAndGetAddressOf())));
				index++)
		{
			DXGI_ADAPTER_DESC1 _desc;
			ThrowIfFailed(_adapter->GetDesc1(&_desc));

			if (_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue; // Skip software adapter

			break;
		}
	}

	if (!_adapter) // If we fail to cast to version 6, we'll use the legacy method
	{
		for (UINT index = 0;
			SUCCEEDED(m_factory->EnumAdapters1(index, _adapter.ReleaseAndGetAddressOf()));
			index++)
		{
			DXGI_ADAPTER_DESC1 _desc;
			ThrowIfFailed(_adapter->GetDesc1(&_desc));

			if (_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue; // Skip software adapter

			break;
		}
	}
#pragma endregion

	DevicePtr _device;
	ContextPtr _context;

	HRESULT _hr = E_FAIL;
	if (_adapter)
	{
		_hr = D3D11CreateDevice(
			_adapter.Get(),
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			m_creation_flags,
			c_feature_levels,
			c_feature_level_count,
			D3D11_SDK_VERSION,
			_device.GetAddressOf(),
			&m_feature_level,
			_context.GetAddressOf()
		);
	}

	ThrowIfFailed(_hr, "ClayEngine CRITICAL: Failed to create Direct3D device");

	ThrowIfFailed(_device.As(&m_device));
	ThrowIfFailed(_context.As(&m_context));
	ThrowIfFailed(_context.As(&m_annotation));
}

void ClayEngine::DX11Resources::StopResources()
{
	if (m_context)
	{
		m_context.Reset();
		m_context = nullptr;
	}

	if (m_device)
	{
		m_device.Reset();
		m_device = nullptr;
	}
}

void ClayEngine::DX11Resources::RestartResources()
{
	StopResources();

	StartResources();
}

void ClayEngine::DX11Resources::StartPipeline()
{
	auto hwnd = Services::GetService<WindowSystem>(m_affinity)->GetWindowHandle();

	// Clear the previous window size specific context.
	ID3D11RenderTargetView* views[] = { nullptr };
	m_context->OMSetRenderTargets(_countof(views), views, nullptr);
	m_rendertarget.Reset();
	m_depthstencil.Reset();
	m_context->Flush();

	//const UINT backbuffer_width = static_cast<UINT>(rect.right - rect.left);
	auto backbuffer_width = UINT(1920);
	auto backbuffer_height = UINT(1080);
	auto backbuffer_count = 2u;

	const DXGI_FORMAT rendertarget_format = { DXGI_FORMAT_B8G8R8A8_UNORM };
	const DXGI_FORMAT depthstencil_format = { DXGI_FORMAT_D24_UNORM_S8_UINT };

	const auto depthstencil_desc = CD3D11_TEXTURE2D_DESC{ depthstencil_format, backbuffer_width, backbuffer_height, 1, 1, D3D11_BIND_DEPTH_STENCIL };
	const auto depthstencilview_desc = CD3D11_DEPTH_STENCIL_VIEW_DESC{ D3D11_DSV_DIMENSION_TEXTURE2D };

	// This is definitely not right, this is checking ResizeBuffers every frame?!?!
	if (m_swapchain)
	{
		HRESULT hr = m_swapchain->ResizeBuffers(backbuffer_count, backbuffer_width, backbuffer_height, rendertarget_format, 0);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			//Services::GetService<RenderSystem>(std::this_thread::get_id())->RestartRenderSystem();
			//TODO: This should be signaling WindowSystem::OnDeviceLost() or something like that...
			return;
		}
		else
		{
			ThrowIfFailed(hr);
		}
	}
	else
	{
		ComPtr<IDXGIDevice1> dxgi_device;
		ThrowIfFailed(m_device.As(&dxgi_device));

		ComPtr<IDXGIAdapter> dxgi_adapter;
		ThrowIfFailed(dxgi_device->GetAdapter(dxgi_adapter.GetAddressOf()));

		ComPtr<IDXGIFactory2> dxgi_factory;
		ThrowIfFailed(dxgi_adapter->GetParent(IID_PPV_ARGS(dxgi_factory.GetAddressOf())));

		// Create a descriptor for the swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
		swapchain_desc.Width = backbuffer_width;
		swapchain_desc.Height = backbuffer_height;
		swapchain_desc.Format = rendertarget_format;
		swapchain_desc.SampleDesc.Count = 1;
		swapchain_desc.SampleDesc.Quality = 0;
		swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchain_desc.BufferCount = backbuffer_count;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapchain_desc_fs = {};
		swapchain_desc_fs.Windowed = TRUE;

		// Create a SwapChain from a Win32 window.
		ThrowIfFailed(dxgi_factory->CreateSwapChainForHwnd(m_device.Get(), hwnd,
			&swapchain_desc, &swapchain_desc_fs, nullptr, m_swapchain.ReleaseAndGetAddressOf()));

		// This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
		ThrowIfFailed(dxgi_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
	}

	// Obtain the backbuffer for this window which will be the final 3D rendertarget.
	ThrowIfFailed(m_swapchain->GetBuffer(0, IID_PPV_ARGS(m_rendertarget_buffer.GetAddressOf())));

	// Create a view interface on the rendertarget to use on bind.
	ThrowIfFailed(m_device->CreateRenderTargetView(m_rendertarget_buffer.Get(), nullptr, m_rendertarget.ReleaseAndGetAddressOf()));

	// Allocate a 2-D surface as the depth/stencil buffer and
	// create a DepthStencil view on this surface to use on bind.
	ThrowIfFailed(m_device->CreateTexture2D(&depthstencil_desc, nullptr, m_depthstencil_buffer.GetAddressOf()));

	ThrowIfFailed(m_device->CreateDepthStencilView(m_depthstencil_buffer.Get(), &depthstencilview_desc, m_depthstencil.ReleaseAndGetAddressOf()));
}

void ClayEngine::DX11Resources::StopPipeline()
{
	if (m_depthstencil)
	{
		m_depthstencil.Reset();
		m_depthstencil = nullptr;
	}

	if (m_rendertarget)
	{
		m_rendertarget.Reset();
		m_rendertarget = nullptr;
	}

	if (m_swapchain)
	{
		m_swapchain.Reset();
		m_swapchain = nullptr;
	}
}

void ClayEngine::DX11Resources::RestartPipeline()
{
	StopPipeline();

	StartPipeline();
}

void ClayEngine::DX11Resources::OnDeviceLost()
{

}

ClayEngine::DeviceRaw ClayEngine::DX11Resources::GetDevice()
{
	if (m_device)
		return m_device.Get();
	else
		return nullptr;
}

ClayEngine::ContextRaw ClayEngine::DX11Resources::GetContext()
{
	if (m_context)
		return m_context.Get();
	else
		return nullptr;
}

ClayEngine::SwapChainRaw ClayEngine::DX11Resources::GetSwapChain()
{
	if (m_swapchain)
		return m_swapchain.Get();
	else
		return nullptr;
}

ClayEngine::RenderTargetViewRaw ClayEngine::DX11Resources::GetRTV()
{
	if (m_rendertarget)
		return m_rendertarget.Get();
	else
		return nullptr;
}

ClayEngine::DepthStencilViewRaw ClayEngine::DX11Resources::GetDSV()
{
	if (m_depthstencil)
		return m_depthstencil.Get();
	else
		return nullptr;
}

ClayEngine::RenderTargetViewPtr ClayEngine::DX11Resources::GetRTVPtr()
{
	return m_rendertarget;
}

ClayEngine::DepthStencilViewPtr ClayEngine::DX11Resources::GetDSVPtr()
{
	return m_depthstencil;
}
