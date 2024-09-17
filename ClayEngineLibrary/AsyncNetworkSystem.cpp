#include "pch.h"
#include "AsyncNetworkSystem.h"

int ClayEngine::ProcessWSALastError()
{
    switch (int rc = WSAGetLastError())
    {
    case WSAEWOULDBLOCK:
        //WriteLine("WSA: Would Block");
        return rc;
    case WSAECONNABORTED:
        WriteLine("WSA: Connection Aborted");
        return rc;
    case WSAECONNRESET:
        WriteLine("WSA: Connection Reset");
        return rc;
    case WSAECONNREFUSED:
        WriteLine("WSA: Connection Refused");
        return rc;
    case WSAETIMEDOUT:
        WriteLine("WSA: Timed Out");
        return rc;
    case WSAEADDRINUSE:
        WriteLine("WSA: Address In Use");
        return rc;
    case WSAENOTSOCK:
        WriteLine("WSA: An operation was attempted on something that is not a socket.");
        return rc;
    case WSAEISCONN:
        WriteLine("WSA: A connect request was made on an already conencted socket.");
        return rc;
    case WSAEADDRNOTAVAIL:
        WriteLine("WSA: The requested address is not valid in its context.");
        return rc;
    case WSAEINVAL:
        WriteLine("WSA: An invalid argument was supplied.");
        return rc;
    case WSAEFAULT:
        WriteLine("WSA: The system detected an invalid pointer address in attempting to use a pointer argument in a call.");
        return rc;
    default:
        std::stringstream ss;
        ss << "WSA: Error code: " << rc;
        WriteLine(ss.str());
        return rc;
    }
}

#pragma region Async Listen Server Module - Accept Socket Data Implementation
struct ClayEngine::AsyncListenServerModule::AcceptSocketData
{
    OVERLAPPED Overlapped;
    SOCKET Socket = INVALID_SOCKET;
    CHAR* Buffer = NULL;
    DWORD BufferLength = 0;
	GUID ServerGUID = {}; // Assigned by server
	GUID ClientGUID = {}; // Provided by client

    AcceptSocketData(DWORD bufferLength);
    ~AcceptSocketData();

    void Reset();
};

ClayEngine::AsyncListenServerModule::AcceptSocketData::AcceptSocketData(DWORD bufferLength)
    : BufferLength(bufferLength)
{
    ClayMemZero(&Overlapped, sizeof(OVERLAPPED));
    Socket = INVALID_SOCKET;
    Buffer = new CHAR[BufferLength];
	if (FAILED(CoCreateGuid(&ServerGUID))) throw;
    ClayMemZero(&ClientGUID, sizeof(GUID));
}

ClayEngine::AsyncListenServerModule::AcceptSocketData::~AcceptSocketData()
{
    delete[] Buffer;
}

void ClayEngine::AsyncListenServerModule::AcceptSocketData::Reset()
{
    ClayMemZero(&Overlapped, sizeof(OVERLAPPED));
    Socket = INVALID_SOCKET;
    ClayMemZero(&Buffer, BufferLength);
	ClayMemZero(&ServerGUID, sizeof(GUID));
	if (FAILED(CoCreateGuid(&ServerGUID))) throw;
	ClayMemZero(&ClientGUID, sizeof(GUID));
}
#pragma endregion

#pragma region Async Listen Server Functor Implementation
struct ClayEngine::AsyncListenServerWorker
{
    THREAD Thread;
    PROMISE Promise = {};
};

