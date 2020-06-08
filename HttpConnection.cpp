#include "HttpConnection.h"
#include <string.h>

HttpConnection::HTTP_CODE HttpConnection::httpMessageDecode()
{
	char* currCheckIndex = inputBuffer.beginRead();
	LINE_STATUS lineStatus = LINE_OK;
	HTTP_CODE ret = NO_REQUEST;
	while ((checkState == CHECK_STATE_CONTENT and lineStatus == LINE_OK) or (lineStatus = parseLine()) == LINE_OK)
	{
		switch (checkState)
		{
		case CHECK_STATE_REQUESTLINE:
			ret = parseRequestLine();
			if (ret == BAD_REQUEST)		return BAD_REQUEST;
			inputBuffer.retrieve(processLength);
			break;
		case CHECK_STATE_HEADER:
			ret = parseHeader();
			if (ret == BAD_REQUEST)		return BAD_REQUEST;
			else if (ret == GET_REQUEST) {
				inputBuffer.retrieve(processLength);
				return doRequest();
			}
			break;
		case CHECK_STATE_CONTENT:
			ret = parseContent();

			if (ret == GET_REQUEST) {
				inputBuffer.retrieve(processLength);
				return doRequest();
			}
			lineStatus = LINE_OPEN;
			break;
		default:
			return INTERNAL_ERROR;
			break;
		}
	}
	return NO_REQUEST;
}

bool HttpConnection::httpMessageWrite()
{
	return false;
}

HttpConnection::LINE_STATUS HttpConnection::parseLine()
{
	char* currCheckPos = inputBuffer.beginRead();
	int index = 0;
	for (; index < inputBuffer.readableBytes(); ++index) {
		currCheckPos++;
		if (*currCheckPos == '\r') {
			//达到最后一个字符，则inputBuffer消息不完整。
			if (currCheckPos + 1 == inputBuffer.end())		return LINE_OPEN;
			else if (*(currCheckPos + 1) == '\n') {
				*(currCheckPos) = '\0';
				*(currCheckPos + 1) = '\0';
				processLength = index + 1;
				return LINE_OK;
			}
			return LINE_BAD;
		}
		//上次读取到\r就到buffer末尾了，没有接收完整，再次接收时会出现这种情况
		else if (*currCheckPos == '\n') {
			if (currCheckPos > inputBuffer.begin() and *(currCheckPos - 1) == '\r') {
				*(currCheckPos - 1) = '\0';
				*(currCheckPos) = '\0';
				processLength = index + 1;
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	//没有\r\n，inputBuffer消息不完整
	return LINE_OPEN;
}

HttpConnection::HTTP_CODE HttpConnection::parseRequestLine()
{
	//在HTTP报文中，请求行用来说明请求类型,要访问的资源以及所使用的HTTP版本，其中各个部分之间通过\t或空格分隔。
    //请求行中最先含有空格和\t任一字符的位置并返回
	char* m_url = strpbrk(inputBuffer.beginRead(), " \t");
	
	//如果没有空格或\t，则报文格式有误
	if (!m_url)		return BAD_REQUEST;

	//将该位置改为\0，用于将前面数据取出
	*m_url++ = '\0';

	//取出数据，并通过与GET和POST比较，以确定请求方式
	char* method = inputBuffer.beginRead();

	if (strcasecmp(inputBuffer.beginRead(), "GET") == 0) {
		requestMethod = GET;
	}
	else if (strcasecmp(inputBuffer.beginRead(), "POST") == 0) {
		requestMethod = POST;
		cgi = 1;
	}
	else	return BAD_REQUEST;

	//m_url此时跳过了第一个空格或\t字符，但不知道之后是否还有
	//将m_url向后偏移，通过查找，继续跳过空格和\t字符，指向请求资源的第一个字符
	m_url += strspn(m_url, " \t");

	//使用与判断请求方式的相同逻辑，判断HTTP版本号
	auto m_version = strpbrk(m_url, " \t");

	if (!m_version)		return BAD_REQUEST;
	*m_version++ = '\0';
	m_version += strspn(m_version, " \t");

	//仅支持HTTP/1.1
	if (strcasecmp(m_version, "HTTP/1.1") != 0)
		return BAD_REQUEST;
	
	//对请求资源前7个字符进行判断
	//这里主要是有些报文的请求资源中会带有http://，这里需要对这种情况进行单独处理
	if (strncasecmp(m_url, "http://", 7) == 0)
	{
		m_url += 7;
		m_url = strchr(m_url, '/');
	}

	if (strncasecmp(m_url, "https://", 8) == 0)
	{
		m_url += 8;
		m_url = strchr(m_url, '/');
	}

	if (!m_url || *m_url != '/')
		return BAD_REQUEST;

	if (strlen(m_url) == 1) {
	}
	checkState = CHECK_STATE_HEADER;
	return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::parseHeader()
{
	char* text = inputBuffer.beginRead();
	if (text[0] == '\0')
	{
		//判断是GET还是POST请求 0 - GET , else - POST
		if (contentLength != 0)
		{
			checkState = CHECK_STATE_CONTENT;
			return NO_REQUEST;
		}
		return GET_REQUEST;
	}
	else if (strncasecmp(text, "Connection:", 11) == 0)
	{
		text += 11;
		text += strspn(text, " \t");
		if (strcasecmp(text, "keep-alive") == 0)
		{
			// 如果是长连接，则将linger标志设置为true
			linger = true;
		}
	}
	else if (strncasecmp(text, "Content-length:", 15) == 0)
	{
		text += 15;
		text += strspn(text, " \t");
		contentLength = atol(text);
	}
	else if (strncasecmp(text, "Host:", 5) == 0)
	{
		text += 5;
		text += strspn(text, " \t");
		host = text;
	}
	else
	{
		printf("oop!unknow header: %s\n", text);
	}
	return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::doRequest()
{
	return HTTP_CODE();
}

HttpConnection::HTTP_CODE HttpConnection::parseContent()
{
	return HTTP_CODE();
}
