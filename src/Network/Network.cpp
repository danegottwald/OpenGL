#include "Network.h"

#include <WinSock2.h>

Network* Network::s_Instance = nullptr;

Network& Network::GetInstance()
{
    if (s_Instance)
        return *s_Instance;

    SOCKET socket;
    SOCKADDR_IN socketAddr;
    int addrlen = sizeof(socketAddr);

    //socket = accept(true, (SOCKADDR*)&socketAddr, &addrlen);

    return *s_Instance;
}

