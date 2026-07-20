#include "pch.h"

#include "Utility.h"
//
//void ClearConsole(HANDLE hConsole)
//{
//	COORD coordScreen = { 0, 0 };    // home for the cursor 
//	DWORD cCharsWritten;
//	CONSOLE_SCREEN_BUFFER_INFO csbi;
//	DWORD dwConSize;
//
//	// Get the number of character cells in the current buffer. 
//	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
//	{
//		return;
//	}
//
//	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
//
//	// Fill the entire screen with blanks.
//	if (!FillConsoleOutputCharacterW(hConsole, // Handle to stdout
//		L'?',                                  // Character to write to the buffer
//		dwConSize,                             // Number of cells to write 
//		coordScreen,                           // Coordinates of first cell 
//		&cCharsWritten))                       // Receive number of characters written
//	{
//		return;
//	}
//
//	// Get the current text attribute.
//	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
//	{
//		return;
//	}
//
//	// Set the buffer's attributes accordingly.
//	if (!FillConsoleOutputAttribute(hConsole,  // Handle to stdout
//		csbi.wAttributes,                      // Character attributes to use
//		dwConSize,                             // Number of cells to set attribute 
//		coordScreen,                           // Coordinates of first cell 
//		&cCharsWritten))                       // Receive number of characters written
//	{
//		return;
//	}
//
//	// Put the cursor at its home coordinates.
//	SetConsoleCursorPosition(hConsole, coordScreen);
//}
//
//int wmain(int argc, wchar_t* argv[])
//{
//	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
//	ClearConsole(hStdout);
//
//	system("pause");
//	return 0;
//}

int gAddressFamily = AF_UNSPEC; // default to unspecified
int gSocketType = SOCK_STREAM;  // default to stream socket type
int gProtocol = IPPROTO_TCP;    // default to TCP protocol
int	gBufferSize = c_BufferSize;
int gOverlappedCount = c_ServerOverlappedCount;

wchar_t* gLocalAddr = nullptr;      // local interface to bind to
std::wstring gBindPort = c_DefaultPort;      // local port to bind to
USHORT gLocalPort = 0x0000FFFD;
std::wstring _gLocalPort = c_LocalPort;

CRITICAL_SECTION gThreadListCritSec;

// Counters
volatile LONG gBytesRead = 0;
volatile LONG gBytesSent = 0;
volatile LONG gStartTime = 0;
volatile LONG gBytesReadLast = 0;
volatile LONG gBytesSentLast = 0;
volatile LONG gStartTimeLast = 0;
volatile LONG gTotalConnections = 0;
volatile LONG gCurrentConnections = 0;
volatile LONG gConnectionRefused = 0;

