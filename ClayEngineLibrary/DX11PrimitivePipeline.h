#pragma once

#include "Effects.h"
#include "CommonStates.h"
#include "PlatformHelpers.h"

#include "SimpleMath.h"
#include "PrimitiveBatch.h"
#include "VertexTypes.h"

#include "DX11Resources.h"

namespace ClayEngine
{
	namespace Graphics
	{
		class PrimitivePipeline
		{
			using PrimitiveBatchVPC = DirectX::PrimitiveBatch<DirectX::VertexPositionColor>;
			using PrimitiveBatchVPCPtr = std::unique_ptr<PrimitiveBatchVPC>;

			PrimitiveBatchVPCPtr m_batch = nullptr;

		public:
			PrimitivePipeline(ContextRaw context);
			~PrimitivePipeline();

			// Pass a reference to a vertex buffer object that you want this pipeline to draw
			void Draw(const std::vector<DirectX::SimpleMath::Vector3>& vbo);
		};
		using PrimitivePipelinePtr = std::unique_ptr<PrimitivePipeline>;
		using PrimitivePipelineRaw = PrimitivePipeline*;

		// This class is ripe for accessor extensions for World, View, and Projection matrices
		class Camera
		{
			using MatrixPtr = std::unique_ptr<DirectX::SimpleMath::Matrix>;

			MatrixPtr m_world = nullptr;
			DirectX::SimpleMath::Matrix m_world_origin = DirectX::SimpleMath::Matrix::Identity;
			
			MatrixPtr m_view = nullptr;

			MatrixPtr m_projection = nullptr;
			float m_proj_fov = DirectX::XM_PI / 4.f;
			float m_proj_nearclip = 0.1f;
			float m_proj_farclip = 10.f;

			float m_pitch, m_yaw, m_roll;
		public:
			Camera(RECT size); // This size value probably comes from WindowClass now
			~Camera();

			void Update(float elapsedTime);

			DirectX::SimpleMath::Matrix& GetWorld() const { return *m_world; };
			DirectX::SimpleMath::Matrix& GetView() const { return *m_view; };
			DirectX::SimpleMath::Matrix& GetProjection() const { return *m_projection; };
		};
		using CameraPtr = std::unique_ptr<Camera>;
		using CameraRaw = Camera*;

		class CameraEffects
		{
			std::unique_ptr<DirectX::BasicEffect> m_effect;
			std::unique_ptr<DirectX::CommonStates> m_states;

			Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
			Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;

			// Wrap this into an RIAA object with a cleaner interface...
			const void* m_shaderCode;
			size_t m_shaderCodeLength;

		public:
			CameraEffects(Microsoft::WRL::ComPtr<ID3D11Device> device, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& projection);
			~CameraEffects();

			Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout() const;

			void SetState(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
			void SetWorld(const DirectX::SimpleMath::Matrix& world);
			void SetContext(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
		};
		using CameraEffectsPtr = std::unique_ptr<CameraEffects>;
		using CameraEffectsRaw = CameraEffects*;
	}
}