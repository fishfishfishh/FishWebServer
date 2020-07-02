#pragma once
#include "TcpServer.h"
#include "ConnectionPool.h"

class HttpServer
{
public:
	HttpServer(std::string ipAddr, uint16_t port) :server(ipAddr, port) {
		server.sethandleRead(std::bind(&HttpServer::handleRead, this, std::placeholders::_1));
		server.sethandleWrite(std::bind(&HttpServer::handleWrite, this, std::placeholders::_1));
	}
	void start() { server.start(); }
	void handleRead(TcpConnectionPtr);
	void handleWrite(TcpConnectionPtr);
private:
	void signUp(std::string&, HttpConnection*);
	void signIn(std::string&, HttpConnection*);
	void changePassword(std::string&, HttpConnection*);
	void responseInfo(const std::string&, const std::string&, HttpConnection*);
	std::string& mySqlCommand(std::string&);
	TcpServer server;
};

