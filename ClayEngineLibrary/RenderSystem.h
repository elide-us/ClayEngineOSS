#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* Rendering system API class                                                 */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include "Services.h"

#include "DX11Resources.h"
#include "DX11PrimitivePipeline.h"

#include "SpriteBatch.h"

namespace ClayEngine
{
	using SpriteBatchPtr = std::unique_ptr<DirectX::SpriteBatch>;
	using SpriteBatchRaw = DirectX::SpriteBatch*;

	/// <summary>
	/// API entry point for the graphical pipeline and GPU resources
	/// </summary>
	class RenderSystem
	{
		AffinityData m_affinity_data;

		DX11ResourcesPtr m_resources = nullptr;

		SpriteBatchPtr m_spritebatch = nullptr;
		PrimitivePipelinePtr m_primitive = nullptr;

		bool m_device_lost = true;

	public:
		RenderSystem(AffinityData affinityData);
		~RenderSystem();

		void StartRenderSystem();
		void StopRenderSystem();
		void RestartRenderSystem();

		void Clear();
		void Present();

		void OnDeviceLost();

		DX11ResourcesRaw GetDeviceResources();

		SpriteBatchRaw GetSpriteBatch();
		PrimitivePipelineRaw GetPrimitivePipeline();
	};
	using RenderSystemPtr = std::unique_ptr<RenderSystem>;
	using RenderSystemRaw = RenderSystem*;

	//class RenderSystemExtension
	//{
	//protected:
	//	RenderSystemRaw m_rs = nullptr;
	//public:
	//	RenderSystemExtension()
	//	{
	//		m_rs = Services::GetService<RenderSystem>(std::this_thread::get_id());
	//	}
	//	~RenderSystemExtension()
	//	{
	//		m_rs = nullptr;
	//	}
	//};

	//class SpriteBatchExtension
	//{
	//protected:
	//	SpriteBatchRaw m_spritebatch = nullptr;
	//public:
	//	SpriteBatchExtension()
	//	{
	//		m_spritebatch = Services::GetService<DirectX::SpriteBatch>(m_affinity_data.this_thread);
	//	}
	//	~SpriteBatchExtension()
	//	{
	//		m_spritebatch = nullptr;
	//	}
	//};
}
