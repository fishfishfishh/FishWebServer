#include "HttpServer.h"
#include <boost/implicit_cast.hpp>
#include "CallBacks.h"
void HttpServer::handleRead(TcpConnectionPtr ptr)
{
	ptr->read();
	ptr->processMessage();
}
const char* blank = "\r\n";
void HttpServer::handleWrite(TcpConnectionPtr ptr)
{
	auto upCastptr = dynamic_cast<HttpConnection*>(ptr.get());
	char buf[32];
	snprintf(buf, sizeof buf, "HTTP/1.1 %d ", upCastptr->statusCode);
	upCastptr->write(buf);
	upCastptr->write("OK");
	upCastptr->write("\r\n");
	upCastptr->write("Connection: ");
	upCastptr->write(upCastptr->getHeaderMessage("Connection"));
	upCastptr->write("\r\n");
	//std::string b = "Connection: " + upCastptr->getHeaderMessage("Connection")+blank;
	//std::cout << "upCastptr->getretStatus() is " << upCastptr->getretStatus() << std::endl;
	//std::cout << upCastptr->getRequestPath() << std::endl;
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
			upCastptr->write("Content-Length: ");
			upCastptr->write(std::to_string(content.size()));
			upCastptr->write("\r\n");
			upCastptr->write("Content-Type: ");
			upCastptr->write("text/html");
			upCastptr->write("\r\n");
			upCastptr->write(blank);
			upCastptr->write(content);

		}
		else {
			std::string content;
			char tempBuf[200];
			int fd = open(upCastptr->getRequestPath().c_str(), O_RDONLY);
			int ret;
			while ((ret = read(fd, tempBuf, 200)) > 0) {
				content += std::string(tempBuf, tempBuf + ret);
			}
			upCastptr->write("Content-Length: ");
			upCastptr->write(std::to_string(content.size()));
			upCastptr->write("\r\n");
			upCastptr->write("Content-Type: ");
			upCastptr->write(upCastptr->getContentType());
			upCastptr->write("\r\n");
			upCastptr->write(blank);
			upCastptr->write(content);
		}
		break;
	}
	case HttpConnection::GET_REQUEST:
	{
		//POST
		std::string& content = upCastptr->getRequestBody();

		switch (content[1])
		{
			//sign in
		case 'i':
			signIn(content, upCastptr);
			break;
			//sign up
		case 'u':
			signUp(content, upCastptr);
			break;
			//change password
		case 'p':
			changePassword(content);
			break;
		default:
			//never came here
			break;
		}
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
		//这里在http长连接中可能会出问题，之后再改
		upCastptr->clearAll();
	}
}

void HttpServer::signUp(std::string &word, HttpConnection* upCastptr)
{

}

void HttpServer::signIn(std::string &word, HttpConnection* upCastptr)
{
	auto iter = word.find('&');
	auto userIter = std::find(word.begin(), word.begin() + iter, '=');
	std::string username = std::string(userIter + 1, word.begin() + iter);
	auto passwordIter = std::find(word.begin() + iter + 1, word.end(), '=');
	std::string password = std::string(passwordIter + 1, word.end());
	auto conn = ConnectionPool::getInstance().take();
	redisReply* r = (redisReply*)redisCommand(conn->context, "GET %s", username.c_str());

	if (r->str == nullptr or std::string(r->str) != password) {
		std::string content;
		char tempBuf[200];
		int fd = open("/home/pi/projects/HttpServerForLinux/html/SignInError.html", O_RDONLY);
		int ret;
		while ((ret = read(fd, tempBuf, 200)) > 0) {
			content += std::string(tempBuf, tempBuf + ret);
		}
		upCastptr->write("Content-Length: ");
		upCastptr->write(std::to_string(content.size()));
		upCastptr->write("\r\n");
		upCastptr->write("Content-Type: ");
		upCastptr->write("text/html");
		upCastptr->write("\r\n");
		upCastptr->write(blank);
		upCastptr->write(content);

	}
	else {
		std::string content;
		char tempBuf[200];
		int fd = open("/home/pi/projects/HttpServerForLinux/html/HTMLPage.html", O_RDONLY);
		int ret;
		while ((ret = read(fd, tempBuf, 200)) > 0) {
			content += std::string(tempBuf, tempBuf + ret);
		}
		upCastptr->write("Content-Length: ");
		upCastptr->write(std::to_string(content.size()));
		upCastptr->write("\r\n");
		upCastptr->write("Content-Type: ");
		upCastptr->write("text/html");
		upCastptr->write("\r\n");
		upCastptr->write(blank);
		upCastptr->write(content);
	}
}

void HttpServer::changePassword(std::string &word)
{
	std::cout << "changePassword" << std::endl;
}
