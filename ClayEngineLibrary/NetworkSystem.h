#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Network System Library Class (C) 2022 Epoch Meridian, LLC.      */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include "ClayEngine.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

namespace ClayEngine
{
	namespace Networking
	{
		/// <summary>
		/// Temp struct for holding client sockets created by the listen server module
		/// </summary>
		struct ClientSocket
		{
			SOCKET m_s = {};
			SOCKADDR m_sa = {};
			SOCKADDR_IN m_sin = {};
			SOCKADDR_STORAGE m_sas = {};
		};

		/// <summary>
		/// Provides an interface to client sockets that have connected to this server
		/// </summary>
		class ClientSocketModule
		{
			using Clients = std::vector<ClientSocket>;
			Clients m_client_sockets = {};
			std::mutex m_client_sockets_mutex = {};

		public:
			ClientSocketModule()
			{

			}
			~ClientSocketModule()
			{

			}

			void AddClientSocket(SOCKET s, SOCKADDR sa)
			{
				std::scoped_lock guard(m_client_sockets_mutex);
				m_client_sockets.push_back(ClientSocket{ s, sa });
			}

			const Clients& GetClients()
			{
				return m_client_sockets;
			}

		};
		using ClientSocketModulePtr = std::unique_ptr<ClientSocketModule>;
		using ClientSocketModuleRaw = ClientSocketModule*;

		/// <summary>
		/// Process the HRESULT from a WinSock2 API call
		/// </summary>
		/// <returns></returns>
		inline int ProcessWSALastError();

		/// <summary>
		/// A class that provides static address lookup and result printing services
		/// </summary>
		class AddressResolver
		{
		public:
			static int WriteResolvedAddress(SOCKADDR* sa, int sa_length);
			static ADDRINFO* ResolveAddress(String address, String port, int family, int socktype, int protocol);
			static size_t MeasureAddressLength(ADDRINFO* address);
			static void FreeAddress(ADDRINFO* ai);
			static void FreeSocket(SOCKET s);
		};

		/// <summary>
		/// Thread entry point for listen and accept socket server
		/// </summary>
		struct AcceptThreadFunctor
		{
			void operator()(std::future<void> future, void* ptr);
		private:
			std::tuple<bool, SOCKET, SOCKADDR> checkAcceptForClient(SOCKET s);
		};

		/// <summary>
		/// Thread management and access context for the AcceptThreadFunctor
		/// </summary>
		class AcceptThreadContext
		{
			std::thread m_thread;
			std::promise<void> m_promise{};
		public:
			AcceptThreadContext();
			~AcceptThreadContext();
		};
		using AcceptThreadContextPtr = std::unique_ptr<AcceptThreadContext>;

		/// <summary>
		/// A module that manages multiple listen and accept socket threads
		/// </summary>
		class ListenServerModule
		{
			using Listeners = std::vector<AcceptThreadContextPtr>;
			Listeners m_listeners = {};

		public:
			ListenServerModule();
			~ListenServerModule();
		};
		using ListenServerModulePtr = std::unique_ptr<ListenServerModule>;
		using ListenServerModuleRaw = ListenServerModule*;

		/// <summary>
		/// A module that connects to a listen server
		/// </summary>
		class ClientConnectionModule
		{
			SOCKET m_s = {};
			SOCKADDR_IN m_sin = {};

			bool tryConnectToServer();

		public:
			ClientConnectionModule(String address);
			~ClientConnectionModule();
		};
		using ClientConnectionModulePtr = std::unique_ptr<ClientConnectionModule>;

		/// <summary>
		/// API for network subsystem
		/// </summary>
		class NetworkSystem
		{
			WSADATA m_wsadata;

			// Automatically instantiated
			ClientSocketModulePtr m_client_sockets = nullptr;

			// Conditional instantiation
			ListenServerModulePtr m_listen_server = nullptr;
			ADDRINFO m_listen_server_hints = {};
			USHORT m_listen_server_port = 0;
			int m_listen_server_loop_timeout = 0;

			ClientConnectionModulePtr m_client_connection = nullptr;
			ADDRINFO m_client_connection_hints = {};
			USHORT m_client_connection_port = 0;
			int m_client_connection_loop_timeout = 0;

		public:
			NetworkSystem();
			~NetworkSystem();

			#pragma region Listen Server API
			void StartListenServer();
			void StopListenServer();

			void SetListenServerTimeout(int timeout);
			int GetListenServerTimeout();

			void SetListenServerHints(int family, int socktype, int protocol);
			ADDRINFO GetListenServerHints();

			void SetListenServerPort(USHORT port);
			USHORT GetListenServerPort();

			ClientSocketModuleRaw GetClientSocketModule();
			#pragma endregion

			#pragma region Client Connection API
			void StartClientConnection(String address)
			{
				m_client_connection = std::make_unique<ClientConnectionModule>(address);
			}
			void StopClientConnection()
			{
				m_client_connection.reset();
				m_client_connection = nullptr;
			}

			void SetClientConnectionHints(int family, int socktype, int protocol)
			{
				m_client_connection_hints.ai_family = family;
				m_client_connection_hints.ai_socktype = socktype;
				m_client_connection_hints.ai_protocol = protocol;
			}
			ADDRINFO GetClientConnectionHints()
			{
				return m_client_connection_hints;
			}

			void SetClientConnectionPort(USHORT port)
			{
				m_client_connection_port = port;
			}
			USHORT GetClientConnectionPort()
			{
				return m_client_connection_port;
			}

			void SetClientConnectionTimeout(int timeout)
			{
				m_client_connection_loop_timeout = timeout;
			}
			int GetClientConnectionTimeout()
			{
				return m_client_connection_loop_timeout;
			}
			#pragma endregion

			void Run() // Debug dummy
			{
				while (true)
				{
					String s;
					std::cin >> s;
					break;
				}
			}

			// Compress()->Encrypt()->Send()
			// Recieve()->Decrypt()->Decompress()
		};
		using NetworkSystemPtr = std::unique_ptr<NetworkSystem>;
		using NetworkSystemRaw = NetworkSystem*;
	}
}
