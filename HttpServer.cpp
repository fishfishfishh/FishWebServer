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
			responseInfo("/home/pi/projects/HttpServerForLinux/html/SignIn.html", "text/html", upCastptr);
		}
		else {
			responseInfo(upCastptr->getRequestPath().c_str(), upCastptr->getContentType(), upCastptr);
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
			changePassword(content, upCastptr);
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
		upCastptr->clearAll();
	}
}

void HttpServer::signUp(std::string &word, HttpConnection* upCastptr)
{
	auto iterFirst = word.find_first_of('&');
	auto iterSecond = word.find_last_of('&');
	auto userIter = std::find(word.begin(), word.begin() + iterFirst, '=');
	std::string username = std::string(userIter + 1, word.begin() + iterFirst);

	auto conn = ConnectionPool::getInstance().take();
	redisReply* r = (redisReply*)redisCommand(conn->context, "GET %s", username.c_str());
	if (r->str == nullptr) {
		auto passwordIter = std::find(word.begin() + iterFirst + 1, word.begin() + iterSecond, '=');
		std::string password = std::string(passwordIter + 1, word.begin() + iterSecond);
		redisReply* reply = (redisReply*)redisCommand(conn->context, "SET %s %s", username.c_str(), password.c_str());
		freeReplyObject(reply);
		responseInfo("/home/pi/projects/HttpServerForLinux/html/SignIn.html", "text/html", upCastptr);
	}
	else {
		responseInfo("/home/pi/projects/HttpServerForLinux/html/SignUp.html", "text/html", upCastptr);
	}


	freeReplyObject(r);
	ConnectionPool::getInstance().push(conn);
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
		responseInfo("/home/pi/projects/HttpServerForLinux/html/SignInError.html", "text/html", upCastptr);
	}
	else {
		responseInfo("/home/pi/projects/HttpServerForLinux/html/HTMLPage.html", "text/html", upCastptr);
	}
	freeReplyObject(r);
	ConnectionPool::getInstance().push(conn);
}

void HttpServer::changePassword(std::string &word, HttpConnection* upCastptr)
{
	auto iterFirst = word.find_first_of('&');
	auto iterSecond = word.find_last_of('&');
	auto userIter = std::find(word.begin(), word.begin() + iterFirst, '=');
	std::string username = std::string(userIter + 1, word.begin() + iterFirst);

	auto oldpasswordIter = std::find(word.begin() + iterFirst + 1, word.begin() + iterSecond, '=');
	std::string oldpassword = std::string(oldpasswordIter + 1, word.begin() + iterSecond);

	auto conn = ConnectionPool::getInstance().take();
	redisReply* r = (redisReply*)redisCommand(conn->context, "GET %s", username.c_str());

	if (r->str == nullptr or std::string(r->str) != oldpassword) {
		responseInfo("/home/pi/projects/HttpServerForLinux/html/ChangePasswordError.html", "text/html", upCastptr);
	}
	else {
		auto newpasswordIter = std::find(word.begin() + iterSecond + 1, word.end(), '=');
		std::string newpassword = std::string(newpasswordIter + 1, word.end());
		redisReply* reply = (redisReply*)redisCommand(conn->context, "SET %s %s", username.c_str(), newpassword.c_str());
		freeReplyObject(reply);
		responseInfo("/home/pi/projects/HttpServerForLinux/html/SignIn.html", "text/html", upCastptr);
	}
	freeReplyObject(r);
	ConnectionPool::getInstance().push(conn);
}

void HttpServer::responseInfo(const std::string & path, const  std::string &type, HttpConnection* upCastptr)
{
	std::string content;
	char tempBuf[200];
	int fd = open(path.c_str(), O_RDONLY);
	int ret;
	while ((ret = read(fd, tempBuf, 200)) > 0) {
		content += std::string(tempBuf, tempBuf + ret);
	}
	upCastptr->write("Content-Length: ");
	upCastptr->write(std::to_string(content.size()));
	upCastptr->write("\r\n");
	upCastptr->write("Content-Type: ");
	upCastptr->write(type);
	upCastptr->write("\r\n");
	upCastptr->write(blank);
	upCastptr->write(content);
}