void usage(const wchar_t* exename)
{
	Console::WriteLine(L"Server usage: %s [-a 4|6] [-e port] [-l local-addr] [-p udp|tcp]", exename);
	Console::WriteLine(L"  -a 4|6     Address family, 4 = IPv4, 6 = IPv6 [default = IPv4]");
	Console::WriteLine(L"  -p tcp|udp Which protocol to use [default = TCP]");
	Console::WriteLine(L"  -s size    Buffer size for buffer/recv [default = %d]", gBufferSize);
	Console::WriteLine(L"  -c count   Overlapped count");
	Console::WriteLine(L"  -l addr    Local address to bind [default INADDR_ANY for IPv4 or INADDR6_ANY for IPv6]");
	Console::WriteLine(L"  -o port    Port number [default = %s]", gBindPort.c_str());
	Console::Pause();
	ExitProcess(-1);
}
void parse(int argc, wchar_t* argv[])
{
	if (argc == 1) usage(argv[0]);

	for (auto i = 1; i < argc; i++)
	{
		if (argv[i][0] != L'-' || wcslen(argv[i]) < 2)
			usage(argv[0]);
		else
		{
			switch (tolower(argv[i][1]))
			{
			case L'a':               // address family - IPv4 or IPv6
				if (i + 1 >= argc)
					usage(argv[0]);
				if (argv[i + 1][0] == L'4')
					gAddressFamily = AF_INET;
				else if (argv[i + 1][0] == L'6')
					gAddressFamily = AF_INET6;
				else
					usage(argv[0]);
				i++;
				break;
			case L'p':               // protocol - TCP or UDP
				if (i + 1 >= argc)
					usage(argv[0]);
				if (_wcsnicmp(argv[i + 1], L"tcp", 3) == 0)
				{
					gProtocol = IPPROTO_TCP;
					gSocketType = SOCK_STREAM;
				}
				else if (_wcsnicmp(argv[i + 1], L"udp", 3) == 0)
				{
					gProtocol = IPPROTO_UDP;
					gSocketType = SOCK_DGRAM;
				}
				else
					usage(argv[0]);
				i++;
				break;
			case L's':               // buffer size for send/recv
				if (i + 1 >= argc)
					usage(argv[0]);
				gBufferSize = _wtol(argv[++i]);
				break;
			case L'c':               // overlapped count
				if (i + 1 >= argc)
					usage(argv[0]);
				gOverlappedCount = _wtol(argv[++i]);
				break;
			case L'l':               // local address for binding
				if (i + 1 >= argc)
					usage(argv[0]);
				gLocalAddr = argv[++i];
				break;
			case L'o':               // endpoint - port number
				if (i + 1 >= argc)
					usage(argv[0]);
				gBindPort = argv[++i];
				break;
			default:
				usage(argv[0]);
				break;
			}
		}
	}
}

void PrintStatistics()
{
	ULONG bps;
	ULONG tick;
	ULONG elapsed;

	tick = GetTickCount();
	elapsed = (tick - gStartTime) / 1000;
	if (elapsed == 0) return;

	Console::WriteLine();

	bps = gBytesSent / elapsed;
	Console::WriteLine(L"Average BPS sent: %lu [%lu]", bps, gBytesSent);

	bps = gBytesRead / elapsed;
	Console::WriteLine(L"Average BPS read: %lu [%lu]", bps, gBytesRead);

	elapsed = (tick - gStartTimeLast) / 1000;
	if (elapsed == 0) return;

	bps = gBytesSentLast / elapsed;
	Console::WriteLine(L"Current BPS sent: %lu", bps);

	bps = gBytesReadLast / elapsed;
	Console::WriteLine(L"Current BPS read: %lu", bps);


	InterlockedExchange(&gBytesSentLast, 0);
	InterlockedExchange(&gBytesReadLast, 0);

	gStartTimeLast = tick;
}

int PostAccept(OverlappedSocket* socket_, OverlappedBuffer* buffer)
{
	DWORD dwBytes;
	int result = NO_ERROR;

	buffer->operation = OP_ACCEPT;

	// Create the client socket for an incoming connection
	//TODO: Review, uses hard coded SOCK_STREAM and IPPROTO_TCP here to create a new socket()
	buffer->sclient = socket(socket_->af, SOCK_STREAM, IPPROTO_TCP);
	if (buffer->sclient == INVALID_SOCKET)
	{
		Console::NetworkError(L"PostAccept: socket failed");
		return -1;
	}

	result = socket_->lpfnAcceptEx(socket_->s, buffer->sclient, buffer->buf, buffer->buflen - ((sizeof(SOCKADDR_STORAGE) + 16) * 2), sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16, &dwBytes, &buffer->ol);
	if (result == FALSE)
	{
		result = NO_ERROR;
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			Console::NetworkError(L"PostAccept: AcceptEx failed");
			result = SOCKET_ERROR;
		}
	}

	// Increment the outstanding overlapped count for this socket
	InterlockedIncrement(&socket_->OutstandingOps);

	return result;
}

