#include <thread>
#include <vector>

#include "job_system.hpp"


void TestJob(Job* job,const void* data)
{

}


int main()
{
	auto& job_system = JobSystem::Get();
	job_system.Start(std::thread::hardware_concurrency());
	auto root = job_system.CreateJob(&TestJob);

	for (int i = 0; i < 1000; ++i)
	{
		auto* job = job_system.CreateJobAsChild(root, &TestJob);
		job_system.Run(job);
	}

	job_system.Run(root);
	job_system.Wait(root);
	job_system.Stop();

	return true;
}