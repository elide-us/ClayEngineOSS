#pragma once

#include "TimingSystem.h"
#include "ClayEngine.h"

namespace ClayEngine
{
	namespace SimpleMath
	{
		namespace Extensions
		{
			/// <summary>
			/// A class extension that provides a virtual Update function that is attached to the Update callback
			/// </summary>
			class UpdateExtension : public ClayEngine::TimingSystemExtension
				, public ClayEngine::IUpdate
			{
			public:
				UpdateExtension()
				{
					ticker->AddUpdateCallback([&](float elapsedTime) { Update(elapsedTime); });
				}
			};

			/// <summary>
			/// A class extension that provides a virtual Draw function that is attached to the Draw callback
			/// </summary>
			class DrawExtension : public ClayEngine::TimingSystemExtension
				, public ClayEngine::IDraw
			{
			public:
				DrawExtension()
				{
					ticker->AddDrawCallback([&]() { Draw(); });
				}
			};

			/// <summary>
			/// A class extension that provides a combination of the Update and Draw extensions
			/// </summary>
			class UpdateDrawExtension : public ClayEngine::TimingSystemExtension
				, public ClayEngine::IUpdate
				, public ClayEngine::IDraw
			{
			public:
				UpdateDrawExtension()
				{
					ticker->AddUpdateCallback([&](float elapsedTime) { Update(elapsedTime); });
					ticker->AddDrawCallback([&]() { Draw(); });
				}
			};

		}
	}
}