void ClayEngine::AsyncListenServerFunctor::operator()(FUTURE future, AsyncListenServerModule* context)
{
    if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED))) throw;

	while (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout)
    {
        if (!context) continue;

        AsyncListenServerModule::AcceptSocketData* opData = nullptr;
        bool isAcceptDataReady = false;

        const DWORD bufferLength = c_dwReceiveDataLength + c_dwLocalAddressLength + c_dwRemoteAddressLength;

        if (!isAcceptDataReady)
        {
            // Create a AcceptSocketData
            if (!opData)
                opData = context->MakeAcceptSocketData(bufferLength);
            else
                opData->Reset();

            // Create a new socket
            opData->Socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
            if (opData->Socket == INVALID_SOCKET)
            {
                throw std::runtime_error("Failed to create a new socket");
            }

            // Associate the socket with the completion port
            if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(opData->Socket), context->GetListenQueue(), reinterpret_cast<ULONG_PTR>(opData), 0) == NULL)
            {
                closesocket(opData->Socket);
                throw std::runtime_error("Failed to associate the socket with the completion port");
            }

            // Run AcceptEx to start accepting connections asynchronously
            DWORD bytesReceived = 0;
            BOOL result = context->GetAcceptExFunction()(
                context->GetListenSocket(),
                opData->Socket,
                opData->Buffer,
                c_dwReceiveDataLength,
                c_dwLocalAddressLength, // Size of local address buffer
                c_dwRemoteAddressLength, // Size of remote address buffer
                &bytesReceived,
                &opData->Overlapped // Overlapped structure to be notified when the operation completes
                );

            if (!result && WSAGetLastError() != WSA_IO_PENDING)
            {
                // Handle AcceptEx failure
                closesocket(opData->Socket);
                throw std::runtime_error("Failed to post AcceptEx");
            }

            isAcceptDataReady = true;
        }

        // Loop to wait for incoming connections using GetQueuedCompletionStatus 
        DWORD bytesTransferred;
        ULONG_PTR completionKey; // I expect this to be the handle to the listen queue
        LPOVERLAPPED overlapped; // Should == opData
        if (GetQueuedCompletionStatus(context->GetListenQueue(), &bytesTransferred, &completionKey, &overlapped, INFINITE))
        {
            // Cast the overlapped structure back to AcceptSocketData to retrieve connection details
            auto completedOpData = reinterpret_cast<AsyncListenServerModule::AcceptSocketData*>(overlapped);

            if (opData != completedOpData) throw;;

            // Retrieve local and remote addresses (assumes buffers for AcceptEx are set correctly)
            SOCKADDR* localAddr = nullptr;
            SOCKADDR* remoteAddr = nullptr;
            int localAddrLen = 0;
            int remoteAddrLen = 0;
            context->GetAcceptExSockAddrsFunction()(completedOpData->Buffer, c_dwReceiveDataLength,
                c_dwLocalAddressLength, c_dwRemoteAddressLength,
                &localAddr, &localAddrLen,
                &remoteAddr, &remoteAddrLen
            );

            memcpy(&completedOpData->ClientGUID, completedOpData->Buffer, sizeof(GUID));
            GUID serverGuid = completedOpData->ServerGUID;
            GUID clientGuid = completedOpData->ClientGUID;

			BOOL bNoDelay = TRUE; // Disable Nagle's algorithm
            setsockopt(completedOpData->Socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&bNoDelay, sizeof(BOOL));

			int nSendBufSize = 64 * 1042; // 64KB send buffer
			setsockopt(completedOpData->Socket, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBufSize, sizeof(int));
			int nRecvBufSize = 64 * 1042; // 64KB recv buffer
			setsockopt(completedOpData->Socket, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBufSize, sizeof(int));

			BOOL bKeepAlive = TRUE; // Enable keep-alive
			setsockopt(completedOpData->Socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&bKeepAlive, sizeof(BOOL));

            // Keepalive times
			tcp_keepalive keepalive;
			keepalive.onoff = 1; // Enable keep-alive
			keepalive.keepalivetime = 60000; // 60 seconds
			keepalive.keepaliveinterval = 10000; // 10 second
			DWORD dwBytesReturned = 0;
			WSAIoctl(completedOpData->Socket, SIO_KEEPALIVE_VALS, &keepalive, sizeof(tcp_keepalive), NULL, 0, &dwBytesReturned, NULL, NULL);

            BOOL bReuseAddr = TRUE; // Enable address reuse
			setsockopt(completedOpData->Socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseAddr, sizeof(BOOL));

            linger ling;
			ling.l_onoff = 1; // Enable linger
            ling.l_linger = 0; // Discard unsent data
			setsockopt(completedOpData->Socket, SOL_SOCKET, SO_LINGER, (const char*)&ling, sizeof(linger));

			int nLowWaterMark = 16; // Set the low water mark to 16 bytes
			setsockopt(completedOpData->Socket, SOL_SOCKET, SO_RCVBUF, (const char*)&nLowWaterMark, sizeof(int));

			u_long iMode = 1; // Set the socket to non-blocking
            ioctlsocket(completedOpData->Socket, FIONBIO, &iMode);

            // Create ClientConnectionData to handle the newly accepted client
			auto remoteAddrIn = reinterpret_cast<SOCKADDR_IN*>(remoteAddr);
            context->MakeClientConnectionData(completedOpData->Socket, localAddr, remoteAddrIn);

            // Associate with IOCP... Perhaps something to be done in the ANS

            isAcceptDataReady = false;
        }
        else
        {
            continue;
        }
        
        if (opData) context->FreeAcceptSocketData(opData);

        CoUninitialize();
    }
}
#pragma endregion

#pragma region Async Listen Server Module Implementation
HANDLE ClayEngine::AsyncListenServerModule::createCompletionPort(DWORD listenWorkers)
{
    return CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, listenWorkers);
}

