/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* Basic engine server demonstrating entry point setup                        */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include "pch.h"
#include "ClayEngine.h"

namespace
{
	ClayEngine::ClayHeadlessPtr g_bootstrap = nullptr;
}

int wmain(int argc, wchar_t* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	g_bootstrap = std::make_unique<ClayEngine::ClayHeadless>();
	g_bootstrap->Run();
	g_bootstrap.reset();

	return 0;
}
