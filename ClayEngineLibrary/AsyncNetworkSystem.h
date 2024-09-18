#pragma once

#include "Strings.h"
#include "Storage.h"
#include "Services.h"

namespace ClayEngine
{
	int ProcessWSALastError();

	// Forward declaration for Modules
	class AsyncNetworkSystem;

	#pragma region Async Listen Server Module Declarations
	// Defaults for Listen Server Socket and Worker Threads
	constexpr auto c_listen_workers = 4UL;
	constexpr auto c_listen_backlog = SOMAXCONN;

	constexpr int c_listen_hint_flags = AI_PASSIVE;
	constexpr int c_listen_hint_family = AF_INET;
	constexpr int c_listen_hint_socktype = SOCK_STREAM;
	constexpr int c_listen_hint_protocol = IPPROTO_TCP;

	constexpr auto c_listen_server_address = L"127.0.0.1";
	constexpr auto c_listen_server_port = L"19740";

	constexpr DWORD c_listen_recvdata_length = sizeof(GUID); // We get a GUID from the client
	constexpr DWORD c_listen_local_addr_length = sizeof(SOCKADDR_IN) + 16;
	constexpr DWORD c_listen_remote_addr_length = sizeof(SOCKADDR_IN) + 16;

	struct AsyncListenServerWorker;
	using AsyncListenServerWorkers = std::vector<AsyncListenServerWorker>;
	
	class AsyncListenServerModule;
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
		SOCKET createListenSocket(Unicode address, Unicode port, int flags, int family, int socktype, int protocol);

		void getWSAExtensionFunctions();
		void bindListenSocket();

	public:
		AsyncListenServerModule(AsyncNetworkSystem* ans, DWORD listenWorkers = c_listen_workers, Unicode address = c_listen_server_address, Unicode port = c_listen_server_port);
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

	#pragma region Client Connection Module Declarations
	constexpr auto c_connect_family = AF_INET;
	constexpr auto c_connect_socktype = SOCK_STREAM;
	constexpr auto c_connect_protocol = IPPROTO_TCP;

	struct ConnectClientWorker;
	using ConnectClientWorkers = std::vector<ConnectClientWorker>;

	class ConnectClientModule;
	struct ConnectClientFunctor
	{
		void operator()(FUTURE future, ConnectClientModule* context);
	};

	class ConnectClientModule
	{
		AsyncNetworkSystem* m_ans = nullptr;

		SOCKET m_connect_socket = INVALID_SOCKET;
		SOCKADDR_IN m_connect_sockinfo = {};

		bool m_tryToConnect = true;
		Unicode m_connect_address = {};
		Unicode m_connect_port = {};
		
		SOCKET createConnectSocket(Unicode address, Unicode port, int family = c_connect_family, int socktype = c_connect_socktype, int protocol = c_connect_protocol);

	public:
		ConnectClientModule(AsyncNetworkSystem* ans, Unicode address, Unicode port);
		~ConnectClientModule();

		void ConnectToServer();
		void DisconnectFromServer();
	};
	using ConnectClientModulePtr = std::unique_ptr<ConnectClientModule>;
	#pragma endregion

	class AsyncNetworkSystem
	{
	public:
		struct ClientConnectionData;
		using ClientConnectionDataPtr = std::unique_ptr<ClientConnectionData>;
		using ClientConnectionsData = std::vector<ClientConnectionDataPtr>;

	private:
		AffinityData m_affinity_data = {};
		Unicode m_class_name = {};
		Document m_document = {};

		WSADATA m_wsaData = {};

		// Instantiated for Server and Headless configuration
		AsyncListenServerModulePtr m_listen_server = nullptr;
		//AsyncDataTransferModulePtr m_data_transfer = nullptr;

		// Used by Server and Headless to manage client socket lifetime
		MUTEX m_client_connections_mutex = {};
		ClientConnectionsData m_client_connections = {};

		// Instantiated for Client configuration
		ConnectClientModulePtr m_connect_client = nullptr;

	public:
		AsyncNetworkSystem(AffinityData affinityData, Unicode className, Document document);
		~AsyncNetworkSystem();

		void MakeClientConnectionData(SOCKET socket, SOCKADDR* local, SOCKADDR_IN* remote);

	};
	using AsyncNetworkSystemPtr = std::unique_ptr<AsyncNetworkSystem>;
}
