#pragma once

#include "Strings.h"
#include "Storage.h"
#include "Services.h"

namespace ClayEngine
{
	int ProcessWSALastError();

	// Forward declaration for Modules
	class AsyncNetworkSystem;

	constexpr auto c_ans_control_workers = 4UL; // TCP, UNSPEC, 19740
	constexpr auto c_ans_chat_workers = 8UL; // TCP, UNSPEC, 512KiB 19741/51/61/71/etc...
	constexpr auto c_ans_bulk_workers = 8UL; // TCP, UNSPEC, 4096KiB, 19742/52/62/72/etc...
	constexpr auto c_ans_vector_workers = 16UL; // UDP, UNSPEC, 64KiB, 19743/53/63/73/etc...

	constexpr auto c_ans_socket_backlog = SOMAXCONN;
	constexpr auto c_ans_hints_flags = AI_PASSIVE;
	constexpr auto c_ans_hints_family = AF_UNSPEC;
	constexpr auto c_ans_hints_tcp_socktype = SOCK_STREAM;
	constexpr auto c_ans_hints_tcp_protocol = IPPROTO_TCP;
	constexpr auto c_ans_hints_udp_socktype = SOCK_DGRAM;
	constexpr auto c_ans_hints_udp_protocol = IPPROTO_UDP;

	constexpr auto c_ans_server_address_v4 = L"127.0.0.1";
	constexpr auto c_ans_server_address_v6 = L"::1";
	constexpr auto c_ans_control_port = L"19740";

	constexpr DWORD c_guid_length = sizeof(GUID);
	constexpr DWORD c_address_length = sizeof(SOCKADDR_IN) + 16;


	/// <summary>
	/// 
	/// </summary>
	/// <typeparam name="T">context</typeparam>
	/// <typeparam name="U">functor</typeparam>
	template<class T, class U>
	class AsyncServerSocketModule
	{
		// Port
		// Listen Socket
		// Client Sockets
		// Worker Threads

	public:
		AsyncServerSocketModule()
		{

		}
		virtual ~AsyncServerSocketModule() = default;


	};




	#pragma region Async Listen Server Module Declarations
	// Defaults for Listen Server Socket and Worker Threads
	constexpr auto c_listen_workers = 4UL;
	constexpr auto c_listen_backlog = SOMAXCONN;
	constexpr int c_listen_hint_flags = AI_PASSIVE;
	constexpr int c_listen_hint_family = AF_UNSPEC;
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

	#pragma region Async Bulk Data Server Module Declarations
	constexpr auto c_data_workers = 16UL;

	constexpr auto c_data_server_address = L"127.0.0.1";
	constexpr auto c_data_server_port = L"19741";

	struct AsyncBulkDataServerWorker;
	using AsyncBulkDataServerWorkers = std::vector<AsyncBulkDataServerWorker>;

	class AsyncBulkDataServerModule;
	struct AsyncBulkDataServerFunctor
	{
		void operator()(FUTURE future, AsyncBulkDataServerModule* context);
	};

	class AsyncBulkDataServerModule
	{

	public:
		AsyncBulkDataServerModule(AsyncNetworkSystem* ans, DWORD dataWorkers = c_data_workers, Unicode address = c_data_server_address, Unicode port = c_data_server_port);
		~AsyncBulkDataServerModule();

	};
	#pragma endregion

	constexpr auto c_chat_workers = 16UL;

	constexpr auto c_chat_server_address = L"127.0.0.1";
	constexpr auto c_chat_server_port = L"19742";

	struct AsyncChatDataServerWorker;
	using AsyncChatDataServerWorkers = std::vector<AsyncChatDataServerWorker>;

	class AsyncChatDataServerModule;
	struct AsyncChatDataServerFunctor
	{
		void operator()(FUTURE future, AsyncChatDataServerModule* context);
	};

	class AsyncChatDataServerModule
	{

	public:
		AsyncChatDataServerModule(AsyncNetworkSystem* ans, DWORD chatWorkers = c_chat_workers, Unicode address = c_chat_server_address, Unicode port = c_chat_server_port);
	};

