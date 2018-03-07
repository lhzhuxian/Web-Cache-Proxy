#pragma once
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>
#include <list>
#include <queue>
#include <vector>
#include <string>

class Socket {

public:
    Socket();


    ~Socket();

    bool setAddress (const char *address, const int port);

    bool connect();

 
    void close();


    void shutdown();


    void setUp(int socketHandle, struct sockaddr *hostAddress);


    int getSocketHandle();


    int write(const void *data, int len);


    int read(void *data, int len);


    bool setKeepAlive(bool on) {
        return setIntOption(SO_KEEPALIVE, on ? 1 : 0);
    }


    bool setReuseAddress(bool on) {
        return setIntOption(SO_REUSEADDR, on ? 1 : 0);
    }


    bool setSoLinger (bool doLinger, int seconds);


    bool setTcpNoDelay(bool noDelay);


    bool setTcpQuickAck(bool quickAck);


    bool setIntOption(int option, int value);


    bool setTimeOption(int option, int milliseconds);


    bool setSoBlocking(bool on);


    bool checkSocketHandle();


    int getSoError();


    std::string getAddr();


    uint64_t getId();
    uint64_t getPeerId();

    int getLocalPort();

    static int getLastError() {
        return errno;
    }

protected:
    struct sockaddr_in  _address; 
    int _socketHandle;  
};

