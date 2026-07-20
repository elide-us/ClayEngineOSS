#pragma once
#include "pch.h"

namespace
{
	static std::mt19937 s_engine(1974);

	const size_t c_BufferSize = 4096;
	const int c_ServerOverlappedCount = 5;
	const int c_ClientOverlappedCount = 1;
	const int c_MaxClientConnections = 10;
	const int c_MaxCompletionThreads = 32;
	const wchar_t* c_DefaultPort = L"42001";
	const wchar_t* c_LocalPort = L"65533";
	using CHARBUFFER = std::array<char, c_BufferSize>;
	using VOIDBUFFER = std::array<void*, c_BufferSize>;

	const int c_FileSize = 2000000;
	const int c_ClientSendCount = 100; // Probably used for prototype loop
}

struct NetStats
{
	volatile LONG BytesRead = 0;
	volatile LONG BytesSent = 0;
	volatile LONG StartTime = 0;
	volatile LONG BytesReadLast = 0;
	volatile LONG BytesSentLast = 0;
	volatile LONG StartTimeLast = 0;
	volatile LONG TotalConnections = 0;
	volatile LONG CurrentConnections = 0;
	volatile LONG ConnectionRefused = 0;
};

struct Console
{
	static std::wstring PrintfW(const wchar_t * inFormat, ...)
	{
		static wchar_t temp[c_BufferSize];

		va_list args;
		va_start(args, inFormat);

		_vsnwprintf_s(temp, c_BufferSize, c_BufferSize, inFormat, args);

		return std::wstring(temp);
	}

	static void Write(const wchar_t* format, ...)
	{
		static wchar_t szBuffer[c_BufferSize];

		va_list args;
		va_start(args, format);

		_vsnwprintf_s(szBuffer, c_BufferSize, c_BufferSize, format, args);

		wprintf_s(L"[%04x] %s", GetCurrentThreadId(), szBuffer);
	}
	static void WriteLine(const wchar_t* format, ...)
	{
		static wchar_t szBuffer[c_BufferSize];

		va_list args;
		va_start(args, format);

		_vsnwprintf_s(szBuffer, c_BufferSize, c_BufferSize, format, args);

		wprintf_s(L"[%04x] %s\n", GetCurrentThreadId(), szBuffer);
	}
	static void WriteLine()
	{
		wprintf_s(L"\n");
	}

	static wchar_t ReadKey()
	{
		auto c = _getwch();
		if (c == 0 || c == 0xE0)
		{
			c = _getwch();
			return L'?';
		}
		if (c == 8 || c == 13 || (c > 31 && c < 127))
		{
			return (wchar_t)c;
		}
		return L'!';
	}
	static std::wstring ReadLine()
	{
		std::wstring szReturn;
		getline(std::wcin, szReturn);
		return szReturn;
	}
	static std::wstring ReadLinePrompt(std::wstring prompt)
	{
		Write(prompt.c_str());
		return ReadLine();

	}

	static std::wstring ReadKeys(int count)
	{
		std::wstring szReturn(count + 1, '\0');
		for (auto i = 0; i <= count;)
		{
			auto c = ReadKey();
			if (i == count)
			{
				if (c == 8)
				{
					i--;
					if (i < 0) i++;
					else wprintf_s(L"\b \b");
				}
				if (c == 13)
				{
					_putwch(L'\n');
					break;
				}
			}
			else
			{
				if (c == 13)
				{
					szReturn.clear();
					throw std::exception("Error: Not enough characters provided.");
				}
				if (c == 8)
				{
					i--;
					if (i < 0) i++;
					else wprintf_s(L"\b \b");
				}
				else
				{
					szReturn[i] = c;
					_putwch(c);
					i++;
				}
			}
		}
		return szReturn;
	}
	static int ReadKeysPrompt(std::wstring &input, int count, std::wstring prompt)
	{
		Write(prompt.c_str());
		try
		{
			input = ReadKeys(count);
		}
		catch (std::exception e)
		{
			wprintf_s(L"\n%hs\n", e.what());
			input.clear();
			return -1;
		}
		return 0;
	}

	static void Pause()
	{
#ifdef _DEBUG
		system("pause");
#endif
	}

