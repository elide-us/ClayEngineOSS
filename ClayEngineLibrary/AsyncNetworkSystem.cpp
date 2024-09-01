#include "pch.h"
#include "AsyncNetworkSystem.h"

//using namespace ClayEngine;

enum class Experimental::OPERATION
{
	AcceptConnection = 1,
	ReadFromNetwork = 2,
	WriteToNetwork = 3,
	ReadFromFile = 4,
	WriteToFile = 5,
	TestDebug = 6,
	ExitThread = 7,
};

struct Experimental::WorkerThread
{
	THREAD Thread;
	PROMISE Promise;
};

#define ClayMemZero(Address, Length) memset(Address, 0, Length)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

#pragma region Network Helpers
int Experimental::AsyncNetworkSystem::ProcessWSALastError()
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

int Experimental::AsyncNetworkSystem::WriteResolvedAddress(SOCKADDR* sa, int sa_length)
{
	char host[NI_MAXHOST];
	int host_length = NI_MAXHOST;
	char port[NI_MAXSERV];
	int port_length = NI_MAXSERV;
	int rc;

	rc = getnameinfo(sa, sa_length, host, host_length, port, port_length, NI_NUMERICHOST | NI_NUMERICSERV);

	if (rc != NO_ERROR)
	{
		std::stringstream ss;
		ss << __FILE__ << ": getnameinfo failed: " << rc;
		WriteLine(ss.str());

		return rc;
	}

	if (strcmp(port, "0") == 0)
	{
		if (sa->sa_family == AF_INET)
		{
			std::stringstream ss;
			ss << "Resolved Local Address: " << "[" << host << "]";
			WriteLine(ss.str());
		}
		else
		{
			std::stringstream ss;
			ss << "Resolved Local Address: " << host;
			WriteLine(ss.str());
		}
	}
	else
	{
		if (sa->sa_family == AF_INET)
		{
			std::stringstream ss;
			ss << "Resolved Remote Address: " << "[" << host << "]:" << port;
			WriteLine(ss.str());
		}
		else
		{
			std::stringstream ss;
			ss << "Resolved Remote Address: " << host << ":" << port;
			WriteLine(ss.str());
		}
	}

	return rc;
}

ADDRINFO* Experimental::AsyncNetworkSystem::ResolveAddress(String address, String port, int family, int socktype, int protocol)
{
	ADDRINFO hints = {};
	ADDRINFO* head = nullptr;
	int rc = 0;

	hints.ai_flags = ((address.c_str()) ? 0 : AI_PASSIVE);
	hints.ai_family = family;
	hints.ai_socktype = socktype;
	hints.ai_protocol = protocol;

	rc = getaddrinfo(address.c_str(), port.c_str(), &hints, &head);
	if (rc == WSANOTINITIALISED) return NULL;
	//auto ph = std::unique_ptr<ADDRINFO, AddrInfoDeleter>(head);
	//ph.reset();

	if (rc != 0)
	{
		std::stringstream ss;
		ss << __FILE__ << ": getaddrinfo failed: " << rc << " Invalid address: " << address.c_str();
		WriteLine(ss.str());
		return NULL;
	}
	else
	{
		auto h = head;
		while (h->ai_next)
		{
			WriteResolvedAddress(h->ai_addr, int(h->ai_addrlen));
			h = h->ai_next;
		}
		WriteResolvedAddress(h->ai_addr, int(h->ai_addrlen));
	}

	return head;
}

size_t Experimental::AsyncNetworkSystem::MeasureAddressLength(ADDRINFO* address)
{
	size_t len = 0;
	ADDRINFO* ptr = address;
	if (ptr)
	{
		++len;
		while (ptr->ai_next)
		{
			++len;
			ptr = ptr->ai_next;
		}
	}
	return len;
}

void Experimental::AsyncNetworkSystem::FreeAddress(ADDRINFO* ai)
{
	freeaddrinfo(ai);
}

void Experimental::AsyncNetworkSystem::FreeSocket(SOCKET s)
{
	closesocket(s);
}
#pragma endregion

#pragma region Thread Functors
void Experimental::AsyncNetworkWorker::operator()(HANDLE hCompletionPort, FUTURE future)
{
	DWORD bytesTransferred;
	ULONG_PTR completionKey;
	LPOVERLAPPED pOverlapped;

	while (future.wait_for(Nanoseconds(0)) == std::future_status::timeout)
	{
		// Get the completion status
		BOOL result = GetQueuedCompletionStatus(hCompletionPort, &bytesTransferred, &completionKey, &pOverlapped, INFINITE);
		if (result)
		{
			auto data = reinterpret_cast<Experimental::OperationData*>(completionKey);
			
			switch ((int)data->Operation)
			{
			case 2:
			{
				// Process WSARecv for data
			}
			case 3:
			{
				// Process WSASend for data
			}
			default:
				continue;
			}
		}

	}
}

