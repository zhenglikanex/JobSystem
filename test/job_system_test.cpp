#include <thread>
#include <vector>
#include <iostream>
#include <string>

#include "../include/job_system/job_system.hpp"
#include "timer.hpp"

int main()
{
	std::atomic_uint32_t jobs = 0;

	auto& job_system = JobSystem::Get();
	std::vector<float> vec(500000, 10);
	job_system.Start(std::thread::hardware_concurrency());
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

		auto root = job_system.ParallelFor(vec.data(), vec.size(), func, 256);

		job_system.Run(root);
		job_system.Wait(root);
	}

	std::cout << jobs << std::endl;

	job_system.Stop();
	system("pause");
	return 0;
}