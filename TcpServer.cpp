#include "TcpServer.h"
#include <iostream>
#include <unistd.h>
#include <memory>
#include "Log.h"

TcpServer::TcpServer(std::string ipAddr, uint16_t port)
    : events(initEventVectorSize), start_loop(true) , timeWheel(30 * TIMESLOT + 1)
{
    int ret = 0;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    ret = inet_pton(AF_INET, ipAddr.c_str(), &address.sin_addr);
    assert(ret > 0);
    address.sin_port = htons(port);
    m_epollfd = initialSocketfd();
    handleEventsPool.init();
}

int TcpServer::initialSocketfd()
{
    int ret;
    //创建套接字
    m_listenfd = sockets::createSocketfd(SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC);
    //int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    //assert(listenfd >= 0);
    //m_listenfd = listenfd;


    // 设置端口复用
    //一般不会立即关闭而经历TIME_WAIT的过程 后想继续重用该socket
    int iReuseaddr = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &iReuseaddr, sizeof(iReuseaddr));

    //绑定端口
    sockets::bind(m_listenfd, address);

    //somaxconn参数
    //定义了系统中每一个端口最大的监听队列的长度,这是个全局的参数,默认值为128
    sockets::listen(m_listenfd);
    
    //在 epoll_create() 的最初实现版本时， size参数的作用是创建epoll实例时候告诉内核需要使用多少个文件描述符。
    //内核会使用 size 的大小去申请对应的内存(如果在使用的时候超过了给定的size， 内核会申请更多的空间)。
    //现在，这个size参数不再使用了（内核会动态的申请需要的内存）。
    //但要注意的是，这个size必须要大于0，为了兼容旧版的linux 内核的代码。
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    // listenfd采用LT模式，否则accpet要使用循环
    sockets::addfd(epollfd, m_listenfd,false,true);
	
	//初始化timer

	sockets::socketpair(PF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, socketPair);
	//sockets::setnonblocking(socketPair[1]);
	sockets::addfd(epollfd, socketPair[0], false);

	sockets::addsig(SIGPIPE, SIG_IGN);
	sockets::addsig(SIGALRM, sockets::sig_handler, false);
	sockets::addsig(SIGTERM, sockets::sig_handler, false);



	alarm(TIMESLOT);
    return epollfd;
}

void TcpServer::newConnection(int socketfd)
{
    TcpConnectionPtr newConn(new HttpConnection(socketfd, m_epollfd));

    if (newConn->getSocketfd() < 0) {
        //std::cout << newConn->getSocketfd() << "\t" << " connection failed!" << std::endl;
        return;
    }
    else {
        //std::cout << newConn->getSocketfd() << "\t" << "success connection!" << std::endl;
    }
    int newSocketfd = newConn->getSocketfd();
    assert(userMap.count(newSocketfd) == 0);
    userMap[newSocketfd] = newConn;
    sockets::addfd(m_epollfd, newSocketfd);
    newConn->setCloseCallback(std::bind((void(TcpServer::*)(const TcpConnectionPtr))&TcpServer::removeConnection, this, std::placeholders::_1));

	//设置timer
	NodePtr node(new EventNode(newConn));
	timeWheel.push(node);
	newConn->setWeakNodePtr(node);
}

void TcpServer::removeConnection(int socketfd)
{
    assert(userMap.count(socketfd));
    userMap[socketfd]->closeSocket();
    userMap.erase(socketfd);
}

void TcpServer::removeConnection(const TcpConnectionPtr ptr)
{
    userMap.erase(ptr->getSocketfd());
}

void TcpServer::processEvents(int sockfd, const handleEventCallBack& events)
{
	//判断是否被timer断了链接。
	if (userMap.count(sockfd) == 0)		return;
	else {
		timeWheel.push(userMap[sockfd]->getNodePtr());
		userMap[sockfd]->setHandleEvents(events);
		handleEventsPool.append(WeakTcpConnectionPtr(userMap[sockfd]));
	}
}

void TcpServer::start()
{
    while (start_loop)
    {
        int eventsNum = epoll_wait(m_epollfd, &*events.begin(), static_cast<int>(events.size()), -1);
        if (eventsNum < 0 && errno != EINTR)
        {
			LOG_FATAL("epoll failed! errno: errno: %s",strerror(errno));
            break;
        }
        for (int i = 0; i < eventsNum; ++i)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == m_listenfd) {
                newConnection(sockfd);
            }
            //Tcp链接断开
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                removeConnection(sockfd);
            }
			//处理信号
			else if ((sockfd == socketPair[0]) && (events[i].events & EPOLLIN))
			{
				char signals[1024];
				int ret = recv(socketPair[0], signals, sizeof(signals), 0);
				//std::cout << "tick" << std::endl;
				timeWheel.tick();
				alarm(TIMESLOT);
			}
            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN) {
				processEvents(sockfd, handleRead);
            }
            //发送数据
            else if (events[i].events & EPOLLOUT) {
				processEvents(sockfd, handleWrite);
            }

        }
        if (static_cast<size_t>(eventsNum) == events.size())
        {
            events.resize(events.size() * 2);
        }
    }
}
