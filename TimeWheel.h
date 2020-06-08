#pragma once

#include "CallBacks.h"
#include "TcpConnection.h"

struct EventNode
{
	explicit EventNode(const WeakTcpConnectionPtr& ptr)
		:weakPtr(ptr)
	{ }
	~EventNode()
	{
		auto ptr = weakPtr.lock();
		if (ptr != nullptr) {
			ptr->closeSocket();
		}
	}
	WeakTcpConnectionPtr weakPtr;
};

class TimeWheel
{
public:
	explicit TimeWheel(int bucketNum) :wheel(bucketNum) {
	}
	void tick() {
		wheel.push_back(Bucket());
	}
	void push(const NodePtr& ptr) {
		wheel.back().insert(ptr);
	}
	~TimeWheel() {
	}

private:
	timeWheel wheel;
};


