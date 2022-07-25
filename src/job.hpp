#pragma once

#include <functional>
#include <atomic>

struct Job;
using JobFunction = std::function<void(Job*)>;

struct Job
{
	JobFunction function;
	Job* parent;
	std::atomic_int32_t unfinished_jobs;
};

template<class T,class S>
struct ParallelForJobData
{
	T* data;
	uint32_t count;
	std::function<void(T*, uint32_t)> function;
};