#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Rendering System API Class (C) 2022 Epoch Meridian, LLC.        */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include "Services.h"

#include "DX11Resources.h"
#include "DX11PrimitivePipeline.h"

#include "SpriteBatch.h"

using namespace DirectX;

namespace ClayEngine
{
	using SpriteBatchPtr = std::unique_ptr<SpriteBatch>;
	using SpriteBatchRaw = SpriteBatch*;

	/// <summary>
	/// API entry point for the graphical pipeline and GPU resources
	/// </summary>
	class RenderSystem
	{
		Affinity m_affinity;

		DX11ResourcesPtr m_resources = nullptr;

		SpriteBatchPtr m_spritebatch = nullptr;
		PrimitivePipelinePtr m_primitive = nullptr;

		bool m_device_lost = true;

	public:
		RenderSystem(Affinity affinityId);
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

	//TODO: More problematic Affinity extensions...
	class RenderSystemExtension
	{
	protected:
		RenderSystemRaw m_rs = nullptr;
	public:
		RenderSystemExtension()
		{
			m_rs = Services::GetService<RenderSystem>(std::this_thread::get_id());
		}
		~RenderSystemExtension()
		{
			m_rs = nullptr;
		}
	};

	class SpriteBatchExtension
	{
	protected:
		SpriteBatchRaw m_spritebatch = nullptr;
	public:
		SpriteBatchExtension()
		{
			m_spritebatch = Services::GetService<SpriteBatch>(std::this_thread::get_id());
		}
		~SpriteBatchExtension()
		{
			m_spritebatch = nullptr;
		}
	};
}
