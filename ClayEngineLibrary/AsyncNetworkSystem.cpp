#include "pch.h"
#include "AsyncNetworkSystem.h"

#pragma region Async Listen Server Functor Implementation
struct ClayEngine::AsyncListenServerWorker
{
    THREAD Thread;
    PROMISE Promise = {};
};

void ClayEngine::AsyncListenServerFunctor::operator()(FUTURE future, AsyncListenServerModule* context)
{
	while (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout)
    {
        if (!context) continue;

        AsyncListenServerModule::AcceptSocketData* opData = nullptr;
        bool isAcceptDataReady = false;

        if (!isAcceptDataReady)
        {
            // Create a AcceptSocketData
            if (!opData)
                opData = context->MakeAcceptSocketData(64);
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
                &opData->Buffer,
                opData->BufferLength,
                sizeof(SOCKADDR_IN) + 16, // Size of local address buffer
                sizeof(SOCKADDR_IN) + 16, // Size of remote address buffer
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

            if (opData != completedOpData) throw;

            auto fn = context->GetAcceptExSockAddrsFunction();

            // Retrieve local and remote addresses (assumes buffers for AcceptEx are set correctly)
            SOCKADDR* localAddr = nullptr;
            SOCKADDR* remoteAddr = nullptr;
            int localAddrLen = 0;
            int remoteAddrLen = 0;
            fn(completedOpData->Buffer, completedOpData->BufferLength,
                sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
                &localAddr, &localAddrLen,
                &remoteAddr, &remoteAddrLen
            );

            // Set socket options as needed
            //setsockopt(completedOpData->Socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&ans->GetListenSocket(), sizeof(ans->GetListenSocket()));

            // Create ClientConnectionData to handle the newly accepted client
			auto remoteAddrIn = reinterpret_cast<SOCKADDR_IN*>(remoteAddr);
			// Above call throws a null ptr exception for remoteAddr

            context->MakeClientConnectionData(completedOpData->Socket, std::move(localAddr), std::move(remoteAddrIn));

            isAcceptDataReady = false;
        }
        else
        {
            continue;
        }
        
        if (opData) context->FreeAcceptSocketData(opData);
    }
}
#pragma endregion

#pragma region Async Listen Server Module - Accept Socket Data Implementation
struct ClayEngine::AsyncListenServerModule::AcceptSocketData
{
    OVERLAPPED Overlapped;
    SOCKET Socket = INVALID_SOCKET;
    CHAR* Buffer = NULL;
    DWORD BufferLength = 0;

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
ClayEngine::AsyncNetworkSystem::AsyncNetworkSystem(AffinityData affinityData)
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

    m_client_connections.emplace_back(std::make_unique<ClientConnectionData>(socket, local, remote));
}
#pragma endregion
