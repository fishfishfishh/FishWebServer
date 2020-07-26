#pragma once
#include <boost/noncopyable.hpp>
#include <mutex>
#include <vector>
#include <assert.h>
#include <functional>
#include <iostream>
#include <thread>
#include "CallBacks.h"
#include "Semaphore.h"
#include "BlockingQueue.h"


class ThreadPool : boost::noncopyable
{
	class connection_pool;
	typedef std::vector<std::thread*> ThreadPtrVector;
	typedef BlockingQueue<WeakTcpConnectionPtr> messageQueue;
public:
	ThreadPool(int thread_number = 4, int max_requests = 10000);

	//最好不要再初始化里注册回调函数，有可能出问题。
	void init();
	void append(WeakTcpConnectionPtr& events);
	void append(WeakTcpConnectionPtr&& events);
	void run();
	~ThreadPool();

private:
	bool stop;
	//std::mutex mutexLock;
	ThreadPtrVector thread_pool;
	messageQueue  workQue;
	//connection_pool* conn_pool;
	//Semaphore queuestate;
};


inline ThreadPool::ThreadPool(int thread_number, int max_requests):
	thread_pool(thread_number), stop(false), workQue(max_requests)
{
}


inline void ThreadPool::init()
{
	for (int i = 0; i < thread_pool.size(); ++i) {
		thread_pool[i] = new std::thread(std::bind(&ThreadPool::run, this));
		assert(thread_pool[i] != nullptr);
		thread_pool[i]->detach();
	}
}


inline void ThreadPool::append(WeakTcpConnectionPtr& events)
{
	workQue.append(events);
}

inline void ThreadPool::append(WeakTcpConnectionPtr&& events)
{
	workQue.append(std::move(events));
}

inline void ThreadPool::run()
{
	while (!stop)
	{
		auto req(std::move(workQue.take()));
		auto ptr = req.lock();
		if(ptr != nullptr)	ptr->handleEvents();
	}
}


inline ThreadPool::~ThreadPool()
{
	//如何安全退出？
	stop = true;
	for (int i = 0; i < thread_pool.size(); ++i)	delete thread_pool[i];
}