	static void NetworkError(std::wstring message)
	{
		WCHAR sMessageBuffer;
		DWORD dwErrorCode = WSAGetLastError();

		// GetLocaleInfoEx for new Locale Names 
		FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, dwErrorCode, GetLocaleInfoEx(L"en_US.utf-8", std::locale::ctype, nullptr, 0),
			&sMessageBuffer, 0, nullptr);

		Console::WriteLine(L"Network Error! %s: %d - %s", message.c_str(), dwErrorCode, sMessageBuffer);
	}
	static void Error(std::wstring message)
	{
		WCHAR sMessageBuffer;
		DWORD dwErrorCode = GetLastError();

		// GetLocaleInfoEx for new Locale Names 
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, dwErrorCode, GetLocaleInfoEx(L"en_US.utf-8", std::locale::ctype, nullptr, 0),
			&sMessageBuffer, 0, nullptr);

		Console::WriteLine(L"Win32 Error! %s: %d - %s", message.c_str(), dwErrorCode, sMessageBuffer);
	}

	static void SetLocale()
	{
		std::locale::global(std::locale("en_US.utf-8"));
		WriteLine(L"Set global locale to en_US.utf-8");
	}
};

struct Memory
{
	static void* MallocDefault(size_t bytes)
	{
		void* buffer = malloc(bytes);
		if (buffer == nullptr)
		{
			Console::WriteLine(L"Bad Malloc");
			Console::Pause();
			ExitProcess(EXIT_FAILURE);
		}
		return buffer;
	}
	static void* CallocDefault(size_t count, size_t bytes)
	{
		void* buffer = calloc(count, bytes);
		if (buffer == nullptr)
		{
			Console::WriteLine(L"Bad Calloc");
			Console::Pause();
			ExitProcess(EXIT_FAILURE);
		}
		return buffer;
	}
	static void* ReallocDefault(LPVOID lpParameter, size_t bytes)
	{
		void* buffer = realloc(lpParameter, bytes);
		if (buffer == nullptr)
		{
			Console::WriteLine(L"Bad Realloc");
			Console::Pause();
			ExitProcess(EXIT_FAILURE);
		}
		return buffer;
	}
	static void Free(LPVOID lpParameter)
	{
		free(lpParameter);
	}
};

struct Random
{
	static int GetNext(int min, int max)
	{
		static std::uniform_int_distribution<int> s_distribution(min, max);
		return s_distribution(s_engine);
	}
};