SOCKET ClayEngine::AsyncListenServerModule::createListenSocket(int flags, int family, int socktype, int protocol, Unicode address, Unicode port)
{
    // Configure hints for the listen socket
    ADDRINFOW hints = {};
    hints.ai_flags = flags;
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;

    // Get addrinfo for the listen socket
    if (FAILED(GetAddrInfoW(address.c_str(), port.c_str(), &hints, &m_listen_addrinfo)))
    {
        WSACleanup();
        throw;
    }

    auto hSocket = WSASocketW(m_listen_addrinfo->ai_family, m_listen_addrinfo->ai_socktype, m_listen_addrinfo->ai_protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (hSocket == INVALID_SOCKET)
    {
        FreeAddrInfoW(m_listen_addrinfo);
        WSACleanup();
        throw;
    }

    return hSocket;
}

void ClayEngine::AsyncListenServerModule::getWSAExtensionFunctions()
{
    DWORD dwBytes = 0;
    GUID guidAcceptEx = WSAID_ACCEPTEX;
    auto rc = WSAIoctl(m_listen_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidAcceptEx, sizeof(GUID),
        &m_fnAcceptEx, sizeof(LPFN_ACCEPTEX),
        &dwBytes, NULL, NULL);
    if (rc == SOCKET_ERROR)
    {
        closesocket(m_listen_socket);
        FreeAddrInfoW(m_listen_addrinfo);
        WSACleanup();
        throw;
    }

    dwBytes = 0;
    GUID guidGetAcceptExSockAddr = WSAID_GETACCEPTEXSOCKADDRS;
    rc = WSAIoctl(m_listen_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidGetAcceptExSockAddr, sizeof(GUID),
        &m_fnGetAcceptExSockAddrs, sizeof(LPFN_GETACCEPTEXSOCKADDRS),
        &dwBytes, NULL, NULL);
    if (rc == SOCKET_ERROR)
    {
        closesocket(m_listen_socket);
        FreeAddrInfoW(m_listen_addrinfo);
        WSACleanup();
        throw;
    }
}

void ClayEngine::AsyncListenServerModule::bindListenSocket()
{
    // Bind listen socket to address configuration
    auto rc = bind(m_listen_socket, m_listen_addrinfo->ai_addr, static_cast<int>(m_listen_addrinfo->ai_addrlen));
    if (rc == SOCKET_ERROR)
    {
        closesocket(m_listen_socket);
        FreeAddrInfoW(m_listen_addrinfo);
        throw;
    }

    // Set the socket to listen
    rc = SOCKET_ERROR;
    rc = listen(m_listen_socket, c_listen_backlog);
    if (rc == SOCKET_ERROR)
    {
        closesocket(m_listen_socket);
        FreeAddrInfoW(m_listen_addrinfo);
        throw;
    }

    // Associate the listen socket with the completion port
    if (!CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listen_socket), m_listen_queue, NULL, 1))
    {
        auto ec = GetLastError();

        closesocket(m_listen_socket);
        FreeAddrInfoW(m_listen_addrinfo);
        throw;
    }
}

ClayEngine::AsyncListenServerModule::AsyncListenServerModule(AsyncNetworkSystem* ans, DWORD listenWorkers, int flags, int family, int socktype, int protocol, Unicode address, Unicode port) : m_ans(ans)
{
    // Create Completion Port
    m_listen_queue = createCompletionPort(listenWorkers);

    // Create Listen Socket
    m_listen_socket = createListenSocket(flags, family, socktype, protocol, address, port);

    // Configure Accept and Bind Listen Socket
    getWSAExtensionFunctions();
    bindListenSocket();

    // Free Address Info
    FreeAddrInfoW(m_listen_addrinfo);

    // Create Worker Threads
    for (auto i = 0UL; i < listenWorkers; i++)
    {
        auto lt = AsyncListenServerWorker();
        lt.Thread = THREAD(AsyncListenServerFunctor(), lt.Promise.get_future(), this);
        m_listen_threads.emplace_back(std::move(lt));
    }
}

ClayEngine::AsyncListenServerModule::~AsyncListenServerModule()
{
    for (auto& element : m_listen_threads)
    {
        // Shutdown the listen worker threads
        element.Promise.set_value();
        if (element.Thread.joinable()) element.Thread.join();
    }

    closesocket(m_listen_socket);
    CloseHandle(m_listen_queue);
}

