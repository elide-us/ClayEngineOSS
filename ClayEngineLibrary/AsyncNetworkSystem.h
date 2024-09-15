#pragma once

#include "Services.h"

#include <thread>
#include <future>
#include <mutex>
#include <chrono>
#include <vector>
#include <list>
#include <string>
#include <memory>

//namespace Experimental
//{
//	constexpr auto c_service_threads = 10UL; // 5x Send, 5x Recv
//	constexpr auto c_listen_threads = 4UL;
//	constexpr auto c_family = AF_INET;
//	constexpr auto c_socktype = SOCK_STREAM;
//	constexpr auto c_protocol = IPPROTO_TCP;
//
//	enum class OPERATION;
//	#define OP_WSA_ACCEPT 1
//	#define OP_WSA_SEND 2
//	#define OP_WSA_RECV 3
//	#define OP_FILE_READ 4
//	#define OP_FILE_WRITE 5
//	#define OP_TEST_DEBUG 6
//	#define OP_THREAD_EXIT 7
//
//	using THREAD = std::thread;
//	using PROMISE = std::promise<void>;
//	using FUTURE = std::future<void>;
//	using MUTEX = std::mutex;
//
//	using LockGuard = std::lock_guard<MUTEX>;
//	using Nanoseconds = std::chrono::milliseconds;
//	using Unicode = std::wstring;
//	using String = std::string;
//
//	// A worker thread object holding the promise that is used to shut down the thread's loop
//	struct WorkerThread;
//	using WorkerThreads = std::vector<WorkerThread>;
//
//	// Per operation data, primarily used for WSASend/WSARecv
//	struct OperationData;
//	using OperationsData = std::vector<OperationData*>;
//
//	struct AsyncServerSocket;
//	using AsyncServerSocketPtr = std::unique_ptr<AsyncServerSocket>;
//	using AsyncServerSockets = std::vector<AsyncServerSocketPtr>;
//
//	struct ClientSocket;
//	using ClientSocketPtr = std::unique_ptr<ClientSocket>;
//	using ClientSockets = std::vector<ClientSocketPtr>;
//
//	// This functor responds to the AcceptEx messages
//	// Should assign the completionKey return value to the ListenerAffinity value of the ClientSocket object
//	struct AsyncListenWorker
//	{
//		void operator()(HANDLE hCompletionPort, FUTURE future);
//	};
//
//	// This functor responds to the WSASend/WSARecv messages
//	struct AsyncNetworkWorker
//	{
//		void operator()(HANDLE hCompletionPort, FUTURE future);
//	};
//
//	// Uses Non-blocking I/O loop to accept client sockets
//	struct NbioListenWorker
//	{
//		void operator()(FUTURE future);
//	private:
//		std::tuple<bool, SOCKET, SOCKADDR> checkAcceptForClient(SOCKET s);
//	};
//
//	class NbioListenContext
//	{
//		THREAD m_thread;
//		PROMISE m_promise;
//
//	public:
//		NbioListenContext();
//		~NbioListenContext();
//	};
//	using NbioListenContextPtr = std::unique_ptr<NbioListenContext>;
//
//	// Uses worker threads to complete AcceptEx
//	class AsyncListenServerModule
//	{
//		// Listen Socket
//		AsyncServerSocketPtr m_listen_socket = nullptr;
//
//		// Worker Threads
//		size_t m_worker_count = 4;
//		WorkerThreads m_workers = {};
//	public:
//		AsyncListenServerModule(const Unicode address, const Unicode port);
//		~AsyncListenServerModule();
//
//		void Start();
//		void Stop();
//	};
//	using AsyncListenServerModulePtr = std::unique_ptr<AsyncListenServerModule>;
//
//	// Uses non-blocking I/O to complete accept
//	class NbioListenServerModule
//	{
//		class AcceptThreadContext
//		{
//			std::thread m_thread;
//			std::promise<void> m_promise = {};
//		};
//		using AcceptThreadContextPtr = std::unique_ptr<AcceptThreadContext>;
//
//		struct AcceptThreadFunctor
//		{
//			void operator()(FUTURE future)
//			{
//				while (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout)
//				{
//					//checkAcceptForClient(ClientSocket ...);
//				}
//			}
//		private:
//			std::tuple<bool, SOCKET, SOCKADDR> checkAcceptForClient(SOCKET s);
//		};
//
//		void startListenServer();
//		void stopListenServer();
//	public:
//		NbioListenServerModule();
//		~NbioListenServerModule();
//	};
//	using NbioListenServerModulePtr = std::unique_ptr<NbioListenServerModule>;
//
//	// Uses non-blocking I/O to connect to a server
//	class ClientConnectModule
//	{
//		SOCKET m_socket = {};
//		SOCKADDR_IN m_sockaddr_in = {};
//
//		bool tryConnectToServer();
//
//	public:
//		ClientConnectModule(const Unicode address);
//		~ClientConnectModule();
//	};
//	using ClientConnectModulePtr = std::unique_ptr<ClientConnectModule>;
//
//	// Uses worker threads to complete WSARecv/WSASend
//	class AsyncServerModule;
//	using AsyncServerModulePtr = std::unique_ptr<AsyncServerModule>;
//
//	// This experimental class is the top level API for the new
//	// AsyncNetworkSystem. We will need to modularize the pieces 
//	// so we can instantitate client/server/etc in the same way 
//	// as the older NetworkSystem NBIO implementation. The Listener
//	// will not be in its own thread, instead we will have dedicated 
//	// workers just for the listener completion port. Note that 
//	// there can be multiple listeners on different ports. We 
//	// should be able to instatitate multiple modules as required.
//	class AsyncNetworkSystem
//	{
//		#pragma region Async Client Server Setup
//		// Completion Port for WSARecv/WSASend
//		HANDLE m_queue = nullptr;
//
//		// Client Sockets
//
//		// AsyncListenServerModule
//		// - Has-a Completion Port
//		// - Has-a Listen Socket
//		// - Has-a lpfnAcceptEx
//		// NbioListenServerModule
//		// - Has-a ListenServerContext
//		// - Has-a ListenServerFunctor
//		// ClientConnectModule - For the Client to connect to Server
//
//		// AcceptEx workers populate this collection
//		ClientSockets m_clients = {};
//		MUTEX m_clients_mutex = {};
//		//void removeClientSocket(SOCKET socket);
//
//		// Operation Data
//		OperationsData m_operations = {};
//		MUTEX m_operations_mutex = {};
//		OperationData* createOperationData(OPERATION operation, DWORD bufferSize);
//		void deleteOperationData(OperationData* operationData);
//
//		// Worker Threads
//		size_t m_worker_count = 10;
//		WorkerThreads m_workers = {};
//		void startWorkerThreads();
//		void stopWorkerThreads();
//		#pragma endregion
//
//		#pragma region Async Listen Server Setup
//		AsyncListenServerModulePtr m_listen_server = nullptr;
//
//		// Above should replace most of below...
//		Unicode m_address = {};
//		Unicode m_port = {};
//		ADDRINFOW* m_addrinfo = {};
//		
//		LPFN_ACCEPTEX lpfnAcceptEx = nullptr;
//
//		SOCKET m_listen_socket = {}; //v1
//		HANDLE m_listen_queue = {}; //v1
//		AsyncServerSocketPtr m_listener = nullptr; //v2
//		
//		AsyncServerSockets m_listen_sockets = {}; //v3
//		//void addListenSocket(SOCKET socket, HANDLE completionPort);
//		//void removeListenSocket(SOCKET socket);
//
//		size_t m_listen_worker_count = 4;
//		WorkerThreads m_listen_workers = {};
//		void startListenServer(); // To be called in the constructor
//		//void startListenThreads();
//		//void stopListenThreads();
//		#pragma endregion
//
//		AsyncListenServerModulePtr m_async = nullptr;
//		NbioListenServerModulePtr m_nbio = nullptr;
//
//		ClientConnectModulePtr m_client = nullptr;
//
//		AsyncServerModulePtr m_server = nullptr;
//
//	public:
//		AsyncNetworkSystem(const Unicode address, const Unicode port);
//		~AsyncNetworkSystem();
//
//		void PostAccept();
//		void PostTest();
//		void PostExit();
//		void PostRecv();
//		void PostSend();
//
//		int ProcessWSALastError();
//
//		int WriteResolvedAddress(SOCKADDR* sa, int sa_length);
//		ADDRINFO* ResolveAddress(String address, String port, int family, int socktype, int protocol);
//		size_t MeasureAddressLength(ADDRINFO* address);
//		void FreeAddress(ADDRINFO* ai);
//		void FreeSocket(SOCKET s);
//
//		void AddClientSocket(SOCKET socket);
//
//	};
//}

