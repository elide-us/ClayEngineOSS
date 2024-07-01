#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Bootstrap Class Library (C) 2022 Epoch Meridian, LLC.           */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include "ClayEngine.h"

#include <typeindex>

namespace ClayEngine
{
	namespace Platform
	{
		//struct WindowInterfaceExtension
		//{
		//	// OnSuspending // Power/Minimize messages
		//	// OnResuming
		//	// OnActivated // WM_ACTIVATEAPP
		//	// OnDeactivated
		//};
		//struct CoreInterfaceExtension
		//{
		//	void StartCore() {};
		//	void StopCore() {};
		//	void RestartCore() {};
		//	void ExitCore() {};
		//};
		//struct ConsoleInterfaceExtension
		//{
		//	// Callback for system message handler
		//};
		//class ServerWindowServerCore
		//	: public CoreInterfaceExtension
		//	, public WindowInterfaceExtension
		//{
		//public:
		//	// operator() for server entrypoint
		//};
		//class ClientWindowCore
		//	: public CoreInterfaceExtension
		//	, public WindowInterfaceExtension
		//{
		//public:
		//	// operator() for client entrypoint
		//};
		//class DebugConsoleCore
		//	: public CoreInterfaceExtension
		//	, public ConsoleInterfaceExtension
		//{
		//public:
		//	// operator() for console input handler loop
		//};
		//enum class WindowType
		//{
		//	Console, // Use for generic stdin, stdout, stderr
		//	ClientWindow, // Use for release single instance client
		//	DebugClientWindow, // Use for multi-client testing (limited sockets/memory)
		//	DebugClientConsole, // Use for interactive/network debugging client (no GUI)
		//	Server, // Use for release headless instance server (IOCP)
		//	ServerConsole, // Use for release interactive/debug console server (IOCP)
		//	ServerWindow, // Use for release single instance server (IOCP)
		//	DebugServerWindow, // Use for network debugging (NBIO)
		//	DebugServerConsole, // Use for interactive/network debugging server (NBIO)
		//};
		////enum class WindowMessageType{}; // Is it client/server/debug? Maybe not useful... idea.
		//class WindowClass
		//{
		//	WindowType m_window_type;
		//	std::wstring m_class_name; // Use class name to populate WCEX
		//	WNDCLASSEXW m_window_class; // Has icon and cursor refs and class name
		//	std::wstring m_window_name; // Used for the create window call
		//	HWND m_window_handle; // Return from create window with above class and window names
		//	bool m_core_assigned;
		//	void* m_core_pointer;
		//	std::vector<std::type_index> m_core_type;
		//public:
		//	WindowClass(WindowType type)
		//		: m_window_type(type)
		//	{
		//		// Type tells the WinderFactery what kinda winder ta make.
		//	}
		//	// std::function for callback (WNDCLASSEXW)
		//	// width, height
		//	// std::promise
		//};
		//
		//// type guiserver, client
		//// A window system (API) class
		//// Model this after the Services system
		//
		////TODO: After thinking this through...
		//// This is going to give me the pointer back to the specific ClayEngine WindowClass object associated to the HWND/ClassName
		//// To avoid passing the long ptr through the window, use WindowFactory::GetWindow(HWND) to return unique_ptr::pointer
		//
		////IDEA: Static class that is a "command sink", messages to this should have a structure around them with timestamp and other classification
		//// details like what system it's meant for, etc. There should be callback hooks for classes that wish to interpret messages sent to the sink.
		////IDEA: All messages will to to one static message handler function
		//// The function will either do nothing with the message, or will check if there's an object for that window
		//// and if so, will send that message to the appropriate window class object
		//class WindowCorePair
		//{
		//	WindowClass* m_wc;
		//	WindowInterfaceExtension* m_wi;
		//	CoreInterfaceExtension* m_ci;
		//};
		//LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		//class WindowFactory
		//{
		//	// HWND, Pointer to core
		//public:
		//	static HWND MakeWindow(HINSTANCE hInstance, int nShowCmd)
		//	{
		//		// Create ClayWindow
		//		// Register Class
		//		// Create Window, return HWND
		//	}
		//};
	}


	// The message sink should contain a "real" representation of the various pages that may be displayed
	// by the system. For example, the "debug" page would be written to stdout for the Console, but the
	// actual text that would be written there should be maintained here as a list of strings. We can later
	// get these strings and write them to the output when the output is initialized

	class MessageHandler
	{
		// T of my owner class
		// PTR to my owner class (vPtr?)

