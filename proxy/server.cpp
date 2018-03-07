#include "server.h"
#include <iostream>
#include <sstream> 
#include <time.h> 
#include "log.h"
#include <stdlib.h>
#include <map>

extern map<string,vector<char> > g_mapCache;
static char* getNowTime(){
  time_t timep;
  time(&timep);
  return asctime(gmtime(&timep));
}

static string getFirstLine(char* buff){
  string s(buff);
  int position = s.find("\r\n");
  return s.substr(0,position);
}

static string getFirstLine(vector<char> buff) {
  string s(buff.begin(), buff.end());
  int position = s.find("\r\n");
  return s.substr(0,position);
}

static string getHost(char* buff){
  string s(buff);
  string str = s.substr(s.find("\r\n")+2);
  str = str.substr(0,str.find("\r\n"));
  stringstream ss;
  ss<<str;
  string tmp;
  string host;
  ss>>tmp;
  ss>>host;
  return host;
}

CWorkThread::CWorkThread(Socket* pClientSocket,int id)
{
  m_pClientSocket = pClientSocket;
  m_nId = id;
}

CWorkThread::~CWorkThread()
{
  if (m_pClientSocket != NULL)
    {
      delete m_pClientSocket;
      m_pClientSocket = NULL;
    }	
}

void CWorkThread::Run()
{
  if(m_pClientSocket == NULL)
    return;
  char recv_buf[10000] = {0};  
  int recv_len = 0;
  while( (recv_len = m_pClientSocket->read(recv_buf, sizeof(recv_buf))) > 0 )  
    {  
      //printf("recv_buf: %s\n", recv_buf);
      
      string firstLine = getFirstLine(recv_buf);
      
      LOG_INFO("%u: \"%s\" from %s @ %s",m_nId,firstLine.c_str(),m_pClientSocket->getAddr().c_str(),getNowTime());
      printf("%u: \"%s\" from %s @ %s\n",m_nId,firstLine.c_str(),m_pClientSocket->getAddr().c_str(),getNowTime());
      stringstream ss;
      ss<<firstLine;
      string method;
      ss>>method;
      string url;
      ss>>url;
      if(method.compare("GET") == 0)
	{
	  map<string, vector<char> >::iterator it;
	  if((it = g_mapCache.find(url)) == g_mapCache.end())
	    {
	      LOG_INFO("%u: not in cache",m_nId);
	      printf("%u: not in cache",m_nId);
	    }
	  else
	    {
	      LOG_INFO("%u: in cache,valid",m_nId);
	      printf("%u: in cache,valid",m_nId);
	      m_pClientSocket->write(&((*it).second), (*it).second.size());
	      continue;
	    }
	}
      
      string host = getHost(recv_buf);
      int port = 80;
      int idx = host.find(":");
      if(idx != -1){
	port = atoi(host.substr(idx+1).c_str());
				host = host.substr(0,idx);
      }
      
      LOG_INFO("%u: Requesting \"%s\" from %s",m_nId,url.c_str(),host.c_str());
      printf("%u: Requesting \"%s\" from %s\n",m_nId,url.c_str(),host.c_str());

      Socket s;
      s.setAddress(host.c_str(),port);
      s.connect();
      s.write(recv_buf,recv_len);
      char rep_buff[10000] = {0};
      //vector<char> rep_buff;
      int replen = s.read(rep_buff,sizeof(rep_buff));
      printf("content:\n %s", rep_buff);
      m_pClientSocket->write(rep_buff,replen);
      //vector<char> temp;
      //temp.insert(temp.end(), rep_buff. rep_buff + replen);
      //g_mapCache[url] = temp;

      LOG_INFO("%u: Received \"%s\" from %s",m_nId,getFirstLine(rep_buff).c_str(),host.c_str());
      printf("%u: Received \"%s\" from %s\n",m_nId,getFirstLine(rep_buff).c_str(),host.c_str());
      LOG_INFO("%u: Responding \"%s\"",m_nId, getFirstLine(rep_buff).c_str());
      printf("%u: Responding \"%s\"\n",m_nId, getFirstLine(rep_buff).c_str());
    }
  if (m_pClientSocket != NULL)
    {
      delete m_pClientSocket;
      m_pClientSocket = NULL;
    }	
}

CServer::CServer(int port)
{
  m_nListenPort = port;
  m_pServerSocket = NULL;
  m_nSerialNo = 0;
}

CServer::~CServer()
{
	this->stopServer();
}

void CServer::stopServer()
{
	if(m_pServerSocket != NULL)
		delete m_pServerSocket;
	m_pServerSocket = NULL;
	for(int i = 0;i < m_vecThread.size(); i++)
	{
		Thread* pThread = m_vecThread[i];
		pThread->Exit();
		delete pThread;
	}
	m_vecThread.clear();
}

void CServer::run()
{
  if(m_pServerSocket != NULL)
    return;
  m_pServerSocket = new ServerSocket();
  m_pServerSocket->setAddress(NULL,m_nListenPort);
  if(m_pServerSocket->listen())
    printf("begin linsten %d\n",m_nListenPort);
  else
    {
      printf("linsten %d fail\n",m_nListenPort);
      return;
    }

  while(1)
    {
      Socket* pClientSocket = m_pServerSocket->accept();
      if(pClientSocket == NULL)
	break;
      pClientSocket->setSoBlocking(true);
      printf("client from %s\n",pClientSocket->getAddr().c_str());
      CWorkThread* pWorkThread = new CWorkThread(pClientSocket,makeID());
      pWorkThread->Start();
      m_vecThread.push_back(pWorkThread);
    }
}
