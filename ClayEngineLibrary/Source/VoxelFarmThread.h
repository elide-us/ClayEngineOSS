#pragma once

#include "ClayEngine.h"

#include <memory>

namespace ClayEngine
{
	struct VoxelFarmThreadFunctor
	{
		void operator()(Future future);
	};

	class VoxelFarmThread
	{
		Thread m_thread;
		Promise m_promise = {};
	public:
		VoxelFarmThread();
		~VoxelFarmThread();
	};
	using VoxelFarmThreadPtr = std::unique_ptr<VoxelFarmThread>;
}
