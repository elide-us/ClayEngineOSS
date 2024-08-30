#pragma once

#include "Strings.h"

#include <thread>
#include <future>
#include <mutex>
#include <chrono>
#include <vector>
#include <string>
//#include <memory>

namespace Experimental
{
	constexpr auto c_service_threads = 10UL; // 2x Accept, 4x Send, 4x Recv
	constexpr auto c_family = AF_INET;
	constexpr auto c_socktype = SOCK_STREAM;
	constexpr auto c_protocol = IPPROTO_TCP;

	enum class OPERATION;
	#define OP_WSA_ACCEPT 1
	#define OP_WSA_SEND 2
	#define OP_WSA_RECV 3
	#define OP_FILE_READ 4
	#define OP_FILE_WRITE 5
	#define OP_TEST_DEBUG 6
	#define OP_THREAD_EXIT 7

	using THREAD = std::thread;
	using PROMISE = std::promise<void>;
	using FUTURE = std::future<void>;
	using MUTEX = std::mutex;

	using LockGuard = std::lock_guard<MUTEX>;
	using Nanoseconds = std::chrono::milliseconds;
	using Unicode = std::wstring;
	using String = std::string;

	// A worker thread object holding the promise that is used to shut down the thread's loop
	struct WorkerThread;
	using WorkerThreads = std::vector<WorkerThread>;

	// Per operation data, primarily used for WSASend/WSARecv
	struct OperationData;
	using OperationsData = std::vector<OperationData*>;

	struct AsyncServerSocket;
	using AsyncServerSocketPtr = std::unique_ptr<AsyncServerSocket>;
	using AsyncServerSockets = std::vector<AsyncServerSocketPtr>;

	struct ClientSocket;
	using ClientSocketPtr = std::unique_ptr<ClientSocket>;
	using ClientSockets = std::vector<ClientSocketPtr>;

	// This functor responds to the AcceptEx messages
	// Should assign the completionKey return value to the ListenerAffinity value of the ClientSocket object
	struct AsyncListenWorker
	{
		void operator()(HANDLE hCompletionPort, FUTURE future);
	};

	// This functor responds to the WSASend/WSARecv messages
	struct AsyncNetworkWorker
	{
		void operator()(HANDLE hCompletionPort, FUTURE future);
	};

	// Uses Non-blocking I/O loop to accept client sockets
	struct NbioListenWorker
	{
		void operator()(FUTURE future);
	private:
		std::tuple<bool, SOCKET, SOCKADDR> checkAcceptForClient(SOCKET s);
	};

	class NbioListenContext
	{
		THREAD m_thread;
		PROMISE m_promise;

	public:
		NbioListenContext();
		~NbioListenContext();
	};
	using NbioListenContextPtr = std::unique_ptr<NbioListenContext>;

	// Uses worker threads to complete AcceptEx
	class AsyncListenServerModule
	{
		// Listen Socket
		AsyncServerSocketPtr m_listen_socket = nullptr;

		// Worker Threads
		size_t m_worker_count = 4;
		WorkerThreads m_workers = {};
	public:
		AsyncListenServerModule(const Unicode address, const Unicode port);
		~AsyncListenServerModule();

		void Start();
		void Stop();
	};
	using AsyncListenServerModulePtr = std::unique_ptr<AsyncListenServerModule>;

	// Uses non-blocking I/O to complete accept
	class NbioListenServerModule
	{
		class AcceptThreadContext
		{
			std::thread m_thread;
			std::promise<void> m_promise = {};
		};
		using AcceptThreadContextPtr = std::unique_ptr<AcceptThreadContext>;

		struct AcceptThreadFunctor
		{
			void operator()(std::future<void> future)
			{
				while (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout)
				{
					//checkAcceptForClient(ClientSocket ...);
				}
			}
		private:
			std::tuple<bool, SOCKET, SOCKADDR> checkAcceptForClient(SOCKET s);
		};

	public:
		NbioListenServerModule();
		~NbioListenServerModule();