#define ClayMemZero(Address, Length) memset(Address, 0, Length)

namespace ClayEngine
{
	//using THREAD = std::thread;
	//using FUTURE = std::future<void>;
	//using PROMISE = std::promise<void>;
	
	class AsyncNetworkSystem;
	
	struct AsyncListenFunctor
	{
		struct AcceptSocketData
		{
			OVERLAPPED Overlapped;
			SOCKET Socket = INVALID_SOCKET;
			CHAR* Buffer = NULL;
			DWORD BufferLength = 0;

			AcceptSocketData(DWORD bufferLength) : BufferLength(bufferLength)
			{
				ClayMemZero(&Overlapped, sizeof(OVERLAPPED));
				Socket = INVALID_SOCKET;
				Buffer = new CHAR[BufferLength];
			}
			~AcceptSocketData()
			{
				delete[] Buffer;
			}

			void Reset()
			{
				ClayMemZero(&Overlapped, sizeof(OVERLAPPED));
				Socket = INVALID_SOCKET;
				ClayMemZero(&Buffer, BufferLength);
			}
		};
		using AcceptSocketDataPtr = std::unique_ptr<AcceptSocketData>;
		using AcceptSocketsData = std::list<AcceptSocketDataPtr>;
	
		void operator()(FUTURE future, AsyncNetworkSystem* ans);
	};

