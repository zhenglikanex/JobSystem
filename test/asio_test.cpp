#include "../3rdparty/asio-1.16.1/include/asio.hpp"

#include <iostream>

#include "timer.hpp"
#include <cstdlib>


class ThreadPool {
public:
	explicit ThreadPool(std::size_t size)
		: work_guard_(asio::make_work_guard(io_context_)) {
		workers_.reserve(size);
		for (std::size_t i = 0; i < size; ++i) {
			workers_.emplace_back([this]
			{
				io_context_.run();
			});
		}
	}

	~ThreadPool() {
		io_context_.stop();

		for (auto& w : workers_) {
			w.join();
		}
	}

	// Add new work item to the pool.
	template<class F>
	void Enqueue(F f) {
		asio::post(io_context_, f);
	}

private:
	std::vector<std::thread> workers_;
	asio::io_context io_context_;

	typedef asio::io_context::executor_type ExecutorType;
	asio::executor_work_guard<ExecutorType> work_guard_;
};

int main()
{
	std::vector<float> vec(500000, 10);

	std::cout << "begin" << std::endl;
	{
		auto pool = std::make_unique<ThreadPool>(1);
		Timer t("time");
		

		for (int i = 0; i < vec.size() / 256; ++i)
		{
			float* data = vec.data() + i * 256;
			uint32_t count = 256;
			std::function<void()> func = [data,count,&pool]()
			{
				for (int i = 0; i < count; ++i)
				{
					for (int j = 0; j < 2000; ++j)
					{
						*(data + i) = sqrt((sqrt(*(data + i)) * sqrt(*(data + i)))) * sqrt((sqrt(*(data + i)) * sqrt(*(data + i))));
						*(data + i) = sqrt((sqrt(*(data + i)) * sqrt(*(data + i)))) * sqrt((sqrt(*(data + i)) * sqrt(*(data + i))));
					}
				}

			};
			pool->Enqueue(func);
		}

		auto count = vec.size() - (vec.size() / 256 * 256);
		if (count > 0)
		{
			float* data = vec.data() + vec.size() - count;
			std::function<void()> func = [data, count]()
			{
				for (int i = 0; i < count; ++i)
				{
					for (int j = 0; j < 200; ++j)
					{
						*(data + i) = sqrt((sqrt(*(data + i)) * sqrt(*(data + i)))) * sqrt((sqrt(*(data + i)) * sqrt(*(data + i))));
						*(data + i) = sqrt((sqrt(*(data + i)) * sqrt(*(data + i)))) * sqrt((sqrt(*(data + i)) * sqrt(*(data + i))));
					}
				}
			};

			pool->Enqueue(func);
		}

		pool.reset();
	}

	system("pause");
	return 0;
}