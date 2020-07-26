#pragma once
#include "BlockingQueue.h"
#include <hiredis/hiredis.h>
#include <iostream>
struct RedisContext;
class ConnectionPool
{
	typedef std::shared_ptr<RedisContext> RedisContextPtr;
public:
	static ConnectionPool& getInstance() {
		static ConnectionPool pool;
		return pool;
	}

	void init(std::string host, int port, std::string passward);
	void push(RedisContextPtr&);
	RedisContextPtr take();
private:
	ConnectionPool();
	~ConnectionPool();
	
	BlockingQueue<RedisContextPtr> contextQue;
};

struct RedisContext {
	RedisContext(std::string host, int port) {
		context = redisConnect(host.c_str(), port);
		std::cout << "connect sucess!" << std::endl;
	}
	~RedisContext() { redisFree(context); std::cout << "free sucess!" << std::endl;
	}
	redisContext* context;
};