struct Network
{
	//    This routine takes a SOCKADDR structure and its length and prints
	//    converts it to a string representation. This string is printed
	//    to the console via stdout.
	static int PrintAddress(SOCKADDR* address, socklen_t length)
	{
		wchar_t nodeBuffer[NI_MAXHOST];
		int nodeBufferLength = NI_MAXHOST;

		wchar_t serviceBuffer[NI_MAXSERV];
		int serviceBufferLength = NI_MAXSERV;

		auto result = GetNameInfoW(address, length, nodeBuffer, NI_MAXHOST, serviceBuffer, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
		if (result != 0)
		{
			Console::WriteLine(L"%s: getnameinfo failed: %d", __FILE__, result);
			return result;
		}

		if (wcscmp(serviceBuffer, L"0") != 0)
		{
			if (address->sa_family == AF_INET)
			{
				Console::WriteLine(L"Address: [%s]:%s", nodeBuffer, serviceBuffer);
			}
			else
			{
				Console::WriteLine(L"Address: %s:%s", nodeBuffer, serviceBuffer);
			}
		}
		else
		{
			Console::WriteLine(L"Address: %s", nodeBuffer);
		}

		return NO_ERROR;
	}

	// This is similar to the PrintAddress function except that instead of
	//    printing the string address to the console, it is formatted into
	//    the supplied string buffer.
	static int FormatAddress(SOCKADDR* address, socklen_t length, wchar_t* addressBuffer, size_t addressBufferLength)
	{
		wchar_t nodeBuffer[NI_MAXHOST];
		int nodeBufferLength = NI_MAXHOST;
		wchar_t serviceBuffer[NI_MAXSERV];
		int serviceBufferLength = NI_MAXSERV;

		auto result = GetNameInfoW(address, length, nodeBuffer, nodeBufferLength, serviceBuffer, serviceBufferLength, NI_NUMERICHOST | NI_NUMERICSERV);
		if (result != 0)
		{
			Console::WriteLine(L"%s: getnameinfo failed: %d", __FILE__, result);
			return result;
		}

		if ((wcslen(nodeBuffer) + wcslen(serviceBuffer) + 1) > (unsigned)addressBufferLength)
		{
			return WSAEFAULT;
		}

		if (address->sa_family == AF_INET)
		{
			wprintf(addressBuffer, L"[%s]:%s", nodeBuffer, serviceBuffer);
		}
		else
		{
			if (address->sa_family == AF_INET6)
			{
				wprintf(addressBuffer, L"%s:%s", nodeBuffer, serviceBuffer);
			}
			else
			{
				addressBuffer[0] = L'\0';
			}
		}

		return NO_ERROR;
	}

	//    This routine resolves the specified address and returns a list of addrinfo
	//    structure containing SOCKADDR structures representing the resolved addresses.
	//    Note that if 'addr' is non-NULL, then getaddrinfo will resolve it whether
	//    it is a string listeral address or a hostname.
	static ADDRINFOW* ResolveAddress(wchar_t* address, const wchar_t* port, int family, int type, int protocol)
	{
		ADDRINFOW aiHints = {};
		aiHints.ai_flags = ((address) ? 0 : AI_PASSIVE);
		aiHints.ai_family = family;
		aiHints.ai_socktype = type;
		aiHints.ai_protocol = protocol;

		ADDRINFOW* paiResult = nullptr;

		auto result = GetAddrInfoW(address, port, &aiHints, &paiResult);
		if (result != 0)
		{
			Console::WriteLine(L"Invalid address %s, getaddrinfo failed: %d", address, result);
			return nullptr;
		}
		return paiResult;
	}
};

struct OverlappedBuffer;
struct OverlappedSocket;

struct OverlappedBuffer
{
	OVERLAPPED           ol;            // Overlapped structure

	SOCKET               sclient;       // Used for AcceptEx client socket
	HANDLE				 hFile;         // Open file handle for TransmitFile

	char*                buf;           // Buffer for send/recv/AcceptEx
	int                  buflen;        // Length of the buffer

	int                  operation;     // Type of operation submitted

#define OP_ACCEPT        0                   // AcceptEx (server)
#define OP_CONNECT       0                   // ConnectEx (client)
#define OP_READ          1                   // WSARecv (TCP) or WSARecvFrom (UDP)
#define OP_WRITE         2                   // WSASend (TCP) or WSASendTo (UDP)
#define OP_TRANSMIT      3                   // TransmitFile
// Additional message type operations would be defined here

	SOCKADDR_STORAGE     addr;          // Remote address (UDP)
	int                  addrlen;       // Remote address length

	ULONG				 IoOrder;       // Order in which this I/O was posted

	OverlappedSocket*    socket;         // SOCKET_OBJ that this I/O belongs to

	OverlappedBuffer*    prev;
	OverlappedBuffer*    next;
};
struct OverlappedSocket
{
	SOCKET               s;             // Socket handle for client connection

	int                  af;            // Address family of socket (AF_INET or AF_INET6)
	BOOL                 bConnected;
	BOOL				 bClosing;      // Indicates socket is closing

	volatile LONG        OutstandingOps;    // Number of outstanding overlapped ops
	volatile LONG        SendCount;         // Number of sends to perform on connection

	OverlappedBuffer**	 PendingAccepts;    // Array of pending AcceptEx calls (listening socket only)

	ULONG                LastSendIssued; // Last sequence number sent
	ULONG				 IoCountIssued;  // Next sequence number assigned to receives
	OverlappedBuffer*	 OutOfOrderSends;// List of send buffers that completed out of order

	// Pointers to Microsoft specific extensions (listening socket only)
	LPFN_ACCEPTEX        lpfnAcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs;

	// Pointer to Microsoft specific extensions (client connect socket)
	LPFN_CONNECTEX       lpfnConnectEx;
	LPFN_TRANSMITFILE    lpfnTransmitFile;

