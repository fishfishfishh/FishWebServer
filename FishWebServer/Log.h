#pragma once
#include <string>
#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream> 
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "BlockingQueue.h"
#include "CallBacks.h"
class Log
{
public:
	enum LogLevel
	{
		ALL,
		TRACE,
		DEBUG,
		INFO,
		WARNNING,
		ERROR,
		FATAL,
		OFF,
	};
	static Log& getInstance() {
		static Log instance(10000);
		return instance;
	}
	static void threadFunc() {
		Log::getInstance().run();
	}
	void setLogLevel(LogLevel level) { logLevel = level; }
	Log::LogLevel getLogLevel() { return logLevel; }

	void write(Log::LogLevel level , const char *format, ...) {
		char s[16] = { 0 };
		switch (level)
		{
		case 0:
			strcpy(s, "[ALL]:");
			break;
		case 1:
			strcpy(s, "[TRACE]:");
			break;
		case 2:
			strcpy(s, "[DEBUG]:");
			break;
		case 3:
			strcpy(s, "[INFO]:");
			break;
		case 4:
			strcpy(s, "[WARNNING]:");
			break;
		case 5:
			strcpy(s, "[ERROR]:");
			break;
		case 6:
			strcpy(s, "[FATAL]:");
			break;
		default:
			strcpy(s, "[OFF]:");
			break;
		}
		char str_tmp[128];
		timeval now = { 0, 0 };
		gettimeofday(&now, nullptr);
		time_t lt = now.tv_sec;
		tm *curTime = localtime(&lt);
		//判断时间,若日期改变，则换.log文件

		judgementTime(curTime);

		//写入的具体时间内容格式
		int n = snprintf(str_tmp, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
			curTime->tm_year + 1900, curTime->tm_mon + 1, curTime->tm_mday,
			curTime->tm_hour, curTime->tm_min, curTime->tm_sec, now.tv_usec,s );
		
		va_list vArgList;                            //定义一个va_list型的变量,这个变量是指向参数的指针.
		va_start(vArgList, format);                 //用va_start宏初始化变量,这个宏的第二个参数是第一个可变参数的前一个参数,是一个固定的参数
		int i = vsnprintf(str_tmp + n, 128, format, vArgList);
		str_tmp[n + i] = '\n';
		str_tmp[n + i + 1] = '\0';
		va_end(vArgList);

		logQue.append(std::string(str_tmp));
	}
private:
	void judgementTime(tm *time) {
		if (loggingTime->tm_mday == time->tm_mday)	return;
		//更换log文件
		{
			std::lock_guard<std::mutex> lckGuard(lock);
			if (loggingTime->tm_mday == time->tm_mday)	return;
			writeFile.close();
			initIostream(time, writeFile);
			loggingTime = time;
		}
	}
	void init() {
		//init logging thread
		std::thread writeThread(std::bind(&Log::threadFunc));
		writeThread.detach();
	}
	void run() {
		while (!stop) {
			std::string curLog(std::move(logQue.take()));

			{
				std::lock_guard<std::mutex> lckGuard(lock);
				//每次都flash性能会出现问题，如何解决？
				writeFile << curLog;
				writeFile.flush();
			}

		}
	}
	void initIostream(tm* time, std::ofstream& stream) {
		std::string&& fileName = std::to_string(time->tm_year + 1900) + "-"
			+ std::to_string(time->tm_mon + 1) + "-"
			+ std::to_string(time->tm_mday) + ".log";
		stream.open(fileName, std::ios::out);
		if (stream.fail()) {
			std::cerr << "log file could not be opened!" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	Log(size_t maxSize) :logQue(maxSize),stop(false) {
		//init logging time
		time_t lt = time(nullptr);
		loggingTime = localtime(&lt);
		initIostream(loggingTime, writeFile);
		init();

	}
	~Log() { stop = true; writeFile.close(); }
	BlockingQueue<std::string> logQue;
	LogLevel logLevel;
	bool stop;
	std::thread logThread;

	std::mutex lock;
	tm* loggingTime;
	std::ofstream writeFile;
};

#define LOG_ALL(format, ...) if(Log::getInstance().getLogLevel() <= Log::ALL) \	
	Log::getInstance().write(Log::ALL, format, ##__VA_ARGS__)
#define LOG_TRACE(format, ...) if(Log::getInstance().getLogLevel() <= Log::TRACE) \	
		Log::getInstance().write(Log::TRACE, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) if(Log::getInstance().getLogLevel() <= Log::DEBUG) \	
		Log::getInstance().write(Log::DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) if(Log::getInstance().getLogLevel() <= Log::INFO) \	
		Log::getInstance().write(Log::INFO, format, ##__VA_ARGS__)
#define LOG_WARNNING(format, ...) Log::getInstance().write(Log::WARNNING, format, ##__VA_ARGS__))
#define LOG_ERROR(format, ...) Log::getInstance().write(Log::ERROR, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...) Log::getInstance().write(Log::FATAL, format, ##__VA_ARGS__)
