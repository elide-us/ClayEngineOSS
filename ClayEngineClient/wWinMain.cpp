/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* Basic engine client demonstrating entry point setup                        */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include "pch.h"
#include "ClayEngine.h"

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace
{
	ClayEngine::ClayEnginePtr g_bootstrap = nullptr;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	g_bootstrap = std::make_unique<ClayEngine::ClayEngine>(hInstance, lpCmdLine,
		(UINT)nCmdShow, std::locale::global(std::locale("")));
	g_bootstrap->Run();
	g_bootstrap.reset();

	return 0;
}

// Random thoughts
// Octree, should we go with a traditional implementation?
// Octree will have 16 LOD (four bits 0 - 15)
// Smallest LOD would represent a voxel cluster
// Each voxel cluster should be 256x256x256
// LOD should be calculated in a fully 3D radial manner
// The Octree should be considered fully 3D
// LOD_0 will always be rendered.
// Cells will be subdivided if they are within a certain radius
// Cells within those subdivisions will be further subdivided
//   regressivly until we are within a certain radius of the player in the world
// Each voxel should be composed of one vector and callback slots for the
//   remaining vectors for drawing a cube in standard clockwise winding left hand orientation

