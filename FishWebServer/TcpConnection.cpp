#include "TcpConnection.h"

void TcpConnection::read()
{
	if (inputBuffer.readFd()){
		std::cout << "socket id: "<< socketfd << " received "<< inputBuffer.readableBytes() << " successfully!" << std::endl;
	}
	else {
		//若timeWheel调用closeSocket(),而此时工作线程正在read.则调用了两遍closeSocket();
		closeSocket();
	}
}

void TcpConnection::write(std::string& str)
{
	outputBuffer.append(str.c_str(), str.size());
}

void TcpConnection::closeSocket()
{
	handleClose();
	closecb(shared_from_this());
}

void TcpConnection::handleClose()
{
	epoll_ctl(epollfd, EPOLL_CTL_DEL, socketfd, 0);
	close(socketfd);
}
