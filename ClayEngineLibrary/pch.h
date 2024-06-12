#pragma once
//#pragma warning(push)
//#pragma warning(disable : )
// 4265 4471 4571 4616 4619 4628 4643
// 4710 4711 4746 4774 4917 4986 4987
// 5029 5031 5032 5043 5219 26812
//#pragma warning(pop)

// nlohmann::json
#pragma warning(disable : 4061 4100 4189 4245 4265 4355 4365 4623 4625 4626 4668 4820 5026 5027 5039 5045 5204 5219 5220 5246)

#include <WinSDKVer.h>
#include <SDKDDKVer.h>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#ifndef _WIN32_WINNT_WIN10
#define _WIN32_WINNT_WIN10 0x0A00
#endif

#ifndef WINAPI_FAMILY_GAMES
#define WINAPI_FAMILY_GAMES 6
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX // Include <mcx.h> if you need this
#define NOSERVICE // Include <winsvc.h> if you need this
#define NOHELP

#include <Windows.h>

//#include <wrl.h>
#include <wrl/client.h>

#include <wincodec.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>

#define _XM_NO_XMVECTOR_OVERLOADS_
#include <d3d11_1.h>
#include <dxgi1_2.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
//#include <DirectXHelpers.h>

#include <cmath>
//#include <cstdio>
#include <cstdint>
#include <cstddef>
//#include <cfloat>

#include <memory>
#include <exception>
#include <stdexcept>

#include <typeinfo>
#include <typeindex>
#include <type_traits>

#include <functional>

#include <algorithm>
#include <utility>
#include <chrono>
#include <thread>
#include <mutex>

#include <future>

#include <vector>
#include <tuple>
#include <array>
#include <list>
#include <map>
#include <set>

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <fstream>
