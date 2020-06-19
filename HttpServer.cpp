#include "HttpServer.h"
#include <boost/implicit_cast.hpp>
#include <sys/mman.h>
void HttpServer::handleRead(TcpConnectionPtr ptr)
{
	ptr->read();
	ptr->processMessage();
}
static std::string blank = "\r\n";
void HttpServer::handleWrite(TcpConnectionPtr ptr)
{
	auto upCastptr = dynamic_cast<HttpConnection*>(ptr.get());
	std::string a("Http/1.1 200 OK\r\n");
	std::string b = "Connection: " + upCastptr->getHeaderMessage("Connection")+blank;
	//std::cout << "upCastptr->getretStatus() is " << upCastptr->getretStatus() << std::endl;
	switch (upCastptr->getretStatus())
	{
	case HttpConnection::NO_REQUEST:
	{
		//GET
		if (upCastptr->getRequestPath() == "/") {
			std::string content;
			char tempBuf[200];
			int fd = open("/home/pi/projects/HttpServerForLinux/html/SignIn.html", O_RDONLY);
			int ret;
			while ((ret = read(fd, tempBuf, 200)) > 0) {
				content += std::string(tempBuf, tempBuf + ret);
			}
			std::string c = "Content-length: " + std::to_string(content.size()) + blank;
			std::string d = "Content-Type: text/html\r\n";
			std::string response = a + b + c + d + blank + content;
			upCastptr->write(response);
		}
		else {
			std::string content;
			char tempBuf[200];
			int fd = open(upCastptr->getRequestPath().c_str(), O_RDONLY);
			int ret;
			while ((ret = read(fd, tempBuf, 200)) > 0) {
				content += std::string(tempBuf, tempBuf + ret);
			}
			std::string c = "Content-length: " + std::to_string(content.size()) + "\r\n";
			std::string d = "Content-Type: image/jpeg\r\n";
			std::string blank = "\r\n";
			//std::string response = a + b + c + d + blank + content;
			//upCastptr->write(response);
		}
		break;
	}
	case HttpConnection::GET_REQUEST:
	{
		//POST
		std::string content;
		char tempBuf[200];
		int fd = open("/home/pi/projects/HttpServerForLinux/html/HTMLPage.html", O_RDONLY);
		int ret;
		while ((ret = read(fd, tempBuf, 200)) > 0) {
			content += std::string(tempBuf, tempBuf + ret);
		}
		std::string c = "Content-length: " + std::to_string(content.size()) + "\r\n";
		std::string d = "Content-Type: text/html\r\n";
		std::string blank = "\r\n";
		//std::string response = a + b + c + d + blank + content;
		//upCastptr->write(response);
		break;
	}
	case HttpConnection::BAD_REQUEST:
		//协议错了
		
		break;
	default:
		//NO_RESOURCE,
		//FORBIDDEN_REQUEST,
		//FILE_REQUEST,
		//INTERNAL_ERROR,
		//CLOSED_CONNECTION
		upCastptr->send("Http/1.1 404 Not Found\r\n\r\n");
		break;
	}
	upCastptr->send();
	
	if (upCastptr->getHeaderMessage("Connection") == "close") {
		upCastptr->closeSocket();
	}
	else {
		sockets::update(EPOLLIN | EPOLLONESHOT | EPOLLRDHUP, upCastptr->getSocketfd(), upCastptr->getepollfd());
		upCastptr->clearAll();
	}	
}
