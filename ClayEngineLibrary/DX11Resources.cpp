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

	ComPtr<IDXGIAdapter1> _adapter; // Our adapter, we'll discard this after device and context creation

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
			ThrowIfFailed(_adapter->GetDesc1(&_desc), "ClayEngine ERROR: Failed to get DESC from adapter");

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
			ThrowIfFailed(_adapter->GetDesc1(&_desc), "ClayEngine ERROR: Failed to get DESC from adapter");

			if (_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue; // Skip software adapter

			break;
		}
	}
#pragma endregion

	ComPtr<ID3D11Device> _device;
	ComPtr<ID3D11DeviceContext> _context;

#pragma region Create Device
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
	if (FAILED(_hr))
	{
		_hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_WARP,
			nullptr,
			m_creation_flags,
			c_feature_levels,
			c_feature_level_count,
			D3D11_SDK_VERSION,
			_device.GetAddressOf(),
			&m_feature_level,
			_context.GetAddressOf()
		);

		if (SUCCEEDED(_hr))
		{
			WriteLine("ClayEngine WARNING: Using WARP adapter for rendering.");
		}
	}

	ThrowIfFailed(_hr, "ClayEngine CRITICAL: Failed to create Direct3D device");
#pragma endregion

#pragma region Enable Debug Layer
#ifdef _DEBUG
	ComPtr<ID3D11Debug> _debug;
	if (SUCCEEDED(_device.As(&_debug)))
	{
		ComPtr<ID3D11InfoQueue> _info;
		if (SUCCEEDED(_debug.As(&_info)))
		{
			D3D11_MESSAGE_ID _hide[] =
			{
				D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS
			};

			D3D11_INFO_QUEUE_FILTER _filter = {};
			_filter.DenyList.NumIDs = static_cast<UINT>(std::size(_hide));
			_filter.DenyList.pIDList = _hide;

			_info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			_info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
			//_info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
			//_info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_INFO, true);
			//_info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_MESSAGE, true);

			_info->AddStorageFilterEntries(&_filter);
		}
	}
#endif
#pragma endregion

	ThrowIfFailed(_device.As(&m_device), "ClayEngine ERROR: Failed to cast device");
	ThrowIfFailed(_context.As(&m_device_context), "ClayEngine ERROR: Failed to cast context");
	ThrowIfFailed(_context.As(&m_annotation), "ClayEngine ERROR: Failed to cast annotation");
}

void ClayEngine::DX11Resources::StopResources()
{
	if (m_device_context)
	{
		m_device_context.Reset();
		m_device_context = nullptr;
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
	auto _hwnd = Services::GetService<WindowSystem>(m_affinity)->GetWindowHandle();
	if (!_hwnd) throw std::exception("WindowSystem must be initialised first");
	auto _size = Services::GetService<WindowSystem>(m_affinity)->GetWindowSize();

	// Clear the previous window size specific context.
	m_device_context->OMSetRenderTargets(0, nullptr, nullptr);
	m_rendertarget_context.Reset();
	m_depthstencil_context.Reset();
	m_rendertarget.Reset();
	m_depthstencil.Reset();
	m_device_context->Flush();

	const auto _bbw = std::max<UINT>(static_cast<UINT>(_size.right - _size.left), 1u);
	const auto _bbh = std::max<UINT>(static_cast<UINT>(_size.bottom - _size.top), 1u);
	const auto _bbf = (m_device_options & (c_flip_present | c_allow_tearing | c_enable_hdr))
		? NoSRGB(m_backbuffer_format) : m_backbuffer_format;

	// This is definitely not right, this is checking ResizeBuffers every frame?!?!
	if (m_swapchain)
	{
		//HRESULT hr = m_swapchain->ResizeBuffers(backbuffer_count, backbuffer_width, backbuffer_height, rendertarget_format, 0);

		//if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		//{
		//	//Services::GetService<RenderSystem>(std::this_thread::get_id())->RestartRenderSystem();
		//	//TODO: This should be signaling WindowSystem::OnDeviceLost() or something like that...
		//	return;
		//}
		//else
		//{
		//	ThrowIfFailed(hr);
		//}
	}
	else
	{
		DXGI_SWAP_CHAIN_DESC1 _scd = {};
		_scd.Width = _bbw;
		_scd.Height = _bbh;
		_scd.Format = _bbf;
		_scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		_scd.BufferCount = m_backbuffer_count;
		_scd.SampleDesc.Count = 1;
		_scd.SampleDesc.Quality = 0;
		_scd.Scaling = DXGI_SCALING_STRETCH;
		_scd.SwapEffect = (m_device_options & (c_flip_present | c_allow_tearing | c_enable_hdr))
			? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
		_scd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		_scd.Flags = (m_device_options & c_allow_tearing)
			? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC _fsscd = {};
		_fsscd.Windowed = true;

		HRESULT _hr = S_OK;
		_hr = m_factory->CreateSwapChainForHwnd(
			m_device.Get(), _hwnd, &_scd, &_fsscd, nullptr,
			m_swapchain.ReleaseAndGetAddressOf());
		if (FAILED(_hr)) throw std::exception("CreateSwapChainForHwnd");

		_hr = S_OK;
		_hr = m_factory->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_ALT_ENTER);
		if (FAILED(_hr)) throw std::exception("MakeWindowAssociation");


	}
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
	if (m_device_context)
		return m_device_context.Get();
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
