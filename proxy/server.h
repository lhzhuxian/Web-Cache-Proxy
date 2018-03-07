#pragma once
#include "serversocket.h"
#include "thread.h"
#include <vector>
#include <memory>
using namespace std;

class CWorkThread : public Thread
{
public:
	CWorkThread(Socket* pClientSocket,int id);

	~CWorkThread();

	void Run();
private:
	Socket* m_pClientSocket;
	int m_nId;
};

class CServer
{
public:
	CServer(){}
	CServer(int port);
	~CServer();

	void run();

	void stopServer();
private:
	int m_nListenPort;
	ServerSocket* m_pServerSocket;
	vector<Thread*> m_vecThread;

	unsigned int makeID()
	{
		++m_nSerialNo;
		if ( m_nSerialNo == 0 )
			m_nSerialNo = 1;
		return m_nSerialNo;
	}

	unsigned int m_nSerialNo;
};
typedef CServer* CServerPtr;