	OverlappedBuffer*    Repost;        // Send buffer to repost (used with rate limit)

	CRITICAL_SECTION     SockCritSec;   // Synchronize access to this SOCKET_OBJ

	OverlappedSocket*    prev;
	OverlappedSocket*    next;          // Used to chain SOCKET_OBJ together
};

OverlappedBuffer* NewBufferObject(OverlappedSocket* socket, int length)
{
	OverlappedBuffer* buffer = new OverlappedBuffer();

	buffer->buf = reinterpret_cast<char*>(Memory::CallocDefault(length, sizeof(BYTE)));
	if (buffer->buf == nullptr)
	{
		Console::Error(L"NewBufferObject: CallocDefault failed");
		Console::Pause();
		ExitProcess(-1);
	}

	buffer->buflen = length;
	buffer->addrlen = sizeof(buffer->addr);
	buffer->socket = socket;

	return buffer;
}
void DeleteBufferObject(OverlappedBuffer* buffer)
{
	Memory::Free(buffer->buf);
	delete buffer;
}

OverlappedSocket* NewSocketObject(SOCKET s, int family, int protocol, bool server)
{
	OverlappedSocket* socket = new OverlappedSocket();

	socket->s = s;
	socket->af = family;

	if (server)// For TCP we initialize the IO count to one since the AcceptEx is posted to receive data
		socket->IoCountIssued = (protocol == IPPROTO_TCP) ? 1 : 0;

	InitializeCriticalSection(&socket->SockCritSec);

	return socket;
}
void DeleteSocketObject(OverlappedSocket* socket)
{
	if (socket->OutstandingOps != 0)
	{
		return;
	}
	if (socket->s != INVALID_SOCKET)
	{
		closesocket(socket->s);
		socket->s = INVALID_SOCKET;
	}

	DeleteCriticalSection(&socket->SockCritSec);

	//HeapFree(GetProcessHeap(), 0, socket);
	delete socket;
}

void AddBufferObjectToList(OverlappedBuffer** bufferList, OverlappedBuffer* buffer)
{
	OverlappedBuffer* end = nullptr;
	OverlappedBuffer* ptr = nullptr;

	// Find the end of the list
	ptr = *bufferList;
	if (ptr)
	{
		while (ptr->next)
		{
			ptr = ptr->next;
		}
		end = ptr;
	}

	buffer->next = nullptr;
	buffer->prev = end;

	if (end == nullptr)
	{
		// List is empty
		*bufferList = buffer;
	}
	else
	{
		// Put new object at the end 
		end->next = buffer;
		buffer->prev = end;
	}
}
OverlappedBuffer* RemoveBufferObjectFromList(OverlappedBuffer** bufferList, OverlappedBuffer* buffer)
{
	if (*bufferList != nullptr)
	{
		if (buffer->prev)
			buffer->prev->next = buffer->next;
		if (buffer->next)
			buffer->next->prev = buffer->prev;

		if (*bufferList == buffer)
			(*bufferList) = buffer->next;
	}

	return buffer;
}

void AddSocketObjectToList(OverlappedSocket** socketList, OverlappedSocket* socket)
{
	OverlappedSocket* end = nullptr;
	OverlappedSocket* ptr = nullptr;

	// Find the end of the list
	ptr = *socketList;
	if (ptr)
	{
		while (ptr->next)
		{
			ptr = ptr->next;
		}
		end = ptr;
	}

	socket->next = nullptr;
	socket->prev = end;

	if (end == nullptr)
	{
		// List is empty
		*socketList = socket;
	}
	else
	{
		// Put new object at the end
		end->next = socket;
		socket->prev = end;
	}
}
OverlappedSocket* RemoveSocketObjectFromList(OverlappedSocket** socketList, OverlappedSocket* socket)
{
	// Make sure list isn't empty
	if (*socketList != nullptr)
	{
		// Fix up the next and prev pointers
		if (socket->prev)
			socket->prev->next = socket->next;
		if (socket->next)
			socket->next->prev = socket->prev;

		if (*socketList == socket)
			(*socketList) = socket->next;
	}
	return socket;
}