int PostRecv(OverlappedSocket* socket, OverlappedBuffer* buffer)
{
	WSABUF  wsaBuffer;
	DWORD   dwBytes;
	DWORD	dwFlags = 0;
	int     result = NO_ERROR;

	buffer->operation = OP_READ;

	wsaBuffer.buf = buffer->buf;
	wsaBuffer.len = buffer->buflen;

	EnterCriticalSection(&socket->SockCritSec);

	// Assign the IO order to this receive. This must be performned within
	// the critical section. The operation of assigning the IO count and posting
	// the receive cannot be interupted.
	buffer->IoOrder = socket->IoCountIssued;
	socket->IoCountIssued++;

	if (gProtocol == IPPROTO_TCP)
	{
		result = WSARecv(socket->s, &wsaBuffer, 1, &dwBytes, &dwFlags, &buffer->ol, NULL);
	}
	else
	{
		result = WSARecvFrom(socket->s, &wsaBuffer, 1, &dwBytes, &dwFlags,
			reinterpret_cast<SOCKADDR*>(&buffer->addr), &buffer->addrlen, &buffer->ol, NULL);
	}

	LeaveCriticalSection(&socket->SockCritSec);

	if (result == SOCKET_ERROR)
	{
		result = NO_ERROR;
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			Console::NetworkError(L"PostRecv: WSARecv failed");
			result = SOCKET_ERROR;
		}
	}

	// Increment outstanding overlapped operations
	InterlockedIncrement(&socket->OutstandingOps);

	return result;
}

int PostSend(OverlappedSocket* socket, OverlappedBuffer* buffer)
{
	WSABUF  wsaBuffer;
	DWORD   bytes;
	int     result = NO_ERROR;

	buffer->operation = OP_WRITE;

	wsaBuffer.buf = buffer->buf;
	wsaBuffer.len = buffer->buflen;

	EnterCriticalSection(&socket->SockCritSec);

	// Incrmenting the last send issued and issuing the send should not be
	// interuptable.
	socket->LastSendIssued++;

	if (gProtocol == IPPROTO_TCP)
	{
		result = WSASend(socket->s, &wsaBuffer, 1, &bytes, 0, &buffer->ol, NULL);
	}
	else
	{
		result = WSASendTo(socket->s, &wsaBuffer, 1, &bytes, 0,
			reinterpret_cast<SOCKADDR*>(&buffer->addr), buffer->addrlen, &buffer->ol, NULL);
	}

	LeaveCriticalSection(&socket->SockCritSec);

	if (result == SOCKET_ERROR)
	{
		result = NO_ERROR;
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			Console::NetworkError(L"PostSend: WSASend failed");
			result = SOCKET_ERROR;
		}
	}

	// Increment the outstanding operation count
	InterlockedIncrement(&socket->OutstandingOps);

	return result;
}

//    This routine inserts a send buffer object into the socket's list
//    of out of order sends. The routine DoSends will go through this
//    list to issue those sends that are in the correct order.
void InsertPendingSend(OverlappedSocket* socket, OverlappedBuffer* buffer)
{
	OverlappedBuffer* ptr = nullptr;
	OverlappedBuffer* prev = nullptr;

	EnterCriticalSection(&socket->SockCritSec);

	buffer->next = nullptr;

	// This loop finds the place to put the send within the list.
	//    The send list is in the same order as the receives were
	//    posted.
	ptr = socket->OutOfOrderSends;
	while (ptr)
	{
		if (buffer->IoOrder < ptr->IoOrder)
		{
			break;
		}

		prev = ptr;
		ptr = ptr->next;
	}
	if (prev == NULL)
	{
		// Inserting at head
		socket->OutOfOrderSends = buffer;
		buffer->next = ptr;
	}
	else
	{
		// Insertion somewhere in the middle
		prev->next = buffer;
		buffer->next = ptr;
	}

	LeaveCriticalSection(&socket->SockCritSec);
}

