#include "pch.h"
#include "AsyncNetworkSystem.h"

void ClayEngine::AsyncListenFunctor::operator()(FUTURE future, AsyncNetworkSystem* ans)
{
	while (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout)
    {
        if (!ans) continue;

        AcceptSocketData* opData = nullptr;
        bool isAcceptDataReady = false;

        if (!isAcceptDataReady)
        {
            // Create a AcceptSocketData
            if (!opData)
                opData = ans->MakeAcceptSocketData(64);
            else
                opData->Reset();

            // Create a new socket
            opData->Socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
            if (opData->Socket == INVALID_SOCKET)
            {
                throw std::runtime_error("Failed to create a new socket");
            }

            // Associate the socket with the completion port
            if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(opData->Socket), ans->GetListenQueue(), reinterpret_cast<ULONG_PTR>(opData), 0) == NULL)
            {
                closesocket(opData->Socket);
                throw std::runtime_error("Failed to associate the socket with the completion port");
            }

            // Run AcceptEx to start accepting connections asynchronously
            DWORD bytesReceived = 0;
            BOOL result = ans->GetAcceptExFunction()(
                ans->GetListenSocket(),
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
        if (GetQueuedCompletionStatus(ans->GetListenQueue(), &bytesTransferred, &completionKey, &overlapped, INFINITE))
        {
            // Cast the overlapped structure back to AcceptSocketData to retrieve connection details
            auto completedOpData = reinterpret_cast<AcceptSocketData*>(overlapped);

            if (opData != completedOpData) throw;

            // Retrieve local and remote addresses (assumes buffers for AcceptEx are set correctly)
            SOCKADDR* localAddr = nullptr;
            SOCKADDR* remoteAddr = nullptr;
            int localAddrLen = 0;
            int remoteAddrLen = 0;
            GetAcceptExSockaddrs(
                completedOpData->Buffer,
                completedOpData->BufferLength,
                sizeof(SOCKADDR_IN) + 16,
                sizeof(SOCKADDR_IN) + 16,
                &localAddr,
                &localAddrLen,
                &remoteAddr,
                &remoteAddrLen
            );

            // Set socket options as needed
            //setsockopt(completedOpData->Socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&ans->GetListenSocket(), sizeof(ans->GetListenSocket()));

            // Create ClientConnectionData to handle the newly accepted client
            auto clientData = ans->MakeClientConnectionData(completedOpData->Socket);

            isAcceptDataReady = false;
        }
        else
        {
            continue;
        }
    }
}
