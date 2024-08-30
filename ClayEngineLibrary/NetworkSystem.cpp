#include "pch.h"
#include "NetworkSystem.h"



#pragma region Listen Server Module
void ClayEngine::Networking::AcceptThreadFunctor::operator()(std::future<void> future, void* ptr)
{
	auto ns = ClayEngine::Services::GetService<NetworkSystem>(std::this_thread::get_id());
	auto port = ns->GetListenServerPort();
	auto hints = ns->GetListenServerHints();
	auto timeout = ns->GetListenServerTimeout();
	int rc = 0;

	// Create a basic socket with the provided configuration
	auto s = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (s == INVALID_SOCKET)
	{
		rc = ProcessWSALastError();
		throw std::exception("WSA ERROR: socket() INVALID_SOCKET");
	}

	// Reconfigure the socket for non-blocking I/O mode
	u_long argp = 1ul;
	if (ioctlsocket(s, FIONBIO, &argp) == SOCKET_ERROR)
	{
		rc = ProcessWSALastError();
		throw std::exception("WSA ERROR: ioctlsocket() SOCKET_ERROR");
	}

	// Create an inbound socket configuration for listening
	SOCKADDR_IN s_sin = { 0 };
	s_sin.sin_family = ADDRESS_FAMILY(hints.ai_family);
	s_sin.sin_addr.s_addr = htonl(INADDR_ANY);
	s_sin.sin_port = htons(port);

	// Bind the socket configuration to the socket
	if (bind(s, (SOCKADDR*)&s_sin, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		rc = ProcessWSALastError();
		throw std::exception("WSA ERROR: bind() SOCKET_ERROR");
	}

	// Set the socket to listen
	auto backlog = int(std::thread::hardware_concurrency() * 2);
	if (listen(s, backlog) == SOCKET_ERROR)
	{
		rc = ProcessWSALastError();
		throw std::exception("WSA ERROR: listen() SOCKET_ERROR");
	}

	WriteLine("WSA SUCCESS: ListenServerModule started");

	auto csm = ns->GetClientSocketModule();
	while (future.wait_for(std::chrono::milliseconds(timeout)) == std::future_status::timeout)
	{
		auto [b, r_s, r_sa] = checkAcceptForClient(s);
		if (b)
		{
			WriteLine("WSA SUCCESS: Connection accepted!");
			csm->AddClientSocket(r_s, r_sa);
		}
	}

	shutdown(s, SD_BOTH);
	closesocket(s);
}

std::tuple<bool, SOCKET, SOCKADDR> ClayEngine::Networking::AcceptThreadFunctor::checkAcceptForClient(SOCKET s)
{
	SOCKADDR sa = { 0 };
	int sa_length = sizeof(SOCKADDR_IN);
	int rc = 0;

	auto rs = accept(s, &sa, &sa_length);
	if (rs == INVALID_SOCKET) rc = ProcessWSALastError();

	if (rc != WSAEWOULDBLOCK)
	{
		WriteLine("WSA Connection accepted!");
		return std::make_tuple(true, rs, sa);
	}

	return std::make_tuple(false, NULL, SOCKADDR{ 0 });
}

ClayEngine::Networking::AcceptThreadContext::AcceptThreadContext()
{
	try
	{
		m_thread = std::thread{ AcceptThreadFunctor(), std::move(m_promise.get_future()), nullptr };
	}
	catch (std::exception ex)
	{
		WriteLine(ex.what());
	}
}

ClayEngine::Networking::AcceptThreadContext::~AcceptThreadContext()
{
	m_promise.set_value();
	if (m_thread.joinable()) m_thread.join();
}

ClayEngine::Networking::ListenServerModule::ListenServerModule()
{
	m_listeners.emplace_back(std::make_unique<AcceptThreadContext>());
}

ClayEngine::Networking::ListenServerModule::~ListenServerModule()
{
	for (auto& element : m_listeners)
	{
		element.reset();
	}
}
#pragma endregion

#pragma region Network System (Network Service API)
ClayEngine::Networking::NetworkSystem::NetworkSystem()
{
	auto rc = WSAStartup(MAKEWORD(2, 2), &m_wsadata);
	if (rc != 0) throw;

	m_client_sockets = std::make_unique<ClientSocketModule>();
}

ClayEngine::Networking::NetworkSystem::~NetworkSystem()
{
	StopListenServer();

	m_client_sockets.reset();
	m_client_sockets = nullptr;

	WSACleanup();
}

void ClayEngine::Networking::NetworkSystem::StartListenServer()
{
	if (!m_listen_server)
	{
		m_listen_server = std::make_unique<ListenServerModule>();
	}
}

void ClayEngine::Networking::NetworkSystem::StopListenServer()
{
	if (m_listen_server)
	{
		m_listen_server.reset();
		m_listen_server = nullptr;
	}
}

void ClayEngine::Networking::NetworkSystem::SetListenServerTimeout(int timeout)
{
	m_listen_server_loop_timeout = timeout;
}

int ClayEngine::Networking::NetworkSystem::GetListenServerTimeout()
{
	return m_listen_server_loop_timeout;
}

void ClayEngine::Networking::NetworkSystem::SetListenServerHints(int family, int socktype, int protocol)
{
	m_listen_server_hints.ai_family = family;
	m_listen_server_hints.ai_socktype = socktype;
	m_listen_server_hints.ai_protocol = protocol;
}

ADDRINFO ClayEngine::Networking::NetworkSystem::GetListenServerHints()
{
	return m_listen_server_hints;
}

void ClayEngine::Networking::NetworkSystem::SetListenServerPort(USHORT port)
{
	m_listen_server_port = port;
}

USHORT ClayEngine::Networking::NetworkSystem::GetListenServerPort()
{
	return m_listen_server_port;
}

ClayEngine::Networking::ClientSocketModuleRaw ClayEngine::Networking::NetworkSystem::GetClientSocketModule()
{
	return m_client_sockets.get();
}
#pragma endregion

#pragma region Client Connection Module
bool ClayEngine::Networking::ClientConnectionModule::tryConnectToServer()
{
	if (connect(m_s, (SOCKADDR*)&m_sin, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		auto rc = ProcessWSALastError();
		if (rc != WSAEWOULDBLOCK)
		{
			return false;
		}
	}

	return true;
}

ClayEngine::Networking::ClientConnectionModule::ClientConnectionModule(String address)
{
	auto ns = ClayEngine::Services::GetService<NetworkSystem>(std::this_thread::get_id());
	auto port = ns->GetClientConnectionPort();
	auto hints = ns->GetClientConnectionHints();
	auto timeout = ns->GetClientConnectionTimeout();
	UNREFERENCED_PARAMETER(timeout);
	int rc = 0;

	// Create a basic socket with the provided configuration
	m_s = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (m_s == INVALID_SOCKET)
	{
		rc = ProcessWSALastError();
		throw std::exception("WSA ERROR: socket() INVALID_SOCKET");
	}
	
	// Reconfigure the socket for non-blocking I/O mode
	u_long argp = 1ul;
	if (ioctlsocket(m_s, FIONBIO, &argp) == SOCKET_ERROR)
	{
		rc = ProcessWSALastError();
		throw std::exception("WSA ERROR: ioctlsocket() SOCKET_ERROR");
	}

	if (InetPtonA(AF_INET, address.c_str(), &m_sin.sin_addr.s_addr) == SOCKET_ERROR) throw;
	m_sin.sin_family = ADDRESS_FAMILY(hints.ai_family);
	m_sin.sin_port = htons(port);

	WriteLine("WSA SUCCESS: ClientConnectionModule begin tryConnectToServer()");

	while (!tryConnectToServer())
	{
		//Sleep(timeout);

		//if (future.wait_for(std::chrono::milliseconds(timeout)) != std::future_status::timeout) break;
	}

	WriteLine("WSA SUCCESS: ClientConnectionModule connected!");

}

ClayEngine::Networking::ClientConnectionModule::~ClientConnectionModule()
{
	shutdown(m_s, SD_BOTH);
	closesocket(m_s);
}
#pragma endregion