//    This routine goes through a socket object's list of out of order send
//    buffers and sends as many of them up to the current send count. For each
//    send posted, the LastSendIssued is incremented. This means that the next
//    buffer sent must have an IO sequence nubmer equal to the LastSendIssued.
//    This is to preserve the order of data echoed back.
int DoSends(OverlappedSocket* socket)
{
	OverlappedBuffer* sendBuffer = nullptr;
	int result = NO_ERROR;

	EnterCriticalSection(&socket->SockCritSec);

	sendBuffer = socket->OutOfOrderSends;
	while ((sendBuffer) && (sendBuffer->IoOrder == socket->LastSendIssued))
	{
		if (PostSend(socket, sendBuffer) != NO_ERROR)
		{
			DeleteBufferObject(sendBuffer);

			result = SOCKET_ERROR;
			break;
		}
		socket->OutOfOrderSends = sendBuffer = sendBuffer->next;
	}

	LeaveCriticalSection(&socket->SockCritSec);

	return result;
}

//    This function handles the IO on a socket. In the event of a receive, the 
//    completed receive is posted again. For completed accepts, another AcceptEx
//    is posted. For completed sends, the buffer is freed.
void HandleIo(OverlappedSocket* socket, OverlappedBuffer* buffer, HANDLE completionPort, DWORD bytesTransferred, DWORD error)
{
	OverlappedSocket* clientSocket = nullptr;     // New client object for accepted connections
	OverlappedBuffer* recvBuffer = nullptr;       // Used to post new receives on accepted connections
	OverlappedBuffer* sendBuffer = nullptr;       // Used to post new sends for data received
	
	BOOL bFreeSocketObject = FALSE;
	
	char* tempBuffer;

	if (error != 0) Console::WriteLine(L"OP = %d, Error = %d", buffer->operation, error);
	if ((error != NO_ERROR) && (gProtocol == IPPROTO_TCP))
	{
		//An error occured on a TCP socket, free the associated per I/O buffer
		//  and see if there are any more outstanding operations. If so we must
		//  wait until they are complete as well.
		DeleteBufferObject(buffer);

		if (InterlockedDecrement(&socket->OutstandingOps) == 0)
		{
			Console::WriteLine(L"Freeing socket obj in GetOverlappedResult");
			DeleteSocketObject(socket);
		}
		return;
	}

	EnterCriticalSection(&socket->SockCritSec);

	switch (buffer->operation)
	{
	case OP_ACCEPT:
	{
		SOCKADDR_STORAGE* LocalSockaddr = nullptr;
		int LocalSockaddrLen;
		SOCKADDR_STORAGE* RemoteSockaddr = nullptr;
		int RemoteSockaddrLen;

		HANDLE hResult = nullptr;

		// Update counters
		InterlockedExchangeAdd(&gBytesRead, bytesTransferred);
		InterlockedExchangeAdd(&gBytesReadLast, bytesTransferred);

		// Print the client's addresss
		socket->lpfnGetAcceptExSockaddrs(buffer->buf,
			buffer->buflen - ((sizeof(SOCKADDR_STORAGE) + 16) * 2),
			sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
			(SOCKADDR**)&LocalSockaddr, &LocalSockaddrLen,
			(SOCKADDR**)&RemoteSockaddr, &RemoteSockaddrLen);

#ifdef _DEBUG
		//Under high connection stress this print slows things down
		wprintf_s(L"Received connection from: ");
		Network::PrintAddress((SOCKADDR*)RemoteSockaddr, RemoteSockaddrLen);
		wprintf_s(L"\n");
#endif

		// Get a new SOCKET_OBJ for the client connection
		clientSocket = NewSocketObject(buffer->sclient, socket->af, gProtocol, true);

		// Associate the new connection to our completion port
		hResult = CreateIoCompletionPort((HANDLE)buffer->sclient, completionPort, (ULONG_PTR)clientSocket, 0);
		if (hResult == nullptr)
		{
			Console::Error(L"CompletionThread: CreateIoCompletionPort failed");
			return;
		}

		// Get a BUFFER_OBJ to echo the data received with the accept back to the client
		sendBuffer = NewBufferObject(clientSocket, bytesTransferred);

		// Copy the buffer to the sending object
		memcpy(sendBuffer->buf, buffer->buf, bytesTransferred);

		// Post the send
		if (PostSend(clientSocket, sendBuffer) == NO_ERROR)
		{
			// Now post some receives on this new connection
			for (auto i = 0; i < gOverlappedCount; i++)
			{
				recvBuffer = NewBufferObject(clientSocket, gBufferSize);

				if (PostRecv(clientSocket, recvBuffer) != NO_ERROR)
				{
					// If for some reason the send call fails, clean up the connection
					DeleteBufferObject(recvBuffer);
					error = SOCKET_ERROR;
					break;
				}
			}
		}
		else
		{
			// If for some reason the send call fails, clean up the connection
			DeleteBufferObject(sendBuffer);
			error = SOCKET_ERROR;
		}

		// Re-post the AcceptEx
		PostAccept(socket, buffer);

		if (error != NO_ERROR)
		{
			if (clientSocket->OutstandingOps == 0)
			{
				closesocket(clientSocket->s);
				clientSocket->s = INVALID_SOCKET;
				DeleteSocketObject(clientSocket);
			}
			else
			{
				clientSocket->bClosing = TRUE;
			}
			error = NO_ERROR;
		}
		break;
	}
	case OP_READ:
	{
		if (error == NO_ERROR)
		{
			// Receive completed successfully
			if ((bytesTransferred > 0) || (gProtocol == IPPROTO_UDP))
			{
				InterlockedExchangeAdd(&gBytesRead, bytesTransferred);
				InterlockedExchangeAdd(&gBytesReadLast, bytesTransferred);

				// Create a buffer to send
				sendBuffer = NewBufferObject(socket, gBufferSize);

				if (gProtocol == IPPROTO_UDP)
				{
					memcpy(&sendBuffer->addr, &buffer->addr, buffer->addrlen);
				}

				// Swap the buffers (i.e. buffer we just received becomes the send buffer)
				tempBuffer = sendBuffer->buf;
				sendBuffer->buflen = bytesTransferred;
				sendBuffer->buf = buffer->buf;
				sendBuffer->IoOrder = buffer->IoOrder;

				buffer->buf = tempBuffer;
				buffer->buflen = gBufferSize;

				InsertPendingSend(socket, sendBuffer);

				if (DoSends(socket) != NO_ERROR)
				{
					error = SOCKET_ERROR;
				}
				else
				{
					// Post another receive
					if (PostRecv(socket, buffer) != NO_ERROR)
					{
						// In the event the recv fails, clean up the connection
						DeleteBufferObject(buffer);
						error = SOCKET_ERROR;
					}
				}
			}
			else
			{
				Console::WriteLine(L"0 bytes received");

				// Graceful close - the receive returned 0 bytes read
				socket->bClosing = TRUE;

				// Free the receive buffer
				DeleteBufferObject(buffer);

				if (DoSends(socket) != NO_ERROR)
				{
					Console::WriteLine(L"0: cleaning up in zero byte handler");
					error = SOCKET_ERROR;
				}

				// If this was the last outstanding operation on socket, clean it up
				if ((socket->OutstandingOps == 0) && (socket->OutOfOrderSends == nullptr))
				{
					Console::WriteLine(L"1: cleaning up in zero byte handler");
					bFreeSocketObject = TRUE;
				}
			}
		}
		else
		{
			// If for UDP, a receive completes with an error, we ignore it and re-post the recv
			if (gProtocol == IPPROTO_UDP)
			{
				if (PostRecv(socket, buffer) != NO_ERROR)
				{
					error = SOCKET_ERROR;
				}
			}
		}
		break;
	}
	case OP_WRITE:
	{
		// Update the counters
		InterlockedExchangeAdd(&gBytesSent, bytesTransferred);
		InterlockedExchangeAdd(&gBytesSentLast, bytesTransferred);

		DeleteBufferObject(buffer);

		if (DoSends(socket) != NO_ERROR)
		{
			Console::WriteLine(L"Cleaning up inside OP_WRITE handler");
			error = SOCKET_ERROR;
		}
		break;
	}
	default:
		break;
	}

	if (error != NO_ERROR)
	{
		socket->bClosing = TRUE;
	}

	// Check to see if socket is closing
	if ((InterlockedDecrement(&socket->OutstandingOps) == 0) && (socket->bClosing) && (socket->OutOfOrderSends == nullptr))
	{
		bFreeSocketObject = TRUE;
	}
	else
	{
		if (DoSends(socket) != NO_ERROR)
		{
			bFreeSocketObject = TRUE;
		}
	}

	LeaveCriticalSection(&socket->SockCritSec);

	if (bFreeSocketObject)
	{
		closesocket(socket->s);
		socket->s = INVALID_SOCKET;

		DeleteSocketObject(socket);
	}

	return;
}

