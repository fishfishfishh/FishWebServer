#pragma once
#define ET
#include <string>
#include <memory>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <assert.h>
#include <string.h>
#include <vector>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <unordered_map>
#include "HttpConnection.h"
#include "CallBacks.h"
#include "ThreadPool.h"
#include <signal.h>
#include "TimeWheel.h"


const int TIMESLOT = 1;             //最小超时单位


class HttpConnection;
class TcpServer
{
public:
    typedef std::vector<epoll_event>  EventVector;
    typedef std::map<int, TcpConnectionPtr> TcpMap;
	typedef std::array<TcpConnectionPtr, 65536> TcpArray;
	TcpServer(std::string ipAddr, uint16_t port);
    void sethandleRead(const handleEventCallBack& cb) { handleRead = cb; }
    void sethandleWrite(const handleEventCallBack& cb) { handleWrite = cb; }
	void start();
	~TcpServer()
	{
		close(m_listenfd);
		close(socketPair[0]);
		close(socketPair[1]);
	}
private:
	int initialSocketfd();
	
private:
    handleEventCallBack handleRead;
    handleEventCallBack handleWrite;
    void newConnection(int socketfd);
    void removeConnection(int socketfd);
    void removeConnection(const TcpConnectionPtr);
	void processEvents(int, const handleEventCallBack&);
	static const uint32_t initEventVectorSize = 100;
	sockaddr_in address;
	EventVector events;
	int m_epollfd;
    int m_listenfd;
    int start_loop;
    //TcpMap userMap;
	TcpArray userArray;
    ThreadPool handleEventsPool;

	//timer
	int socketPair[2];
	TimeWheel timeWheel;
};
