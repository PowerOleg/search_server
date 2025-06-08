#pragma once
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <syncstream>
#include <iostream>

class Thread_pool
{
public:
	Thread_pool(boost::asio::io_context &ioc_, size_t pool_size) : ioc{ ioc_ }, prodWork{ boost::asio::make_work_guard(ioc_) }
	{
		for (size_t i = 0; i < pool_size; i++)
		{
			threads.create_thread([this]() {ioc.run(); });
		}
	}

	~Thread_pool()
	{
		ioc.stop();
		threads.join_all();
	}

	template <typename Task>
	void Enqueue(Task task)
	{
		boost::asio::post(ioc, task);//https://www.boost.org/doc/libs/master/doc/html/boost_asio/reference/io_context.html
	}

	void Destroy()
	{
		ioc.stop();
		threads.join_all();
	}
	void JoinAll()
	{
		threads.join_all();
	}

private:
	boost::asio::io_context &ioc;
	boost::thread_group threads;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type, void, void> prodWork;
};
