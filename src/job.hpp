#pragma once

#include <functional>
#include <atomic>
#include <limits>

class Job;

using JobFunction = std::function<void(Job*, const void*)>;

struct Job
{
	JobFunction function;
	Job* parent;
	std::atomic_int32_t unfinished_jobs;
	union 
	{
		char padding[1];	// tМоід todo
		void* data;
	};
};