	/// <summary>
	/// The overall design we expect here is three sockets per connection
	/// 1) TCP - Data connection, this is what we have so far
	/// 2) TCP - Data connection, this will be for transferring large binary objects
	/// 3) UDP - Data connection, this will be used for position data
	/// </summary>
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

namespace Experimental
{
#define OP_CONTROL_CONNECT 1
#define OP_CONTROL_ACCEPT 2
#define OP_CONTROL_SEND 3
#define OP_CONTROL_RECV 4
#define OP_CONTROL_DISCONNECT 5

#define OP_CHAT_CONNECT 6
#define OP_CHAT_ACCEPT 7
#define OP_CHAT_SEND 8
#define OP_CHAT_RECV 9
#define OP_CHAT_DISCONNECT 10

#define OP_DATA_CONNECT 11
#define OP_DATA_ACCEPT 12
#define OP_DATA_SEND 13
#define OP_DATA_RECV 14
#define OP_DATA_DISCONNECT 15

#define OP_VECTOR_CONNECT 16
#define OP_VECTOR_ACCEPT 17
#define OP_VECTOR_SEND 18
#define OP_VECTOR_RECV 19
#define OP_VECTOR_DISCONNECT 20

	// Forward Declaration
	class AsyncBufferData;

	// RAII compliant-buffer and client connection data
	struct ClientConnectionData							// 260 + B
	{
		OVERLAPPED Overlapped;							//  32 = 32
		GUID ClientGUID = {};							//  16 = 48
		GUID ServerGUID = {};							//  16 = 64

		// Initial connection socket, provides addresses for other ports
		HANDLE ControlPort = INVALID_HANDLE_VALUE;		//   8 = 72
		SOCKET ControlSocket = INVALID_SOCKET;			//   8 = 80
		SOCKADDR_IN LocalControlSockAddr = {};			//  16 = 96
		SOCKADDR_IN RemoteControlSockAddr = {};			//  16 = 112
		
		// TCP small packets
		HANDLE ChatPort = INVALID_HANDLE_VALUE;			//   8 = 120
		SOCKET ChatSocket = INVALID_SOCKET;				//   8 = 128
		SOCKADDR_IN LocalChatSockAddr = {};				//  16 = 144
		SOCKADDR_IN RemoteChatsockAddr = {};			//  16 = 160

		// TCP large packets
		HANDLE DataPort = INVALID_HANDLE_VALUE;			//   8 = 168
		SOCKET DataSocket = INVALID_SOCKET;				//   8 = 176
		SOCKADDR_IN LocalDataSockAddr = {};				//  16 = 192
		SOCKADDR_IN RemoteDataSockAddr = {};			//  16 = 208

		// UDP small packets
		HANDLE VectorPort = INVALID_HANDLE_VALUE;		//   8 = 216
		SOCKET VectorSocket = INVALID_SOCKET;			//   8 = 224
		SOCKADDR_IN LocalVectorSockAddr = {};			//  16 = 240
		SOCKADDR_IN RemoteVectorSockAddr = {};			//  16 = 256

		DWORD Length = 0;								//   4 = 260
		CHAR* Buffer = nullptr;							//   B - Buffer

		ClientConnectionData(DWORD bufferLength, GUID clientGuid) : ClientGUID(clientGuid), Length(bufferLength)
		{
			std::lock_guard lock(m_mutex);

			ClayMemZero(&Overlapped, sizeof(OVERLAPPED));
			if (FAILED(CoCreateGuid(&ServerGUID))) throw;

			if (Length > 0)
			{
				Buffer = new CHAR[Length];
				ClayMemZero(&Buffer, sizeof(Length));
			}
		}
		~ClientConnectionData()
		{
			delete[] Buffer;
		}

	private:
		std::mutex m_mutex;
	};
	using ClientConnectionDataPtr = std::unique_ptr<ClientConnectionData>;
	using ClientConnectionsData = std::vector<ClientConnectionDataPtr>;

