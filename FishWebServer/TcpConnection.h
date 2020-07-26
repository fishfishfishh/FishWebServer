#pragma once
#include"Buffer.h"
#include <functional>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <memory>
#include "CallBacks.h"
#include "Log.h"
class TcpConnection : public std::enable_shared_from_this<TcpConnection>	
{
public:
	typedef std::function<void(const TcpConnectionPtr)> CloseCallback;
	TcpConnection(int fd, int epoll) : epollfd(epoll){
		socketfd = sockets::accept(fd, client_address);
		LOG_INFO("New connection have accepted, ip address: %s", inet_ntoa(client_address.sin_addr));
		inputBuffer.setSocketfd(socketfd);
		outputBuffer.setSocketfd(socketfd);
	}
	int getSocketfd() { return socketfd; }
	void setCloseCallback(const CloseCallback& cb) { closecb = cb; }
	//bool read() { return inputBuffer.readFd(); }
	void read();
	void write(std::string&);
	void closeSocket();
	//没有经过outputbuffer
	void send(std::string& msg) {
		::write(socketfd, msg.c_str(), msg.size());
	}
	//经过了outputbuffer
	void send() {
		::write(socketfd, outputBuffer.begin(), outputBuffer.readableBytes());
		std::cout << "socket id: " << socketfd << " have send " << outputBuffer.readableBytes() << " successfully!" << std::endl;
		outputBuffer.clear();
		sockets::update(EPOLLIN | EPOLLONESHOT | EPOLLRDHUP, getSocketfd(), epollfd);
	}
	void setHandleEvents(const handleEventCallBack& callback) {
		cb = callback; 

	}
	void setWeakNodePtr(const NodePtr& ptr) { weakTimerPtr = ptr; }
	NodePtr getNodePtr() { return weakTimerPtr.lock(); }
	void handleEvents() { cb(shared_from_this()); }
	virtual void processMessage() = 0;
	virtual ~TcpConnection(){ 
		LOG_INFO("Ip address: %s has close connection", inet_ntoa(client_address.sin_addr));
	}
protected:
	Buffer inputBuffer;
	Buffer outputBuffer;
	int epollfd;
	TcpConnection(){}
private:
	void handleClose();
	int socketfd;
	sockaddr_in client_address;
	CloseCallback closecb;
	handleEventCallBack cb;
	WeakNodePtr weakTimerPtr;
};
