#pragma once

#include "Services.h"

//	constexpr auto c_service_threads = 10UL; // 5x Send, 5x Recv

//	#define OP_WSA_ACCEPT 1
//	#define OP_WSA_SEND 2
//	#define OP_WSA_RECV 3
//	#define OP_FILE_READ 4
//	#define OP_FILE_WRITE 5
//	#define OP_TEST_DEBUG 6
//	#define OP_THREAD_EXIT 7

//	using Milliseconds = std::chrono::milliseconds;


namespace ClayEngine
{
	#pragma region Async Listen Server Module Declarations
	// Defaults for Listen Server Socket and Worker Threads
	constexpr auto c_listen_workers = 4UL;
	constexpr auto c_listen_backlog = SOMAXCONN;
	constexpr auto c_listen_hint_flags = AI_PASSIVE;
	constexpr auto c_listen_hint_family = AF_INET;
	constexpr auto c_listen_hint_socktype = SOCK_STREAM;
	constexpr auto c_listen_hint_protocol = IPPROTO_TCP;
	constexpr auto c_listen_server_address = L"127.0.0.1";
	constexpr auto c_listen_server_port = L"19740";

	constexpr DWORD c_dwReceiveDataLength = 64;
	constexpr DWORD c_dwLocalAddressLength = sizeof(SOCKADDR_IN) + 16;
	constexpr DWORD c_dwRemoteAddressLength = sizeof(SOCKADDR_IN) + 16;
	
	class AsyncNetworkSystem;
	class AsyncListenServerModule;

	struct AsyncListenServerWorker;
	using AsyncListenServerWorkers = std::vector<AsyncListenServerWorker>;
	
	struct AsyncListenServerFunctor
	{
		void operator()(FUTURE future, AsyncListenServerModule* context);
	};

	class AsyncListenServerModule
	{
	public:
		struct AcceptSocketData;
		using AcceptSocketDataPtr = std::unique_ptr<AcceptSocketData>;
		using AcceptSocketsData = std::list<AcceptSocketDataPtr>;

	private:
		HANDLE m_listen_queue = INVALID_HANDLE_VALUE;
		SOCKET m_listen_socket = INVALID_SOCKET;
		ADDRINFOW* m_listen_addrinfo = nullptr;

		AsyncListenServerWorkers m_listen_threads = {};

		LPFN_ACCEPTEX m_fnAcceptEx = nullptr;
		LPFN_GETACCEPTEXSOCKADDRS m_fnGetAcceptExSockAddrs = nullptr;

		MUTEX m_accept_sockets_mutex = {};
		AcceptSocketsData m_accept_sockets = {};

		AsyncNetworkSystem* m_ans = nullptr;

		HANDLE createCompletionPort(DWORD listenWorkers);
		SOCKET createListenSocket(int flags, int family, int socktype, int protocol, Unicode address, Unicode port);

		void getWSAExtensionFunctions();
		void bindListenSocket();

	public:
		AsyncListenServerModule(AsyncNetworkSystem* ans, DWORD listenWorkers = c_listen_workers, int flags = c_listen_hint_flags, int family = c_listen_hint_family, int socktype = c_listen_hint_socktype, int protocol = c_listen_hint_protocol, Unicode address = c_listen_server_address, Unicode port = c_listen_server_port);
		~AsyncListenServerModule();

		AcceptSocketData* MakeAcceptSocketData(DWORD bufferLength);
		void FreeAcceptSocketData(AcceptSocketData* acceptSocketData);

		const HANDLE GetListenQueue() { return m_listen_queue; }
		const SOCKET GetListenSocket() { return m_listen_socket; }

		const LPFN_ACCEPTEX GetAcceptExFunction() const { return m_fnAcceptEx; }
		const LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockAddrsFunction() const { return m_fnGetAcceptExSockAddrs; }

		void MakeClientConnectionData(SOCKET socket, SOCKADDR* local, SOCKADDR_IN* remote);
	};
	using AsyncListenServerModulePtr = std::unique_ptr<AsyncListenServerModule>;
	#pragma endregion

	class AsyncNetworkSystem
	{
	public:
		struct ClientConnectionData;
		using ClientConnectionDataPtr = std::unique_ptr<ClientConnectionData>;
		using ClientConnectionsData = std::vector<ClientConnectionDataPtr>;

	private:
		AffinityData m_affinity_data = {};

		WSADATA m_wsaData = {};

		AsyncListenServerModulePtr m_listen_server = nullptr;

		//AsyncDataTransferModulePtr m_data_transfer = nullptr;

		MUTEX m_client_connections_mutex = {};
		ClientConnectionsData m_client_connections = {};

	public:
		AsyncNetworkSystem(AffinityData affinityData);
		~AsyncNetworkSystem();

		void MakeClientConnectionData(SOCKET socket, SOCKADDR* local, SOCKADDR_IN* remote);

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

			//char recvbuf[512];
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
			if (iResult == SOCKET_ERROR)
			{
				std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
				closesocket(m_socket);
				WSACleanup();
				throw;
			}

			std::cout << "Bytes Sent: " << iResult << std::endl;
		}
		~AsyncNetworkTestClient()
		{
			closesocket(m_socket);
			WSACleanup();
		}
	};
}
