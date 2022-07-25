#pragma once

#include <functional>
#include <atomic>
#include <limits>
#include <type_traits>

namespace job_base
{
	constexpr  uint32_t Align(uint32_t size, uint32_t alignment)
	{
		return (size + alignment - 1) & ~((alignment)-1);
	}

}

struct Job;
using JobFunction = void(*)(Job*, const void*);

struct Job
{
	static constexpr uint32_t kJobMemSize = 64;
	static constexpr uint32_t kDataCapacity = kJobMemSize -
		job_base::Align(sizeof(JobFunction) + sizeof(Job*) + sizeof(std::atomic_int32_t),
			sizeof(void*));

	JobFunction function;	// 8		0
	Job* parent;			// 8		8
	std::atomic_int32_t unfinished_jobs;	// 4 16
	union 
	{
		void* data;
		char padding[kDataCapacity];
	};
};

template<class T,class S>
struct ParallelForJobData
{
	T* data;
	uint32_t count;
	std::function<void(T*, uint32_t)> function;
};