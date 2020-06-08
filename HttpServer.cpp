#include "HttpServer.h"

void HttpServer::handleRead(TcpConnectionPtr ptr)
{
	ptr->read();
	ptr->processMessage();
}

void HttpServer::handleWrite(TcpConnectionPtr ptr)
{
	ptr->send();
}
