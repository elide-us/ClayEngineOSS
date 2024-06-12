#include "pch.h"

#include "ClayEngine.h"
#include "ClientCore.h"

using namespace ClayEngine;
using namespace ClayEngine::Platform;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	PlatformStart();

	//m_clayengine = std::make_unique<ClayEngine::Platform::JsonFile>("clayengine.json");
	//std::locale::global(std::locale(m_clayengine->GetValue<std::string>("locale")));
	// Inside the above file, define the startup key to script the startup process for your game
	//m_startup = std::make_unique<ClayEngine::Platform::JsonFile>(m_clayengine->GetValue<std::string>("startup"));

	auto client01 = std::make_unique<ClayEngineClientCore>(hInstance, nCmdShow, L"client01", L"ClayEngine Test Client 01");
	auto client02 = std::make_unique<ClayEngineClientCore>(hInstance, nCmdShow, L"client02", L"ClayEngine Test Client 02");

	client01->Run();

	client02.reset();
    client01.reset();

	PlatformStop();

    return 0;
}
