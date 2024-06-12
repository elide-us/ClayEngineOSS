#pragma once

//#pragma warning(disable : 4619 4616 4061 4265 4365 4571 4623 4625 4626 4628 4668 4710 4711 4746 4774 4820 4987 5026 5027 5031 5032 5039 5045 5219 26812)
//#pragma warning(disable : 4471 4917 4986 5029)
//#pragma warning(disable : 4643 5043)
#pragma warning(disable : 5246 28020)

// Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

// nlohmann::json
#pragma warning(disable : 4061 4623 4668 5045)

//#include <WinSDKVer.h>
//#define _WIN32_WINNT 0x0A00
#include <SDKDDKVer.h>

#ifndef _WIN32_WINNT_WIN10
#define _WIN32_WINNT_WIN10 0x0A00
#endif

#ifndef WINAPI_FAMILY_GAMES
#define WINAPI_FAMILY_GAMES 6
#endif

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
//#pragma warning(push)
//#pragma warning(disable : 4005)
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX // Include <mcx.h> if you need this
#define NOSERVICE // Include <winsvc.h> if you need this
#define NOHELP
//#pragma warning(pop)
#include <Windows.h>

//#pragma warning(push)
//#pragma warning(disable : 4467 5038 5204 5220)
//#include <wrl.h>
#include <wrl/client.h>
//#pragma warning(pop)

#include <wincodec.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>

#define _XM_NO_XMVECTOR_OVERLOADS_
#include <d3d11_1.h>
#include <dxgi1_2.h>

//#define DIRECTINPUT_VERSION 0x0800
//#include <dinput.h>

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

//#include <malloc.h>
//#include <stdlib.h>
//#include <memory.h>
//#include <tchar.h>
//#include <float.h>

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

//#pragma warning(push)
//#pragma warning(disable : 4702)
#include <functional>
//#pragma warning(pop)

#include <algorithm>
#include <utility>
#include <chrono>
#include <thread>
#include <future>
#include <mutex>

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

//#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
//#include <codecvt>

//namespace DX
//{
//    inline void ThrowIfFailed(HRESULT hr)
//    {
//        if (FAILED(hr))
//        {
//            // Set a breakpoint on this line to catch DirectX API errors
//            throw std::exception();
//        }
//    }
//    inline void ThrowIfFailed(HRESULT hr, std::string reason)
//    {
//        if (FAILED(hr))
//        {
//            // Set a breakpoint on this line to catch DirectX API errors
//            throw std::exception(reason.c_str());
//        }
//    }
//}