	class AsyncNetworkSystem
	{
	public:
		struct ClientConnectionData
		{
			SOCKET Socket;
			SOCKADDR SockAddr = {};
			SOCKADDR_IN SockAddrIn = {};

			ClientConnectionData(SOCKET socket)
				: Socket(socket)
			{
				auto sz = static_cast<int>(sizeof(SOCKADDR));
				getpeername(Socket, &SockAddr, &sz);
				memcpy(&SockAddrIn, &SockAddr, sizeof(SOCKADDR_IN));
			}
			~ClientConnectionData()
			{
				if (Socket) closesocket(Socket);
			}
		};
		using ClientConnectionDataPtr = std::unique_ptr<ClientConnectionData>;
		using ClientConnectionsData = std::vector<ClientConnectionDataPtr>;

	private:
		AffinityData m_affinity = {};

		WSADATA m_wsaData = {};

		HANDLE m_listen_queue = 0;
		SOCKET m_listen_socket = INVALID_SOCKET;
		ADDRINFOW* m_listen_addrinfo = nullptr;;

		THREAD m_listen_thread;
		PROMISE m_listen_promise = {};

		LPFN_ACCEPTEX m_fnAcceptEx = nullptr;

		AsyncListenFunctor::AcceptSocketsData m_accept_sockets = {};
		ClientConnectionsData m_client_connections = {};

	public:
		AsyncNetworkSystem(AffinityData affinityData)
			: m_affinity(affinityData)
		{
			WSAStartup(MAKEWORD(2, 2), &m_wsaData);

			// Create Completion Port
			m_listen_queue = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);

			// Create Listen Socket
			ADDRINFOW _hints = {};
			_hints.ai_flags = AI_PASSIVE;
			_hints.ai_family = AF_INET;
			_hints.ai_socktype = SOCK_STREAM;
			_hints.ai_protocol = IPPROTO_TCP;

			std::wstring _address = L"127.0.0.1";
			std::wstring _port = L"19740";

			if (FAILED(GetAddrInfoW(_address.c_str(), _port.c_str(), &_hints, &m_listen_addrinfo)))
			{
				WSACleanup();
				throw std::runtime_error("hResult::FAILED GetAddrInfoW()");
			}
			
			m_listen_socket = WSASocketW(m_listen_addrinfo->ai_family, m_listen_addrinfo->ai_socktype, m_listen_addrinfo->ai_protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (m_listen_socket == INVALID_SOCKET)
			{
				FreeAddrInfoW(m_listen_addrinfo);
				WSACleanup();
				throw std::runtime_error("hResult::FAILED WSASocketW()");
			}

			DWORD dwBytes = 0;
			GUID guidAcceptEx = WSAID_ACCEPTEX;
			auto rc = WSAIoctl(m_listen_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
				&guidAcceptEx, sizeof(guidAcceptEx), 
				&m_fnAcceptEx, sizeof(m_fnAcceptEx), 
				&dwBytes, NULL, NULL);
			if (rc == SOCKET_ERROR)
			{
				closesocket(m_listen_socket);
				FreeAddrInfoW(m_listen_addrinfo);
				WSACleanup();
				throw std::runtime_error("hResult::FAILED WSAIoctl()");
			}

			rc = bind(m_listen_socket, m_listen_addrinfo->ai_addr, static_cast<int>(m_listen_addrinfo->ai_addrlen));
			if (rc == SOCKET_ERROR)
			{
				closesocket(m_listen_socket);
				FreeAddrInfoW(m_listen_addrinfo);
				WSACleanup();
				throw std::runtime_error("hResult::FAILED bind()");
			}

			rc = listen(m_listen_socket, SOMAXCONN);
			if (rc == SOCKET_ERROR)
			{
				closesocket(m_listen_socket);
				FreeAddrInfoW(m_listen_addrinfo);
				WSACleanup();
				throw std::runtime_error("hResult::FAILED listen()");
			}

			// Assign Socket to Completion Port
			if (!CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listen_socket), m_listen_queue, NULL, 1))
			{
				auto ec = GetLastError();

				closesocket(m_listen_socket);
				FreeAddrInfoW(m_listen_addrinfo);
				WSACleanup();
				throw std::runtime_error("hResult::FAILED CreateIoCompletionPort()");
			}

