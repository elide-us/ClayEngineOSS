/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* Basic engine client demonstrating entry point setup                        */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include "pch.h"
#include "ClayEngine.h"

namespace
{
	ClayEngine::ClayEnginePtr g_bootstrap = nullptr;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    g_bootstrap = std::make_unique<ClayEngine::ClayEngine>(hInstance, lpCmdLine, (UINT)nCmdShow, std::locale::global(std::locale("")));
	g_bootstrap->Run();
	g_bootstrap.reset();

    return 0;
}
