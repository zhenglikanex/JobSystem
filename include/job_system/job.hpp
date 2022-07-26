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
	std::atomic_int32_t continuation_count;
	Job* continuations[6];
};

constexpr int s = sizeof(Job);
