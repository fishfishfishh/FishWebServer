#include "ConnectionPool.h"
#include "Log.h"


void ConnectionPool::init(std::string host, int port, std::string passward)
{
	for (int i = 0; i < contextQue.getMaxSize(); ++i) {
		contextQue.append(std::make_shared<RedisContext>(host,port));
	}
	
}

ConnectionPool::ConnectionPool() :contextQue(4)
{
}
ConnectionPool::~ConnectionPool()
{
	while (!contextQue.empty()) {
		auto conn = contextQue.take();
	}
	
}

void ConnectionPool::push(RedisContextPtr& ctx)
{
	contextQue.append(ctx);
}

ConnectionPool::RedisContextPtr ConnectionPool::take()
{
	return contextQue.take();
}

