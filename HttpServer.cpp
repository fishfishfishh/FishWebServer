#include "HttpServer.h"
#include <boost/implicit_cast.hpp>
#include <sys/mman.h>
void HttpServer::handleRead(TcpConnectionPtr ptr)
{
	ptr->read();
	ptr->processMessage();
}

void HttpServer::handleWrite(TcpConnectionPtr ptr)
{
	auto upCastptr = static_cast<HttpConnection*>(ptr.get());
	switch (upCastptr->getretStatus())
	{
	case HttpConnection::NO_REQUEST:
	{
		//GET
		std::string a("Http/1.1 200 OK\r\n");
		std::string b = "Connection: keep-alive\r\n";
		std::cout << upCastptr->getRequestPath() << std::endl;
		if (upCastptr->getRequestPath() == "/") {
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
			std::string response = a + b + c + d + blank + content;
			upCastptr->write(response);
		}
		upCastptr->send();
		upCastptr->clear();
		break;
	}
	case HttpConnection::GET_REQUEST:
		//POST
		upCastptr->send("Http/1.1 401 Unauthorized\r\n\r\n");
		break;
	case HttpConnection::BAD_REQUEST:
		//协议错了
		upCastptr->send("Http/1.1 400 Bad Request\r\n\r\n");
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
	upCastptr->clear();
}