			// Create Worker Thread
			m_listen_thread = THREAD(AsyncListenFunctor(), m_listen_promise.get_future(), this);
		}
		~AsyncNetworkSystem()
		{
			m_listen_promise.set_value();
			if (m_listen_thread.joinable()) m_listen_thread.join();
			if (m_listen_socket) closesocket(m_listen_socket);
			if (m_listen_addrinfo) FreeAddrInfoW(m_listen_addrinfo);
			if (m_listen_queue) CloseHandle(m_listen_queue);
			WSACleanup();
		}

		const LPFN_ACCEPTEX GetAcceptExFunction() const
		{
			return m_fnAcceptEx;
		}

		AsyncListenFunctor::AcceptSocketData* MakeAcceptSocketData(DWORD bufferLength)
		{
			m_accept_sockets.emplace_back(std::make_unique<AsyncListenFunctor::AcceptSocketData>(bufferLength));
			return m_accept_sockets.back().get();
		}
		void FreeAcceptSocketData(SOCKET socket)
		{
  			m_accept_sockets.erase(std::remove_if(m_accept_sockets.begin(), m_accept_sockets.end(), [socket](const AsyncListenFunctor::AcceptSocketDataPtr& ptr) { return ptr->Socket == socket; }), m_accept_sockets.end());
		}

		HANDLE GetListenQueue()
		{
			return m_listen_queue;
		}
		SOCKET GetListenSocket()
		{
			return m_listen_socket;
		}

		ClientConnectionData* MakeClientConnectionData(SOCKET socket)
		{
			m_client_connections.emplace_back(std::make_unique<ClientConnectionData>(socket));
			return m_client_connections.back().get();
		}
	};
	using AsyncNetworkSystemPtr = std::unique_ptr<AsyncNetworkSystem>;

	class AsyncNetworkTestClient
	{
		WSADATA wsaData;
		SOCKET m_socket = INVALID_SOCKET;

		addrinfo* m_pAddrinfo = nullptr;
		addrinfo hints = {};

	public:
		AsyncNetworkTestClient()
		{
			addrinfo* ptr = nullptr;

			const char* sendbuf = "Hello, Server!";

			char recvbuf[512];
			int recvbuflen = 512;

			if (FAILED(WSAStartup(MAKEWORD(2, 2), &wsaData))) throw;

			hints.ai_flags = AI_PASSIVE;
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;

			// Resolve the server address and port
			if (FAILED(getaddrinfo("127.0.0.1", "19740", &hints, &m_pAddrinfo)))
			{
				WSACleanup();
				throw;
			}

			// Attempt to connect to an address until one succeeds
			for (ptr = m_pAddrinfo; ptr != nullptr; ptr = ptr->ai_next)
			{
				// Create a SOCKET for connecting to server
				m_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
				if (m_socket == INVALID_SOCKET) {
					std::cerr << "socket failed with error: " << WSAGetLastError() << std::endl;
					WSACleanup();
					throw;
				}

				// Connect to server.
				auto rc = connect(m_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
				if (rc == SOCKET_ERROR)
				{
					closesocket(m_socket);
					m_socket = INVALID_SOCKET;
					continue;
				}

				break;
			}

			freeaddrinfo(m_pAddrinfo);

			if (m_socket == INVALID_SOCKET) {
				std::cerr << "Unable to connect to server!" << std::endl;
				WSACleanup();
				throw;
			}

			// Send an initial buffer
			auto iResult = send(m_socket, sendbuf, (int)strlen(sendbuf), 0);
			if (iResult == SOCKET_ERROR) {
				std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
				closesocket(m_socket);
				WSACleanup();
				throw;
			}

			std::cout << "Bytes Sent: " << iResult << std::endl;

			// Receive data until the server closes the connection
			do {
				iResult = recv(m_socket, recvbuf, recvbuflen, 0);
				if (iResult > 0)
					std::cout << "Bytes received: " << iResult << std::endl;
				else if (iResult == 0)
					std::cout << "Connection closed" << std::endl;
				else
					std::cerr << "recv failed with error: " << WSAGetLastError() << std::endl;

			} while (iResult > 0);

		}
		~AsyncNetworkTestClient()
		{
			closesocket(m_socket);
			WSACleanup();

		}
	};

}