//    This is the completion thread which services our completion port. One of
//    these threads is created per processor on the system. The thread sits in 
//    an infinite loop calling GetQueuedCompletionStatus and handling socket
//    IO that completed.
DWORD WINAPI CompletionThread(LPVOID lpParameter)
{
	OverlappedSocket* ioSocket = nullptr;          // Per socket object for completed I/O
	OverlappedBuffer* ioBuffer = nullptr;           // Per I/O object for completed I/O
	OVERLAPPED* lpOverlapped = nullptr;     // Pointer to overlapped structure for completed I/O
	
	HANDLE CompletionPort;        // Completion port handle
	DWORD BytesTransfered;       // Number of bytes transfered
	DWORD Flags;                 // Flags for completed I/O

	int result;
	int error;

	CompletionPort = static_cast<HANDLE>(lpParameter);
	while (true)
	{
		error = NO_ERROR;
		result = GetQueuedCompletionStatus(CompletionPort, &BytesTransfered, (PULONG_PTR)&ioSocket, &lpOverlapped, INFINITE);

		ioBuffer = CONTAINING_RECORD(lpOverlapped, OverlappedBuffer, ol);

		if (result == FALSE)
		{
			// If the call fails, call WSAGetOverlappedResult to translate the
			//    error code into a Winsock error code.
			Console::Error(L"CompletionThread: GetQueuedCompletionStatus failed");

			result = WSAGetOverlappedResult(ioSocket->s, &ioBuffer->ol, &BytesTransfered, FALSE, &Flags);
			if (result == FALSE)
			{
				error = WSAGetLastError();
			}
		}
		// Handle the IO operation
		HandleIo(ioSocket, ioBuffer, CompletionPort, BytesTransfered, error);
	}

	ExitThread(0);
	return 0;
}

