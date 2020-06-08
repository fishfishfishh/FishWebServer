#include "CallBacks.h"
#include "sys/epoll.h"
#include <assert.h>
#include <iostream>
#include <string.h>
#include <signal.h>
#include "Log.h"

static int masterSocketfd = 0;
static int slaveSocketfd = 0;

int sockets::setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

void sockets::update(int operation, int socketfd,int epollfd)
{
	epoll_event event;
	event.data.fd = socketfd;
	event.events = operation;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, socketfd, &event);
}

int sockets::createSocketfd(int type)
{
	int socketfd = ::socket(PF_INET,type, IPPROTO_IP);
	assert(socketfd > 0);
	LOG_DEBUG("events::create socketfd,id is : %d", socketfd);
	return socketfd;
}

int sockets::accept(int fd, sockaddr_in& addr)
{
	socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));
	int connfd = accept4(fd, (sockaddr*)&addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if (connfd < 0) {
		LOG_FATAL("events::accept error! errno %s", strerror(errno));
	}
	return connfd;
}

void sockets::bind(int listenfd, sockaddr_in& addr)
{
	int ret = ::bind(listenfd, (struct sockaddr*) & addr, sizeof(addr));
	if (ret < 0) {
		LOG_FATAL("events::bind error! errno %s", strerror(errno));
	}
}

void sockets::listen(int socketfd)
{
	int ret = ::listen(socketfd, SOMAXCONN);
	if (ret < 0) {
		LOG_FATAL("events::listen error! errno %s", strerror(errno));
	}
}

//将内核事件表注册读事件，ET = 0/LT = 1模式，选择开启EPOLLONESHOT
void sockets::addfd(int epollfd, int fd, bool one_shot,bool flag)
{
	epoll_event event;
	event.data.fd = fd;
	event.events =  flag == 0 ? EPOLLIN | EPOLLET | EPOLLRDHUP : EPOLLIN | EPOLLRDHUP;

	if (one_shot)
		event.events |= EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}

void sockets::socketpair(int domain, int type, int protocol, int ptr[2])
{
	int ret = ::socketpair(domain, type, protocol, ptr);
	if (ret == -1)
		LOG_ERROR("socketpair create failed ! errno %s", strerror(errno));
	::masterSocketfd = ptr[0];
	::slaveSocketfd = ptr[1];
}

void sockets::addsig(int sig, void(handler)(int), bool restart)
{
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = handler;
	if (restart)
		sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, NULL) != -1);
}

void sockets::sig_handler(int sig)
{
	//为保证函数的可重入性，保留原来的errno
	int save_errno = errno;
	int msg = sig;
	send(::slaveSocketfd, (char *)&msg, 1, 0);
	errno = save_errno;
}