	// Per operation data object
	class AsyncBufferData
	{
		std::mutex m_mutex;

		SOCKET m_socket;
		HANDLE m_port;

		DWORD m_operation;

		DWORD m_length = 0;
		CHAR* m_buffer = nullptr;

	public:
		AsyncBufferData(SOCKET socket, HANDLE port, DWORD length) : m_socket(socket), m_port(port), m_length(length)
		{
			std::lock_guard lock(m_mutex);

			if (m_length > 0)
			{
				m_buffer = new CHAR[m_length];
				ClayMemZero(&m_buffer, sizeof(m_length));
			}
		}
		~AsyncBufferData()
		{
			delete[] m_buffer;
		}

		DWORD GetLength() { return m_length; }
		CHAR* GetBuffer() { return m_buffer; }

		void SetBuffer(CHAR* buffer, DWORD length)
		{
			std::lock_guard lock(m_mutex);

			if (m_length != length)
			{
				delete[] m_buffer;
				m_length = length;
				m_buffer = new CHAR[m_length];
			}
			ClayMemCopy(m_buffer, buffer, m_length);
		}

	};
	using AsyncBufferDataPtr = std::unique_ptr<AsyncBufferData>;
	using AsyncBuffersData = std::vector<AsyncBufferDataPtr>;

	class AsyncBufferManager
	{
		AsyncBuffersData m_buffers = {};

		// map ptr, free - look aside list

	public:
		AsyncBufferManager() = default;
		~AsyncBufferManager() = default;

		AsyncBufferData* MakeAsyncBufferData(DWORD bufferLength)
		{
			m_buffers.emplace_back(std::make_unique<AsyncBufferData>(bufferLength));
			return m_buffers.back().get();
		}

		CHAR* GetBuffer(DWORD bufferLength)
		{
			for (auto& element : m_buffers)
			{
				if (element->GetLength() == bufferLength)
				{
					return element->GetBuffer();
				}
			}
			return nullptr;
		}

		// free buffer

	};
	using AsyncBufferManagerPtr = std::unique_ptr<AsyncBufferManager>;

	class ConnectedClientManager
	{
		ClientConnectionsData m_client_connections = {};

	public:
		ConnectedClientManager() = default;
		~ConnectedClientManager() = default;

		ClientConnectionData* MakeClientConnectionData(DWORD bufferLength, GUID clientGuid)
		{
			m_client_connections.emplace_back(std::make_unique<ClientConnectionData>(bufferLength, clientGuid));
			return m_client_connections.back().get();
		}

		std::tuple<SOCKET, SOCKET, SOCKET, SOCKET> GetSockets(GUID client)
		{
			for (auto& element : m_client_connections)
			{
				if (element->ClientGUID == client)
				{
					return std::make_tuple(element->ControlSocket, element->ChatSocket, element->DataSocket, element->VectorSocket);
				}
			}
			return std::make_tuple(INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET);
		}
		SOCKET GetClientControlSocket(GUID client)
		{
			for (auto& element : m_client_connections)
			{
				if (element->ClientGUID == client)
				{
					return element->ControlSocket;
				}
			}
			return 0;
		}
		SOCKET GetClientChatSocket(GUID client)
		{
			for (auto& element : m_client_connections)
			{
				if (element->ClientGUID == client)
				{
					return element->ChatSocket;
				}
			}
			return 0;
		}
		SOCKET GetClientDataSocket(GUID client)
		{
			for (auto& element : m_client_connections)
			{
				if (element->ClientGUID == client)
				{
					return element->DataSocket;
				}
			}
			return 0;
		}
		SOCKET GetClientVectorSocket(GUID client)
		{
			for (auto& element : m_client_connections)
			{
				if (element->ClientGUID == client)
				{
					return element->VectorSocket;
				}
			}
			return 0;
		}
	};
	using ConnectedClientManagerPtr = std::unique_ptr<ConnectedClientManager>;
}