//      This is the main program. It parses the command line and creates
//      the main socket. For UDP this socket is used to receive datagrams.
//      For TCP the socket is used to accept incoming client connections.
//      Each client TCP connection is handed off to a worker thread which
//      will receive any data on that connection until the connection is
//      closed.
int wmain(int argc, wchar_t* argv[])
{
	WSADATA wsaData;
	SYSTEM_INFO si;

	OverlappedSocket* pSocket = nullptr;
	OverlappedSocket* pvListenSockets = nullptr;

	int endpointCount = 0;
	DWORD dwResult;

	HANDLE CompletionPort = nullptr;
	HANDLE CompletionPortThreads[c_MaxCompletionThreads] = { nullptr };
	HANDLE hResult = nullptr;

	ADDRINFOW* paiResult = nullptr;
	ADDRINFOW* paiCursor = nullptr;

	parse(argc, argv);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		Console::WriteLine(L"Unable to load WinSock DLL");
		Console::Pause();
		return -1;
	}
	Console::WriteLine(L"WSAStartup completed successfully");

	// Create the completion port used by this server
	CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, static_cast<ULONG_PTR>(NULL), 0);
	if (CompletionPort == nullptr)
	{
		Console::Error(L"CreateIoCompletionPort failed");
		Console::Pause();
		return -1;
	}

	// Find out how many processors are on this system
	GetSystemInfo(&si);
	if (si.dwNumberOfProcessors > c_MaxCompletionThreads)
	{
		si.dwNumberOfProcessors = c_MaxCompletionThreads;
	}

	// Create the worker threads to service the completion notifications
	for (auto i = 0; i < static_cast<int>(si.dwNumberOfProcessors); i++)
	{
		CompletionPortThreads[i] = CreateThread(nullptr, 0, CompletionThread, (LPVOID)CompletionPort, 0, nullptr);
		if (CompletionPortThreads[i] == nullptr)
		{
			Console::Error(L"CreateThread failed");
			Console::Pause();
			return -1;
		}
	}

	Console::WriteLine(L"Resolver Address: %s, Port: %s, Family: %d", gLocalAddr, gBindPort.c_str(), gAddressFamily);
	paiResult = Network::ResolveAddress(gLocalAddr, gBindPort.c_str(), gAddressFamily, gSocketType, gProtocol);
	if (paiResult == nullptr)
	{
		Console::WriteLine(L"ResolveAddress failed to return any addresses");
		Console::Pause();
		return -1;
	}

	// For each local address returned, create a listening/receiving socket
	paiCursor = paiResult;
	while (paiCursor)
	{
		Network::PrintAddress(paiCursor->ai_addr, paiCursor->ai_addrlen);

		// Create a new socket
		pSocket = NewSocketObject(INVALID_SOCKET, paiCursor->ai_family, gProtocol, true);
		pSocket->s = socket(paiCursor->ai_family, paiCursor->ai_socktype, paiCursor->ai_protocol);
		if (pSocket->s == INVALID_SOCKET)
		{
			Console::NetworkError(L"Socket failed");
			return -1;
		}

		// Associate the socket and its SOCKET_OBJ to the completion port
		hResult = CreateIoCompletionPort((HANDLE)pSocket->s, CompletionPort, (ULONG_PTR)pSocket, 0);
		if (hResult == NULL)
		{
			Console::Error(L"CreateIoCompletionPort failed");
			return -1;
		}

		// bind the socket to a local address and port
		dwResult = bind(pSocket->s, paiCursor->ai_addr, paiCursor->ai_addrlen);
		if (dwResult == SOCKET_ERROR)
		{
			Console::NetworkError(L"Bind failed");
			return -1;
		}

		if (gProtocol == IPPROTO_TCP)
		{
			OverlappedBuffer* acceptBuffer = nullptr;

			GUID guidAcceptEx = WSAID_ACCEPTEX;
			GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;

			DWORD dwBytes;

			// Load the extension functions
			dwResult = WSAIoctl(pSocket->s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx), &pSocket->lpfnAcceptEx, sizeof(pSocket->lpfnAcceptEx), &dwBytes, NULL, NULL);
			if (dwResult == SOCKET_ERROR)
			{
				Console::NetworkError(L"WSAIoctl: SIO_GET_EXTENSION_FUNCTION_POINTER failed");
				return -1;
			}

			dwResult = WSAIoctl(pSocket->s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAcceptExSockaddrs, sizeof(guidGetAcceptExSockaddrs), &pSocket->lpfnGetAcceptExSockaddrs, sizeof(pSocket->lpfnGetAcceptExSockaddrs), &dwBytes, NULL, NULL);
			if (dwResult == SOCKET_ERROR)
			{
				Console::NetworkError(L"WSAIoctl: SIO_GET_EXTENSION_FUNCTION_POINTER failed");
				return -1;
			}

			// For TCP sockets, we need to "listen" on them
			dwResult = listen(pSocket->s, 100);
			if (dwResult == SOCKET_ERROR)
			{
				Console::NetworkError(L"Listen failed");
				return -1;
			}

			// Allocate the overlapped structures to keep track of the pending AcceptEx operations
			pSocket->PendingAccepts = reinterpret_cast<OverlappedBuffer**>(Memory::CallocDefault(gOverlappedCount, sizeof(OverlappedBuffer*)));
			for (auto i = 0; i < gOverlappedCount; i++)
			{
				pSocket->PendingAccepts[i] = acceptBuffer = NewBufferObject(pSocket, gBufferSize);
				PostAccept(pSocket, acceptBuffer);
			}

			// Maintain a list of the listening socket structures
			if (pvListenSockets == nullptr)
			{
				pvListenSockets = pSocket;
			}
			else
			{
				pSocket->next = pvListenSockets;
				pvListenSockets = pSocket;
			}
		}
		else
		{
			OverlappedBuffer* recvBuffer = nullptr;
			DWORD       bytes;
			int         inBuffer = 0;

			// Turn off UDP errors resulting from ICMP messages (port/host unreachable, etc)
			dwResult = WSAIoctl(pSocket->s, SIO_UDP_CONNRESET, &inBuffer, sizeof(inBuffer), NULL, 0, &bytes, NULL, NULL);
			if (dwResult == SOCKET_ERROR)
			{
				Console::NetworkError(L"WSAIoctl: SIO_UDP_CONNRESET failed");
			}

			// For UDP, simply post some receives
			for (auto i = 0; i < gOverlappedCount; i++)
			{
				recvBuffer = NewBufferObject(pSocket, gBufferSize);

				PostRecv(pSocket, recvBuffer);
			}
		}

		endpointCount++;
		paiCursor = paiCursor->ai_next;
	}
	FreeAddrInfoW(paiResult);

	gStartTime = gStartTimeLast = GetTickCount();
	int interval = 0;
	while (true)
	{
		Console::WriteLine(L"Beginning wait for connection... (5 sec)");
		dwResult = WSAWaitForMultipleEvents(si.dwNumberOfProcessors, CompletionPortThreads, TRUE, 5000, FALSE);
		if (dwResult == WAIT_FAILED)
		{
			Console::NetworkError(L"WSAWaitForMultipleEvents failed");
			break;
		}
		
		if (dwResult == WAIT_TIMEOUT)
		{
			PrintStatistics();

			interval++;
			if (interval == 12)
			{
				OverlappedSocket* listenSocket = nullptr;
				int optval;
				int optlen;

				// For TCP, cycle through all the outstanding AcceptEx operations
				//   to see if any of the client sockets have been connected but
				//   haven't received any data. If so, close them as they could be
				//   a denial of service attack.
				listenSocket = pvListenSockets;
				while (listenSocket)
				{
					for (auto i = 0; i < gOverlappedCount; i++)
					{
						optlen = sizeof(optval);
						dwResult = getsockopt(listenSocket->PendingAccepts[i]->sclient, SOL_SOCKET, SO_CONNECT_TIME, (char*)&optval, &optlen);
						if (dwResult == SOCKET_ERROR)
						{
							Console::NetworkError(L"getsockopt: SO_CONNECT_TIME failed");
							return -1;
						}
						// If the socket has been connected for more than 5 minutes,
						//    close it. If closed, the AcceptEx call will fail in the
						//    completion thread.
						if ((optval != 0xFFFFFFFF) && (optval > 300))
						{
							closesocket(listenSocket->PendingAccepts[i]->sclient);
						}
					}
					listenSocket = listenSocket->next;
				}
				interval = 0;
			}
		}
	}

	WSACleanup();
	Console::Pause(); 
	return 0;
}
