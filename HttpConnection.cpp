#include "HttpConnection.h"
#include <string.h>

bool HttpConnection::setHearMethod(std::string& str, strIter begin, strIter end)
{
	//这个函数有点东西，最简单的就是用if-else语句疯狂判断，但是效率可能会有点差。
	//我想到如果用到reflex + function overloading，可能会好一些，但是CPP没有reflex,怎么办？用map + decltype？
	//现在暂时交由以后处理,暂时先全部返回True
	requestHeader[str] = std::string(begin, end);
	std::cout << str << "\t" << requestHeader[str];
	return true;
}

HttpConnection::HTTP_CODE HttpConnection::httpMessageDecode()
{
	while ((checkState == CHECK_STATE_CONTENT and lineStatus == LINE_OK) or (lineStatus = parseLine()) == LINE_OK)
	{
		switch (checkState)
		{
		case CHECK_STATE_REQUESTLINE:
			retStatus = parseRequestLine();
			if (retStatus == BAD_REQUEST)		return BAD_REQUEST;
			break;
		case CHECK_STATE_HEADER:
			retStatus = parseHeader();
			if (retStatus == BAD_REQUEST)		return BAD_REQUEST;
			break;
		case CHECK_STATE_CONTENT:
			retStatus = parseContent();
			if (retStatus == BAD_REQUEST)		return BAD_REQUEST;
			lineStatus = LINE_END;
			break;
		default:
			return INTERNAL_ERROR;
			break;
		}
	}
	return NO_REQUEST;
}
HttpConnection::LINE_STATUS HttpConnection::parseLine()
{
	auto currCheckPos = start;
	auto index = start;
	for (; index < message.end(); ++index) {
		currCheckPos++;
		if (*currCheckPos == '\r') {
			//达到最后一个字符，则inputBuffer消息不完整。
			if (currCheckPos + 1 == message.end())
				return LINE_OPEN;
			else if (*(currCheckPos + 1) == '\n') {
				end = currCheckPos + 2;
				return LINE_OK;
			}
			return LINE_BAD;
		}
		//上次读取到\r就到buffer末尾了，没有接收完整，再次接收时会出现这种情况
		else if (*currCheckPos == '\n') {
			if (currCheckPos > message.begin() and *(currCheckPos - 1) == '\r') {
				end = currCheckPos + 1;
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
	auto space = std::find(start, end, ' ');
	//找不到空格，说明请求行出错。
	if (space == end)	return BAD_REQUEST;
	requestMethod = getMethod(start, space);
	//出现无效模式， 请求行出错
	if (requestMethod == INVALID)	return BAD_REQUEST;

	start = space + 1;
	space = std::find(start, end, ' ');
	auto question = std::find(start, space, '?');
	if (question != space) {
		requestPath.assign(start, question);
		requestQuery.assign(question, space);
	}
	else {
		requestPath.assign(start, question);
	}

	start = space + 1;
	// HTTP/1.X\r\n 需要8个字节，小于8个则请求行出错。
	if (end - start < 10 and !std::equal(start, end, "HTTP/1."))	return BAD_REQUEST;
	if (*(end - 3) == '1') {
		version = Http11;
	}
	else if (*(end - 3) == '0') {
		version = Http10;
	}
	else	/*未知HTTP协议类型,则请求行出错*/	return BAD_REQUEST;

	//Request hear 的开头
	start += 10;
	checkState = CHECK_STATE_HEADER;

	return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::parseHeader()
{
	auto space = std::find(start, end, ':');
	//若没找到":"
	if (space == end) {
		//到达空行
		if (std::string(start + 1, end) == "\r\n") {
			//下一行
			
			start = end;
			checkState = CHECK_STATE_CONTENT;
			return GET_REQUEST;
		}
		return BAD_REQUEST;
	}
	//滤除":"  判断
	std::string headRequest = std::string(start, space);
	space = space + 1;

	setHearMethod(headRequest, space, end - 2);

	//下一行
	start = end;
	return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::parseContent()
{
	requestBody = std::string(start, end);
	std::cout << requestBody.empty() << std::endl;
	return requestBody.empty() ? NO_REQUEST : GET_REQUEST;
}

std::string& HttpConnection::getRequestPath()
{
	return requestPath;
}