void Experimental::AsyncListenWorker::operator()(HANDLE hCompletionPort, FUTURE future)
{
	DWORD bytesTransferred;
	ULONG_PTR completionKey;
	LPOVERLAPPED pOverlapped;

	while (future.wait_for(Nanoseconds(0)) == std::future_status::timeout)
	{
		// Get the completion status
		BOOL result = GetQueuedCompletionStatus(hCompletionPort, &bytesTransferred, &completionKey, &pOverlapped, INFINITE);
		if (result)
		{
			auto data = reinterpret_cast<Experimental::OperationData*>(pOverlapped);

			if ((int)data->Operation == 1)
			{
				//Process AcceptEx data
			}
		}
	}

}

void Experimental::NbioListenWorker::operator()(FUTURE future)
{

}

std::tuple<bool, SOCKET, SOCKADDR> Experimental::NbioListenWorker::checkAcceptForClient(SOCKET s)
{
	return std::tuple<bool, SOCKET, SOCKADDR>();
}
#pragma endregion

#pragma region Data Objects
struct Experimental::ClientSocket
{
	SOCKET Socket = {}; // AcceptEx binds client to this
	SOCKET ListenerAffinity = {}; // Completion port returns this as the completionKey

	SOCKADDR SockAddr; // Docs said something about having to look this up with another AcceptEx command...
	SOCKADDR_IN SockAddrIn;
	SOCKADDR_STORAGE SockAddrStorage;
};

struct Experimental::AsyncServerSocket
{
	SOCKET Socket; // Socket bound to AddrInfo
	HANDLE CompletionPort; // Reference to the Completion Port this socket will be using.

	Unicode Address;
	Unicode Port;

	ADDRINFOW Hints; // Address configuration request
	ADDRINFOW AddrInfo; // Address configuration returned
};

struct Experimental::OperationData
{
	OVERLAPPED Overlapped;

	OPERATION Operation;

	WSABUF wsaBuf;
	CHAR* Buffer;
	DWORD BufferSize;

	SOCKET Socket;
	SOCKADDR_IN SockAddr;

	OperationData(OPERATION operation, DWORD bufferSize)
		: Operation(operation), BufferSize(bufferSize), Socket(INVALID_SOCKET)
	{
		ClayMemZero(&Overlapped, sizeof(OVERLAPPED));
		Buffer = new CHAR[BufferSize];
		wsaBuf.buf = Buffer;
		wsaBuf.len = BufferSize;
	}
	~OperationData()
	{
		delete[] Buffer;
	}
};
#pragma endregion

// Instantiate this object to manage a thread for the listener
Experimental::NbioListenContext::NbioListenContext()
{

}

Experimental::NbioListenContext::~NbioListenContext()
{

}
// Instantiate this object for the NBIO API
Experimental::NbioListenServerModule::NbioListenServerModule()
{

}

Experimental::NbioListenServerModule::~NbioListenServerModule()
{

}

// Should start up the thread, socket, and listen loop
void Experimental::NbioListenServerModule::startListenServer()
{

}

void Experimental::NbioListenServerModule::stopListenServer()
{

}


bool Experimental::ClientConnectModule::tryConnectToServer()
{
	return false;
}

Experimental::ClientConnectModule::ClientConnectModule(const Unicode address)
{

}

Experimental::ClientConnectModule::~ClientConnectModule()
{

}

Experimental::OperationData* Experimental::AsyncNetworkSystem::createOperationData(OPERATION operation, DWORD bufferSize)
{
	LockGuard lock(m_operations_mutex);

	m_operations.emplace_back(new OperationData(operation, bufferSize));
	
	return m_operations.back();
}

void Experimental::AsyncNetworkSystem::addClientSocket(SOCKET socket)
{
	LockGuard lock(m_clients_mutex);

	m_clients.emplace_back(std::make_unique<ClientSocket>(socket));
}

void Experimental::AsyncNetworkSystem::deleteOperationData(OperationData* operationData)
{
	LockGuard lock(m_clients_mutex);

	m_operations.erase(
		std::remove_if(m_operations.begin(), m_operations.end(),
			[operationData](const OperationData* element) { return element == operationData; }),
		m_operations.end()
	);
}

void Experimental::AsyncNetworkSystem::startWorkerThreads()
{
	for (auto i = 0; i < m_worker_count; ++i)
	{
		auto wt = WorkerThread{};
		wt.Promise = PROMISE{};
		wt.Thread = THREAD{ AsyncNetworkWorker(), m_queue, wt.Promise.get_future() };

		m_workers.emplace_back(std::move(wt));
	}
}

void Experimental::AsyncNetworkSystem::stopWorkerThreads()
{
	for (auto& element : m_workers)
	{
		element.Promise.set_value();

		if (element.Thread.joinable())
			element.Thread.join();
	}
}

