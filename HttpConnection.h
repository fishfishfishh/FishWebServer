#pragma once
#include "TcpConnection.h"
class HttpConnection :public TcpConnection
{
public:
	enum HTTP_CODE
	{
		NO_REQUEST,
		GET_REQUEST,
		BAD_REQUEST,
		NO_RESOURCE,
		FORBIDDEN_REQUEST,
		FILE_REQUEST,
		INTERNAL_ERROR,
		CLOSED_CONNECTION
	};
	enum CHECK_STATE
	{
		CHECK_STATE_HEADER = 0 ,
		CHECK_STATE_REQUESTLINE,
		CHECK_STATE_CONTENT
	};
	enum LINE_STATUS
	{
		LINE_OK = 0,
		LINE_BAD,
		LINE_OPEN
	};
	enum METHOD
	{
		GET = 0,
		POST,
		HEAD,
		PUT,
		DELETE,
		TRACE,
		OPTIONS,
		CONNECT,
		PATH
	};
	HttpConnection(int fd, int epoll) 
		:TcpConnection(fd,epoll), processLength(0),
		checkState(CHECK_STATE_REQUESTLINE),linger(true),host(nullptr),contentLength(0){
	}
	//进入此函数，应是读事件
	void processMessage() {
		//inputBuffer.retrieve(processLength);
		//test
		//if (parseLine() == LINE_OK) {
		//	if(parseHeader() == NO_REQUEST)	std::cout << "fuck you!" << std::endl;
		//	else	std::cout << "love you!" << std::endl;
		//}
		//else {
		//	std::cout << "fuck! bad process!" << std::endl;
		//}
		//此时不应该修改读事件
		//if (httpMessageDecode() == NO_REQUEST)		return;			//消息不全，接着读
		/*if (!httpMessageWrite())	closeSocket();*/
		std::string&& msg = inputBuffer.getMessageAsString();
		write(msg);
		sockets::update(EPOLLOUT | EPOLLONESHOT | EPOLLRDHUP, getSocketfd(), epollfd);
	}
	
private:
	int processLength;
	int contentLength;
	CHECK_STATE checkState;
	HTTP_CODE  httpMessageDecode();
	bool httpMessageWrite();				//此函数将结果copy to outputBuffer
	bool linger;
	char* host;

	LINE_STATUS parseLine();
	//请求模式
	METHOD requestMethod;
	//是否启用cgi
	int cgi;
	HTTP_CODE parseRequestLine();
	HTTP_CODE parseHeader();
	HTTP_CODE doRequest();
	HTTP_CODE parseContent();
};

