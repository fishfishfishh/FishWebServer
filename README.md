# FishWebServer
it is a toy webserver based on reactor model 


# 项目模型
在处理web请求时，通常有两种体系结构，分别为：thread-based architecture（基于线程）、event-driven architecture（事件驱动），本项目使用的是第一种
## thread-based architcture
基于线程的体系结构通常会使用多线程来处理客户端的请求，每当接收到一个请求，便开启一个独立的线程来处理。这种方式虽然是直观的，但是仅适用于并发访问量不大的场景，因为线程需要占用一定的内存资源，且操作系统在线程之间的切换也需要一定的开销，当线程数过多时显然会降低web服务器的性能。并且，当线程在处理I/O操作，在等待输入的这段时间线程处于空闲的状态，同样也会造成cpu资源的浪费（是如此吗？）。一个典型的设计如下：
![img](https://upload-images.jianshu.io/upload_images/10345180-faaebf9335592620.png?imageMogr2/auto-orient/strip|imageView2/2/w/661/format/webp)

# 为什么TCP服务器的监听套接字要设置为非阻塞
我们一般使用IO复用来实现并发模型，如果我们默认监听套接字为阻塞模式，假设一种场景如下：
客户通过connect向TCP服务器发起三次握手
三次握手完成后，触发TCP服务器监听套接字的可读事件，IO复用返回（select、poll、epoll_wait）
客户通过RST报文取消连接
TCP服务器调用accept接受连接，此时发现内核已连接队列为空（因为唯一的连接已被客户端取消）
程序阻塞在accept调用，无法响应其它已连接套接字的事件
为了防止出现上面的场景，我们需要把监听套接字设置为非阻塞



————————————————
版权声明：本文为CSDN博主「swings_ss」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/zhwenx3/article/details/88107428

#HTTP的状态机模型
该部分引用与 微信公众号"两猿社"
![img](https://mmbiz.qpic.cn/mmbiz_jpg/6OkibcrXVmBH2ZO50WrURwTiaNKTH7tCia3AR4WeKu2EEzSgKibXzG4oa4WaPfGutwBqCJtemia3rc5V1wupvOLFjzQ/640?wx_fmt=jpeg&tp=webp&wxfrom=5&wx_lazy=1&wx_co=1)

# 用 Timing Wheel 踢掉空闲的链接
![img](https://upload-images.jianshu.io/upload_images/2116753-4fac9916aef57694.jpg?imageMogr2/auto-orient/strip|imageView2/2/w/1200/format/webp)
Timeing Wheel的主循环是一个循环队列，可以自己实现，也可以使用std::deque来实现，这里我是用的是boost::circular_buffer来做的。

每个slot中的数据结构可以用双向list，也可以用unordered_set来管理链接。

这里有一个小技巧，用智能指针管理链接的周期。具体实现方法如下：
~~~
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
~~~
timeWheel管理的是每一个EventNode的shared_ptr,每一个链接有且只有一个唯一的shared_ptr<EventNode> 

然后我们在EventNode的析构函数中处理链接即可。

因此我们就面临一个循环嵌套的问题，及在TcpConnection中含有EventNode的shared_ptr,然后再EventNode中也含有TcpConnection的shared_ptr，所以必须有一个是weak_ptr,改哪一个好呢？

TcpConnection中的EventNode一定是Weak_ptr,若是shared_ptr,则TimeWheel就不能删除任何一个链接，因为EventNode永远存在（主动关闭连接除外）。

EventNode的TcpConnection呢？若是shared_ptr，那么主动关闭连接的时候,所有的TcpConnection不能正常析构，只有在TimeWheel转动到时间了，才正常析构。

所以，最好的方式就是两个都是Weak_ptr包含在对方的Class。
# 如何实现Log日志的输出级别调整
该部分参考 陈硕先生muduo里的Logging.h中的实现手法。
使用
~~~
#define if(a) / b
~~~
这种形式就可以简单的实现输出级别的调整。