void Experimental::AsyncNetworkSystem::startListenServer()
{
	// Get the local address info struct
	ADDRINFOW hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (FAILED(GetAddrInfoW(m_address.c_str(), m_port.c_str(), &hints, &m_addrinfo)))
	{
		WSACleanup();
		throw std::runtime_error("hResult::FAILED GetAddrInfoW()");
	}

	// Create the listen socket
	m_listen_socket = WSASocketW(m_addrinfo->ai_family, m_addrinfo->ai_socktype, m_addrinfo->ai_protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_listen_socket == INVALID_SOCKET)
	{
		FreeAddrInfoW(m_addrinfo);
		WSACleanup();
		throw std::runtime_error("INVALID_SOCKET WSASocketW()");
	}

	// Initialize the AcceptEx function
	DWORD dwBytes = 0;
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	auto sr = WSAIoctl(m_listen_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(guidAcceptEx),
		&lpfnAcceptEx, sizeof(lpfnAcceptEx),
		&dwBytes, NULL, NULL);
	if (sr == SOCKET_ERROR)
	{
		closesocket(m_listen_socket);
		FreeAddrInfoW(m_addrinfo);
		WSACleanup();

		throw std::runtime_error("SOCKET_ERROR WSAIoctl()");
	}

	// Bind the listen socket
	sr = bind(m_listen_socket, m_addrinfo->ai_addr, static_cast<int>(m_addrinfo->ai_addrlen));
	if (sr == SOCKET_ERROR)
	{
		closesocket(m_listen_socket);
		FreeAddrInfoW(m_addrinfo);
		WSACleanup();

		throw std::runtime_error("SOCKET_ERROR bind()");
	}

	sr = listen(m_listen_socket, SOMAXCONN);
	if (sr == SOCKET_ERROR)
	{
		closesocket(m_listen_socket);
		FreeAddrInfoW(m_addrinfo);
		WSACleanup();

		throw std::runtime_error("SOCKET_ERROR listen()");
	}

	// Associate the listen socket with the completion port
	if (!CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listen_socket), m_queue, 0, static_cast<DWORD>(m_worker_count)))
	{
		auto error = GetLastError();
		std::cout << "GetLastError(): " << error << std::endl;

		closesocket(m_listen_socket);
		FreeAddrInfoW(m_addrinfo);
		WSACleanup();

		throw std::runtime_error("Failed CreateIoCompletionPort()");
	}
}

Experimental::AsyncNetworkSystem::AsyncNetworkSystem(const Unicode address, const Unicode port)
	: m_address(address), m_port(port)
{
	// Initialize WinSock2
	WSADATA wsaData;
	if (FAILED(WSAStartup(MAKEWORD(2, 2), &wsaData)))
	{
		throw std::runtime_error("hResult::FAILED WSAStartup()");
	}
	#pragma region Client Server Setup
	// Client Completion Port WSASend/WSARecv
	m_queue = CreateIoCompletionPort(INVALID_HANDLE_VALUE, m_queue, NULL, 10);
	if (!m_queue) throw std::runtime_error("Failed CreateIoCompletionPort()");

	// Worker Threads
	startWorkerThreads();
	#pragma endregion

	// Server Listen Socket Setup
	startListenServer();
}

Experimental::AsyncNetworkSystem::~AsyncNetworkSystem()
{
	stopWorkerThreads();

	if (m_queue) CloseHandle(m_queue);
	if (m_listen_socket) closesocket(m_listen_socket);
	if (m_addrinfo) FreeAddrInfoW(m_addrinfo);

	WSACleanup();
}

void Experimental::AsyncNetworkSystem::PostAccept()
{
	OperationData* od = createOperationData(OPERATION::AcceptConnection, 15);
	auto s = WSASocketW(m_addrinfo->ai_family, m_addrinfo->ai_socktype, m_addrinfo->ai_protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (s == INVALID_SOCKET)
	{
		throw std::runtime_error("Failed WSASocketW() for AcceptEx()");
	}
	od->Socket = s;
	od->Buffer = "Test message...";

	m_clients_mutex.lock();
	m_clients.emplace_back(s);
	m_clients_mutex.unlock();

	DWORD bytesReceived = 0;
	if (!lpfnAcceptEx(m_listen_socket, od->Socket, od->Buffer,
		0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		&bytesReceived, &od->Overlapped))
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			throw std::runtime_error("Failed AcceptEx()");
		}
	}

	if (CreateIoCompletionPort(
		reinterpret_cast<HANDLE>(s), m_queue,
		reinterpret_cast<ULONG_PTR>(od), 0) == NULL)
	{
		throw std::runtime_error("Failed CreateIoCompletionPort()");
	}
}

void Experimental::AsyncNetworkSystem::PostTest()
{
	OperationData* od = createOperationData(OPERATION::TestDebug, 15);
	od->Buffer = "Test message...";
	PostQueuedCompletionStatus(m_queue, 0, reinterpret_cast<ULONG_PTR>(od), &od->Overlapped);
}

void Experimental::AsyncNetworkSystem::PostExit()
{

}

void Experimental::AsyncNetworkSystem::PostRecv()
{

}

void Experimental::AsyncNetworkSystem::PostSend()
{

}