ClayEngine::AsyncListenServerModule::AcceptSocketData* ClayEngine::AsyncListenServerModule::MakeAcceptSocketData(DWORD bufferLength)
{
    LockGuard lock(m_accept_sockets_mutex);

    m_accept_sockets.emplace_back(std::make_unique<AcceptSocketData>(bufferLength));
    return m_accept_sockets.back().get();
}

void ClayEngine::AsyncListenServerModule::FreeAcceptSocketData(AcceptSocketData* acceptSocketData)
{
    m_accept_sockets.erase(std::remove_if(m_accept_sockets.begin(), m_accept_sockets.end(), [acceptSocketData](const AcceptSocketDataPtr& pAcceptSocketData) { return acceptSocketData == pAcceptSocketData.get(); }), m_accept_sockets.end());
}

void ClayEngine::AsyncListenServerModule::MakeClientConnectionData(SOCKET socket, SOCKADDR* local, SOCKADDR_IN* remote)
{
    m_ans->MakeClientConnectionData(socket, local, remote);
}
#pragma endregion

#pragma region Async Network System - Client Connection Data Implementation
struct ClayEngine::AsyncNetworkSystem::ClientConnectionData
{
    SOCKET Socket = INVALID_SOCKET;
    SOCKADDR SockAddr = {};
    SOCKADDR_IN SockAddrIn = {};

    ClientConnectionData(SOCKET socket, SOCKADDR* local, SOCKADDR_IN* remote);
    ~ClientConnectionData();
};

ClayEngine::AsyncNetworkSystem::ClientConnectionData::ClientConnectionData(SOCKET socket, SOCKADDR* local, SOCKADDR_IN* remote)
    : Socket(socket)
{
    memcpy(&SockAddr, local, sizeof(SOCKADDR));
    memcpy(&SockAddrIn, remote, sizeof(SOCKADDR_IN));
}

ClayEngine::AsyncNetworkSystem::ClientConnectionData::~ClientConnectionData()
{
    if (Socket) closesocket(Socket);
}
#pragma endregion

#pragma region Async Network System Implementation
ClayEngine::AsyncNetworkSystem::AsyncNetworkSystem(AffinityData affinityData, Unicode className, Document document)
    : m_affinity_data(affinityData)
{
    if (FAILED(WSAStartup(MAKEWORD(2, 2), &m_wsaData))) throw std::runtime_error("WSAStartup() failed... WTF?");

    //TODO: Read json and configure as defined


    try
    {
        m_listen_server = std::make_unique<AsyncListenServerModule>(this);
    }
    catch (...)
    {
        WSACleanup();

        throw std::runtime_error("Failed to create Async Listen Server");
    }
}

ClayEngine::AsyncNetworkSystem::~AsyncNetworkSystem()
{
    for (auto& element : m_client_connections)
    {
        element.reset();
    }

    if (m_listen_server) m_listen_server.reset();

    WSACleanup();
}

void ClayEngine::AsyncNetworkSystem::MakeClientConnectionData(SOCKET socket, SOCKADDR* local, SOCKADDR_IN* remote)
{
    LockGuard lock(m_client_connections_mutex);

    WriteLine(L"Added Client Socket");

    m_client_connections.emplace_back(std::make_unique<ClientConnectionData>(socket, local, remote));
}
#pragma endregion

#pragma region Client Connection Module Implementation
bool ClayEngine::ClientConnectionModule::tryConnectToServer()
{
	if (connect(m_socket, reinterpret_cast<SOCKADDR*>(&m_remote_sockaddr), sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		auto rc = ProcessWSALastError();
		if (rc != WSAEWOULDBLOCK) return false;
	}

    return true;
}

ClayEngine::ClientConnectionModule::ClientConnectionModule()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    ADDRINFO hints = {};
	hints.ai_family = AF_INET;

    // Create a basic socket
    m_socket = socket(hints.ai_family, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) throw;

    // Set the socket to non-blocking
    u_long iMode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &iMode) == SOCKET_ERROR) throw;

    // Convert address to network address
    if (FAILED(InetPtonW(hints.ai_family, address.c_str(), &m_remote_sockaddr.sin_addr.s_addr))) throw;
    
    // Convert port to network port
    m_remote_sockaddr.sin_port = htons(static_cast<u_short>(std::stoi(port.c_str())));
    
	// Set the address family
    m_remote_sockaddr.sin_family = ADDRESS_FAMILY(hints.ai_family);


    while (!tryConnectToServer())
    {
        continue;
    }

    WriteLine("WSA SUCCESS: ClientConnectionModule connected!");
}

ClayEngine::ClientConnectionModule::~ClientConnectionModule()
{
    shutdown(m_socket, SD_BOTH);
    closesocket(m_socket);
    WSACleanup();
}
#pragma endregion

