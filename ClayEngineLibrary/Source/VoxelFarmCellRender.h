#pragma once

#include "ClayEngine.h"
#include "ClayMath.h"
#include "DX11Resources.h"
#include "CellData.h"
#include "DirectXHelpers.h"
#include "SimpleMath.h"
#include "WindowSystem.h"
#include "Model.h"

namespace ClayEngine
{
	namespace Game
	{
		using namespace DirectX;
		using namespace DirectX::SimpleMath;
		using namespace Microsoft::WRL;

		class VoxelFarmCellRender
			: public ClayEngine::Graphics::DX11DeviceExtension
			, public ClayEngine::Graphics::DX11ContextExtension
			, public ClayEngine::SimpleMath::Extensions::DrawExtension
		{

			Matrix m_world = Matrix::Identity;
			Matrix m_view = Matrix::Identity;
			Matrix m_proj = Matrix::Identity;

			using VertexType = VertexPositionNormalTexture;
			using EffectType = BasicEffect;

			using CommonStatesPtr = std::unique_ptr<CommonStates>;
			using EffectTypePtr = std::shared_ptr<EffectType>;
			//using PrimitiveBatchPtr = std::unique_ptr<PrimitiveBatch<VertexType>>;

			CommonStatesPtr m_states = nullptr;
			EffectTypePtr m_effect = nullptr;
			//PrimitiveBatchPtr m_batch = nullptr;
			
			//using InputLayoutComPtr = ComPtr<ID3D11InputLayout>;
			//InputLayoutComPtr m_inputLayout = {};

			VoxelFarm::CCellData* m_celldata = nullptr;

			using ModelPtr = std::unique_ptr<Model>;
			ModelPtr m_model;
			
			bool m_vboready = false;
			std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> m_vbdecl = nullptr;

		public:
			VoxelFarmCellRender()
			{
				m_states = std::make_unique<CommonStates>(m_dx11device);
				//m_batch = std::make_unique<PrimitiveBatch<VertexType>>(m_dx11context);
				m_vbdecl = std::make_shared<std::vector<D3D11_INPUT_ELEMENT_DESC>>(
					VertexType::InputElements, VertexType::InputElements + VertexType::InputElementCount);

				auto size = ClayEngine::Platform::WindowSystem::GetClientWindowSize();
				m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f, float(size.right) / float(size.bottom), 0.1f, 10.f);
				m_view = Matrix::CreateLookAt(Vector3(2.f, 2.f, 2.f), Vector3::Zero, Vector3::UnitY);

				m_effect = std::make_shared<EffectType>(m_dx11device);
				m_effect->EnableDefaultLighting();
				m_effect->SetLightingEnabled(true);

				//ThrowIfFailed(CreateInputLayoutFromEffect<VertexPosition>(m_dx11device, m_effect.get(), m_inputLayout.GetAddressOf()));

				m_effect->SetProjection(m_proj);
				m_effect->SetView(m_view);
			}

			std::unique_ptr<DirectX::Model> CreateFromVF()
			{
				uint64_t sizeInBytes = uint64_t(m_celldata->vertexCount[0]) * sizeof(VertexType);

				auto numVertices = uint32_t(m_celldata->vertexCount[0]);
				auto vertSize = static_cast<size_t>(uint64_t(m_celldata->vertexCount[0]) * sizeof(VertexType));
				auto verts = reinterpret_cast<const VertexType*>(m_celldata->vertices[0]);

				ComPtr<ID3D11Buffer> vb;
				{
					D3D11_BUFFER_DESC desc = {};
					desc.Usage = D3D11_USAGE_DEFAULT;
					desc.ByteWidth = static_cast<UINT>(vertSize);
					desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

					D3D11_SUBRESOURCE_DATA initData = { verts, 0, 0 };

					ThrowIfFailed(m_dx11device->CreateBuffer(&desc, &initData, vb.GetAddressOf()));
				}

				//auto indexSize = static_cast<size_t>()

				ComPtr<ID3D11Buffer> ib;
				{

				}

				auto part = new ModelMeshPart();
				//part->indexCount = header->numIndices;
				//part->startIndex = 0;
				part->vertexStride = sizeof(VertexType);
				//part->inputLayout = m_inputLayout;
				//part->indexBuffer = ib;
				part->vertexBuffer = vb;
				part->effect = m_effect;
				part->vbDecl = m_vbdecl;

				auto mesh = std::make_shared<ModelMesh>();
				mesh->ccw = false; // (flags & ModelLoader_CounterClockwise) != 0;
				mesh->pmalpha = false; //(flags & ModelLoader_PremultipledAlpha) != 0;
				BoundingSphere::CreateFromPoints(mesh->boundingSphere, numVertices, &verts->position, sizeof(VertexType));
				BoundingBox::CreateFromPoints(mesh->boundingBox, numVertices, &verts->position, sizeof(VertexType));
				mesh->meshParts.emplace_back(part);

				auto model = std::make_unique<Model>();
				model->meshes.emplace_back(mesh);

				return model;
			}

			void SetCellData(VoxelFarm::CCellData* celldata)
			{
				m_celldata = celldata;

				m_model = CreateFromVF();

				m_vboready = true;
			}

			void Draw() override
			{
				auto time = static_cast<float>(ticker->GetTotalSeconds() / 10);
				m_world = Matrix::CreateRotationZ(cosf(time));

				//m_dx11context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
				//m_dx11context->OMSetDepthStencilState(m_states->DepthNone(), 0);
				//m_dx11context->RSSetState(m_states->CullNone());
				//m_dx11context->IASetInputLayout(m_inputLayout.Get());

				//UINT pStrides[1] = { 1 };
				//UINT pOffsets[1] = { 0 };
				//m_dx11context->IASetVertexBuffers(0, 1, &m_buffer, pStrides, pOffsets);

				m_effect->SetWorld(m_world);
				m_effect->Apply(m_dx11context);

				if (m_vboready)
				{
					//m_batch->Begin();
					m_model->Draw(m_dx11context, *m_states, m_world, m_view, m_proj);
					//m_batch->End();
				}
			}
		};
		using VoxelFarmCellRenderPtr = std::unique_ptr<VoxelFarmCellRender>;
	}
}

