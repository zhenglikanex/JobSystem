#include <thread>
#include <vector>

#include "job_system.hpp"

void TestJob(Job* job,const void* data)
{

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


	
	//job_system.ParallelFor(data.data(), count, func, 20);
}

int main()
{
	std::vector<int> a;
	constexpr int i = sizeof(std::vector<Job>);
	auto& job_system = JobSystem::Get();
	job_system.Start(std::thread::hardware_concurrency());
	int* a = new int(10);
	job_system.CreateJob(&TestJob,a);

	auto root = job_system.CreateJob(&TestJob);

	for (int i = 0; i < 1000; ++i)
	{
		auto* job = job_system.CreateJobAsChild(root, &TestJob);
		job_system.Run(job);
	}

	job_system.Run(root);
	job_system.Wait(root);
	job_system.Stop();

	return 0;
}