		void Start();
		void Stop();

	};
	using NbioListenServerModulePtr = std::unique_ptr<NbioListenServerModule>;

	// Uses non-blocking I/O to connect to a server
	class ClientConnectModule
	{
		SOCKET m_socket = {};
		SOCKADDR_IN m_sockaddr_in = {};

		bool tryConnectToServer();

	public:
		ClientConnectModule(const Unicode address);
		~ClientConnectModule();
	};
	using ClientConnectModulePtr = std::unique_ptr<ClientConnectModule>;

	// Uses worker threads to complete WSARecv/WSASend
	class AsyncServerModule;
	using AsyncServerModulePtr = std::unique_ptr<AsyncServerModule>;

	// This experimental class is the top level API for the new
	// AsyncNetworkSystem. We will need to modularize the pieces 
	// so we can instantitate client/server/etc in the same way 
	// as the older NetworkSystem NBIO implementation. The Listener
	// will not be in its own thread, instead we will have dedicated 
	// workers just for the listener completion port. Note that 
	// there can be multiple listeners on different ports. We 
	// should be able to instatitate multiple modules as required.
	class AsyncNetworkSystem
	{
		#pragma region Async Client Server Setup
		// Completion Port for WSARecv/WSASend
		HANDLE m_queue = nullptr;

		// Client Sockets

		// AsyncListenServerModule
		// - Has-a Completion Port
		// - Has-a Listen Socket
		// - Has-a lpfnAcceptEx
		// NbioListenServerModule
		// - Has-a ListenServerContext
		// - Has-a ListenServerFunctor
		// ClientConnectModule - For the Client to connect to Server

		// AcceptEx workers populate this collection
		ClientSockets m_clients = {};
		MUTEX m_clients_mutex = {};
		void addClientSocket(SOCKET socket);
		//void removeClientSocket(SOCKET socket);

		// Operation Data
		OperationsData m_operations = {};
		MUTEX m_operations_mutex = {};
		OperationData* createOperationData(OPERATION operation, DWORD bufferSize);
		void deleteOperationData(OperationData* operationData);

		// Worker Threads
		size_t m_worker_count = 10;
		WorkerThreads m_workers = {};
		void startWorkerThreads();
		void stopWorkerThreads();
		#pragma endregion

		#pragma region Async Listen Server Setup
		AsyncListenServerModulePtr m_listen_server = nullptr;

		// Above should replace most of below...
		Unicode m_address = {};
		Unicode m_port = {};
		ADDRINFOW* m_addrinfo = {};
		
		LPFN_ACCEPTEX lpfnAcceptEx = nullptr;

		SOCKET m_listen_socket = {}; //v1
		HANDLE m_listen_queue = {}; //v1
		AsyncServerSocketPtr m_listener = nullptr; //v2
		
		AsyncServerSockets m_listen_sockets = {}; //v3
		//void addListenSocket(SOCKET socket, HANDLE completionPort);
		//void removeListenSocket(SOCKET socket);

		size_t m_listen_worker_count = 4;
		WorkerThreads m_listen_workers = {};
		void startListenServer(); // To be called in the constructor
		//void startListenThreads();
		//void stopListenThreads();
		#pragma endregion

		AsyncListenServerModulePtr m_async = nullptr;
		NbioListenServerModulePtr m_nbio = nullptr;

		ClientConnectModulePtr m_client = nullptr;

		AsyncServerModulePtr m_server = nullptr;

	public:
		AsyncNetworkSystem(const Unicode address, const Unicode port);
		~AsyncNetworkSystem();

		void PostAccept();
		void PostTest();
		void PostExit();
		void PostRecv();
		void PostSend();
	
		int ProcessWSALastError();

		int WriteResolvedAddress(SOCKADDR* sa, int sa_length);
		ADDRINFO* ResolveAddress(String address, String port, int family, int socktype, int protocol);
		size_t MeasureAddressLength(ADDRINFO* address);
		void FreeAddress(ADDRINFO* ai);
		void FreeSocket(SOCKET s);

	
	};
}
