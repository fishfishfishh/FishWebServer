#pragma once
#include "TcpConnection.h"
#include <map>
class HttpConnection : public TcpConnection
{
	typedef std::string::iterator strIter;
public:
	enum HttpVersion
	{
		Unknown, Http10, Http11
	};
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
		CHECK_STATE_HEADER = 0,
		CHECK_STATE_REQUESTLINE,
		CHECK_STATE_CONTENT
	};
	enum LINE_STATUS
	{
		LINE_OK = 0,
		LINE_BAD,
		LINE_OPEN,
		LINE_END
	};
	enum CONNECTION_STATE {
		KEEPALIVE = 0,
		CLOSE,
		UNKNOWN
	};
	enum METHOD
	{
		INVALID = 0,
		GET,
		POST,
		HEAD,
		PUT,
		DELETE,
		TRACE,
		OPTIONS,
		CONNECT,
	};
	HttpConnection(int fd, int epoll)
		:processLength(0), checkState(CHECK_STATE_REQUESTLINE),
		lineStatus(LINE_OK),linger(true), contentLength(0)
		, requestMethod(INVALID), version(Unknown), connectionState(UNKNOWN)
		,TcpConnection(fd,epoll){
		start = message.begin();
		end = message.end();
	}
	//进入此函数，应是读事件
	void processMessage() {
		message += inputBuffer.getMessageAsString();
		if (httpMessageDecode() == NO_REQUEST) {
			if (lineStatus == LINE_BAD) {
				printf("lineStatus bad\n");
			}
			else if (lineStatus == LINE_END) {
				printf("lineStatus OK\n");
			}
		}
		else{
			printf("HTTP_CODE BAD_REQUEST\n");
		}
		printf("%s", message.c_str());
	}

private:
	METHOD getMethod(std::string::iterator begin, std::string::iterator end) {
		assert(requestMethod == INVALID);
		std::string method(begin, end);
		if (method == "GET")	 requestMethod = GET;
		else if (method == "POST")	requestMethod = POST;
		else if (method == "HEAD")	requestMethod = HEAD;
		else if (method == "PUT")	requestMethod = PUT;
		else if (method == "DELETE")	requestMethod = DELETE;
		else if (method == "CONNECT")	requestMethod = CONNECT;
		else if (method == "TRACE")		requestMethod = TRACE;
		else if (method == "OPTIONS")	requestMethod = OPTIONS;
		else	requestMethod = INVALID;

		return requestMethod;
	}
	bool setHearMethod(std::string& str, strIter begin, strIter end);
	std::string message;
	int processLength;

	
	LINE_STATUS lineStatus;
	CHECK_STATE checkState;
	HTTP_CODE  httpMessageDecode();
	//bool httpMessageWrite();				//此函数将结果copy to outputBuffer
	bool linger;
	std::string::iterator start;
	std::string::iterator end;

	LINE_STATUS parseLine();
	//请求模式
	METHOD requestMethod;


	HttpVersion version;			//Http版本记录
	int contentLength;				//以8进制表示的请求体的长度
	CONNECTION_STATE connectionState;	//客户端（浏览器）想要优先使用的连接类型	keep-alive or close

	//std::string cookie;				//	由之前服务器通过Set-Cookie设置的一个HTTP协议Cookie
	//std::string cookieControl;		//是否可以被缓存（public可以；private和no - cache不可以；max - age表示可被缓存的时间长）

	//std::string from;				//发起此请求的用户的邮件地址
	//std::string host;				//表示服务器的域名以及服务器所监听的端口号。如果所请求的端口是对应的服务的标准端口（80），则端口号可以省略。
	//std::string acceptLanguage;		//可接受的响应内容语言列表。	
	//std::string acceptDateTime;		//可接受的按照时间来表示的响应内容版本
	//std::string acceptEncoding;		//可接受的响应内容的编码方式。
	//std::string acceptCharset;		//可接受的字符集
	//std::string accept;				//可接受的响应内容类型（Content-Types）。
	//std::string authorization;		//用于表示HTTP协议中需要认证资源的认证信息
	//std::string userAgent;			//浏览器的身份标识字符串
	//std::string warning;			//一个一般性的警告，表示在实体内容体中可能存在错误。
	//std::string via;				//告诉服务器，这个请求是由哪些代理发出的。
	//std::string upGrade;			//浏览器的身份标识字符串

	//std::string expires;			//过期时间，优先级低于cache-control中的max-age。
	//std::string lastModified;		//文件的上一次/最近一次的修改时间。
	std::string requestBody;
	std::map<std::string, std::string> requestHeader;
	size_t age;						//从原始服务器到代理缓存形成的估算时间（以秒计，非负）
	//是否启用cgi
	int cgi;
	HTTP_CODE parseRequestLine();
	HTTP_CODE parseHeader();
	HTTP_CODE doRequest();
	HTTP_CODE parseContent();

	std::string requestPath;
	std::string requestQuery;
};

