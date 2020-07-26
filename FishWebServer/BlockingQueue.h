#pragma once
#include <list>
#include <condition_variable>
#include "CallBacks.h"
#include <boost/circular_buffer.hpp>

template<typename T>
class BlockingQueue
{
	typedef std::lock_guard<std::mutex> LockGuard;
	typedef std::unique_lock<std::mutex> uniqueLock;
public:
	BlockingQueue(int size):maxSize(size)
	{}
	void append(const T& events) {
		uniqueLock lock(mutexLock);
		while (messageQue.size() == maxSize) {
			unFull.wait(lock);
		}
		messageQue.push_back(events);
		unEmpty.notify_one();
	}
	void append(T&& events) {
		uniqueLock lock(mutexLock);
		while (messageQue.size() == maxSize) {
			unFull.wait(lock);
		}
		messageQue.push_back(std::move(events));
		unEmpty.notify_one();
	}
	T take() {
		uniqueLock lock(mutexLock);
		while (messageQue.empty())
		{
			unEmpty.wait(lock);
		}
		T target(std::move(messageQue.front()));
		messageQue.pop_front();

		unFull.notify_one();
		return target;
	}
	size_t size() const{
		LockGuard lock(mutexLock);
		return messageQue.size();
	}
	bool full() const{
		LockGuard lock(mutexLock);
		return maxSize == messageQue.size();
	}
	bool empty()const {
		LockGuard lock(mutexLock);
		return messageQue.empty();
	}
	int getMaxSize() {
		return maxSize;
	}
	~BlockingQueue(){}
private:
	int maxSize;
	std::list<T> messageQue;
	std::condition_variable unEmpty;
	std::condition_variable unFull;
	mutable std::mutex mutexLock;
};

template<>
class BlockingQueue<TcpConnectionPtr> {
	typedef std::lock_guard<std::mutex> LockGuard;
	typedef std::unique_lock<std::mutex> uniqueLock;
public:
	BlockingQueue(size_t size) :maxSize(size)
	{}
	bool append(const TcpConnectionPtr& events) {
		LockGuard lock(mutexLock);
		if (messageQue.size() == maxSize)	return false;
		messageQue.push_back(events);
		unEmpty.notify_one();
		return true;
	}
	TcpConnectionPtr take() {
		uniqueLock lock(mutexLock);
		while (messageQue.empty())
		{
			unEmpty.wait(lock);
		}
		TcpConnectionPtr target(std::move(messageQue.front()));
		messageQue.pop_front();
		return target;
	}
	size_t size()  const {
		LockGuard lock(mutexLock);
		return messageQue.size();
	}
	bool full()  const {
		LockGuard lock(mutexLock);
		return maxSize == messageQue.size();
	}
	bool empty() const {
		LockGuard lock(mutexLock);
		return messageQue.empty();
	}
	~BlockingQueue() {}
private:
	size_t maxSize;
	std::list<TcpConnectionPtr> messageQue;
	std::condition_variable unEmpty;
	mutable std::mutex mutexLock;
};
