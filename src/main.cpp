#include <thread>
#include <vector>
#include <iostream>

#include "job_system.hpp"

struct TestData
{
	char c[200] = { 'a' };

	~TestData() { std::cout << "~TestData" << std::endl; }
};

void RootJob(Job* job,void* data)
{
	
}

void TestJob(Job* job,void* data)
{
	auto *p = *static_cast<int**>(data);
	std::cout << *p << std::endl;
}

void PrintVector(Job* job,void* data)
{
	std::vector<int>& vec = *static_cast<std::vector<int>*>(data);

	for(auto& v : vec)
	{
		std::cout << v << std::endl;
	}
}

void PrintTestData(Job* job,void* data)
{
	std::unique_ptr<TestData> p = std::move(*static_cast<std::unique_ptr<TestData>*>(data));
	p->c[200 - 1] = 0;
	char* pc = (p->c);
	std::cout << pc << std::endl;
}

void SingleFor()
{
	auto& job_system = JobSystem::Get();
	uint32_t count = 10000;
	std::vector<int> data(count);
	auto func = [](int* data,uint32_t count)
	{
		for (int i = 0; i < count; ++i)
		{
			//*data += i;
		}
	};
}


struct Timer
{
	std::string name;
	std::chrono::system_clock::duration start;
	Timer(const std::string& name)
	{
		this->name = name;
		start = std::chrono::system_clock::now().time_since_epoch();
	}

	~Timer()
	{
		auto t = std::chrono::system_clock::now().time_since_epoch() - start;
		std::cout << name << std::chrono::duration_cast<std::chrono::milliseconds>(t).count() << "ms" << std::endl;
	}
};


int main()
{

	auto& job_system = JobSystem::Get();
	//job_system.Start(std::thread::hardware_concurrency());
	job_system.Start(1);
	
	std::vector<float> vec(10000,10);

	std::cout << "begin" << std::endl;
	{
		Timer t("time");
		std::function<void(float*, uint32_t)> func = [](float* data, uint32_t count)
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

		auto root = job_system.ParallelFor(vec.data(), vec.size(), func, 1);

		job_system.Run(root);
		job_system.Wait(root);
	}
	
	job_system.Stop();
	system("pause");
	return 0;
}