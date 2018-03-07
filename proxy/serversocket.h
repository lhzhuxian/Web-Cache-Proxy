#pragma once
#include "socket.h"

class ServerSocket : public Socket {

public:
    ServerSocket();

    Socket *accept();


    bool listen();
private:
    int _backLog; // backlog
};