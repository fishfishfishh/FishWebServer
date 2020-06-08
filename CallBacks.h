#pragma once
#include <functional>
#include <memory>
#include <arpa/inet.h>
#include <boost/circular_buffer.hpp>
#include <unordered_set>
#include <fcntl.h>

class TcpConnection;
class EventNode;

namespace sockets{
	int setnonblocking(int fd);
	void update(int operation, int socketfd,int epollfd);
	int createSocketfd(int type);
	int accept(int fd,sockaddr_in&);
	void bind(int socketfd, sockaddr_in&);
	void listen(int socketfd);
    //将内核事件表注册读事件，ET = 0/LT = 1模式，选择开启EPOLLONESHOT
	void addfd(int epollfd, int fd, bool one_shot = false, bool flag = false);
	void socketpair(int domain, int type, int protocol, int ptr[2]);

	void addsig(int sig, void(handler)(int), bool restart = true);
	void sig_handler(int sig);
}


typedef	std::shared_ptr<EventNode> NodePtr;
typedef std::weak_ptr<EventNode> WeakNodePtr;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void(TcpConnectionPtr)> handleEventCallBack;
typedef std::weak_ptr<TcpConnection>	WeakTcpConnectionPtr;
typedef std::unordered_set<NodePtr> Bucket;
typedef boost::circular_buffer<Bucket> timeWheel;