	public:
		MessageHandler()
		{

		}
		~MessageHandler()
		{

		}

		void HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
		{
			// Depending on T of owner class, handle different message scenarios
		}
	};
	using MessageHandlerPtr = std::unique_ptr<MessageHandler>;
	using MessageHandlerRaw = MessageHandler*;

	class WindowMessageRouter
	{
		using Window = HWND;
		using Handler = void*;
		using WindowHandlerMap = std::map<Window, Handler>;

		inline static WindowHandlerMap m_handlers = {};

	public:
		WindowMessageRouter()
		{
		}
		~WindowMessageRouter()
		{

		}

		template<typename T>
		static std::unique_ptr<T> MakeHandler(Window window, Handler handler)
		{
			// Another class will need to own these objects if I do it this way, probably not useful. Return void, just make a handler.
			// We then need the LRESULT CALLBACK to attach to the instantiation of this class (maybe use the lptr?) and pass message below

			return nullptr;
		}

		void HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			auto it = m_handlers.find(hWnd);
			if (it != m_handlers.end())
			{
				//it->second
			}
		}
	};

	class MessageSink
	{
		struct Message
		{
			// Timestamp 16B/4DW
			// Message
			// Length (num chars)
			// Size (num  bytes)
			void* Serialize()
			{
				// Turn the data into a btye[] (copy)

				return nullptr;
			}
		};

		enum class Subsystem
		{
			System,
			User,
			Network,
		};
		enum class Format
		{
			Ascii,
			Unicode,
		};

		// Collection of pages

	public:
		MessageSink()
		{

		}
		~MessageSink()
		{

		}

		// It does not matter where the messages go at this point, we just need a way to hook up to it, so this should be a registered Service
		// and provide a callback for outputs (later)

		//void PrintMessage(Message message)
		//void AttachDispatch(std:function callback)
		//void DispatchMessage(Message message)
	};
	using MessageSinkPtr = std::unique_ptr<MessageSink>;
	using MessageSinkRaw = MessageSink*;


	// Window system is fine, we need to bolt into it that we inject the raw pointer to our message handler, which should
	// understand the mapping between the HWND and the object that needs to handle _that_ window's message.

	class ClayEngineEntryPoint
	{
		HINSTANCE m_hInstance;
		int m_nShowCmd;

		ClayEngine::Platform::JsonFilePtr m_clayengine = {};
		ClayEngine::Platform::JsonFilePtr m_startup = {};

		// ClayEngine::Platform::MessageSink m_messagesink = {};

	public:
		ClayEngineEntryPoint(HINSTANCE hInstance, int nShowCmd)
		{
			m_hInstance = hInstance;
			m_nShowCmd = nShowCmd;

			if (FAILED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED))) throw std::exception("Core CRITICAL: Failed to Initialize COM");
			if (!DirectX::XMVerifyCPUSupport()) throw std::exception("Core CRITICAL: CPU Unsupported");

			// Intentionally hard-coded, this is your default startup file
			m_clayengine = std::make_unique<ClayEngine::Platform::JsonFile>("clayengine.json");

			//std::locale::global(std::locale(m_clayengine->GetValue<std::string>("locale")));

			// Inside the above file, define the startup key to script the startup process for your game
			m_startup = std::make_unique<ClayEngine::Platform::JsonFile>(m_clayengine->GetValue<std::string>("startup"));




			// EPIPHANY: Here we need to set up our message systems and other things. We've got our application running, 
			// we've initialized some APIs, we need to set up our message handling I/O system now, so we can route messages
			// from the OS and the debug window's stdio in and out of the same places, and eventually also tap this for
			// the GUI system console display as well.

			//std::for_each( , , []() {} );

			// After the message handling system is stood up, iterate the startup collection and wire up the windows and
			// I/O devices to their appropriate interfaces

			// "debug"

			// "client"

			// "guiserver"

			// "headless"
			
			// "etc..."


		}
		~ClayEngineEntryPoint()
		{
			m_startup.reset();
			m_clayengine.reset();

			CoUninitialize();
		}

		HINSTANCE GetInstance()
		{
			return m_hInstance;
		}
		int GetFlags()
		{
			return m_nShowCmd;
		}

		void Run()
		{
			while (true)
			{
				// Running...
			}
		}
	};
	using ClayEngineEntryPointPtr = std::unique_ptr<ClayEngineEntryPoint>;
	using ClayEngineEntryPointRaw = ClayEngineEntryPoint*;
}