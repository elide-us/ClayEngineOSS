#include "pch.h"
#include "DX11PrimitivePipeline.h"

#include "Platform.h"

#pragma region Basic Vertex Position Color (VPC) Primitive Pipeline
ClayEngine::Graphics::PrimitivePipeline::PrimitivePipeline(ContextRaw context)
{
	m_batch = std::make_unique<PrimitiveBatchVPC>(context);
}

ClayEngine::Graphics::PrimitivePipeline::~PrimitivePipeline()
{
	m_batch.reset();
	m_batch = nullptr;
}

void ClayEngine::Graphics::PrimitivePipeline::Draw(const std::vector<DirectX::SimpleMath::Vector3>& vbo)
{
	m_batch->Begin();

	//m_batch->DrawTriangle(VertexPositionColor(Vector3::Zero, DirectX::Colors::White),
	//	VertexPositionColor(Vector3::One, DirectX::Colors::White),
	//	VertexPositionColor(Vector3::Down, DirectX::Colors::White)
	//);

	for (auto i = 0ull; i < vbo.size(); i += 3)
	{
		m_batch->DrawTriangle(
			DirectX::VertexPositionColor(vbo[i], DirectX::Colors::White),
			DirectX::VertexPositionColor(vbo[i + 1ull], DirectX::Colors::White),
			DirectX::VertexPositionColor(vbo[i + 2ull], DirectX::Colors::White)
		);
	}

	m_batch->End();
}
#pragma endregion

#pragma region View Camera
ClayEngine::Graphics::Camera::Camera(RECT size)
{
	m_world = std::make_unique<DirectX::SimpleMath::Matrix>(m_world_origin);

	m_view = std::make_unique<DirectX::SimpleMath::Matrix>(DirectX::SimpleMath::Matrix::CreateLookAt(
		DirectX::SimpleMath::Vector3(2.f, 2.f, 2.f), // Look from
		DirectX::SimpleMath::Vector3::Zero, // Look at
		DirectX::SimpleMath::Vector3::UnitY // Y axis is Up
	));

	m_projection = std::make_unique<DirectX::SimpleMath::Matrix>(DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
		m_proj_fov, // FoV
		float(size.left + size.right) / float(size.top + size.bottom), // Aspect ratio
		m_proj_nearclip, // Near clip
		m_proj_farclip // Far clip
	));
}

ClayEngine::Graphics::Camera::~Camera()
{
	m_projection.reset();
	m_projection = nullptr;

	m_view.reset();
	m_view = nullptr;

	m_world.reset();
	m_world = nullptr;
}

void ClayEngine::Graphics::Camera::Update(float elapsedTime)
{
	//TODO: Update camera from input
}
#pragma endregion

#pragma region Shader Pipeline
ClayEngine::Graphics::CameraEffects::CameraEffects(Microsoft::WRL::ComPtr<ID3D11Device> device, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& projection)
{
	m_effect = std::make_unique<DirectX::BasicEffect>(device.Get());
	m_effect->SetVertexColorEnabled(true);

	m_states = std::make_unique<DirectX::CommonStates>(device.Get());

	m_effect->GetVertexShaderBytecode(&m_shaderCode, &m_shaderCodeLength);

	ClayEngine::Platform::ThrowIfFailed(
		device->CreateInputLayout(
			DirectX::VertexPositionColor::InputElements,
			DirectX::VertexPositionColor::InputElementCount,
			m_shaderCode, m_shaderCodeLength,
			m_inputLayout.ReleaseAndGetAddressOf()
		)
	);

	m_effect->SetView(view);
	m_effect->SetProjection(projection);

	//TODO: Raster Description Tag Struct with Presets
	CD3D11_RASTERIZER_DESC rasterDesc(
		D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE,
		D3D11_DEFAULT_DEPTH_BIAS, D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
		D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, TRUE, FALSE, TRUE, FALSE
	);

	ClayEngine::Platform::ThrowIfFailed(
		device->CreateRasterizerState(
			&rasterDesc, m_rasterizer.ReleaseAndGetAddressOf()
		)
	);
}

ClayEngine::Graphics::CameraEffects::~CameraEffects()
{
	m_rasterizer.Reset();
	m_inputLayout.Reset();
	m_states.reset();
	m_effect.reset();
}

Microsoft::WRL::ComPtr<ID3D11InputLayout> ClayEngine::Graphics::CameraEffects::GetInputLayout() const
{
	return m_inputLayout;
}

void ClayEngine::Graphics::CameraEffects::SetState(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	context->OMSetBlendState(m_states->Opaque(), DirectX::Colors::Black, 0xFFFFFFFF);
	//context->OMSetBlendState(m_states->AlphaBlend(), DirectX::Colors::White, 0xFFFFFFFF);
	//context->OMSetBlendState(m_states->Additive(), DirectX::Colors::White, 0xFFFFFFFF);
	//context->OMSetBlendState(m_states->NonPremultiplied(), DirectX::Colors::White, 0xFFFFFFFF);

	context->OMSetDepthStencilState(m_states->DepthNone(), 0);
	//context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	//context->OMSetDepthStencilState(m_states->DepthRead(), 0);

	//context->RSSetState(m_raster.Get());
	//context->RSSetState(m_states->CullNone());
	//context->RSSetState(m_states->CullCounterClockwise()); // Backface culling default
	//context->RSSetState(m_states->CullClockwise());  // Backface culling left-handed on right
	context->RSSetState(m_states->Wireframe()); // Wireframe shows both sides

	auto samplerState = m_states->LinearClamp();
	//auto samplerState = m_states->LinearWrap();
	//auto samplerState = m_states->PointClamp();
	//auto samplerState = m_states->PointWrap();
	//auto samplerState = m_states->AnisotropicClamp();
	//auto samplerState = m_states->AnisotropicWrap();
	context->PSSetSamplers(0, 1, &samplerState);
}

void ClayEngine::Graphics::CameraEffects::SetWorld(const DirectX::SimpleMath::Matrix& world)
{
	m_effect->SetWorld(world);
}

void ClayEngine::Graphics::CameraEffects::SetContext(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	m_effect->Apply(context.Get());
}
#pragma endregion