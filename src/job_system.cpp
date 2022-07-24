#include "job_system.hpp"

#include <cassert>
#include <iostream>
#include <random>

JobSystem::JobSystem()
	: start_(false)
	, worker_count_(0)
{
}

JobSystem& JobSystem::Get()
{
	static JobSystem job_system;
	return job_system;
}

void JobSystem::Start(uint32_t worker_count)
{
	assert(!start_ && worker_count >= 1);

	if (!start_)
	{
		start_ = true;
		worker_count_ = 0;

		work_queues_.resize(worker_count);

		// 记录主线程队列
		work_queues_[worker_count_] = GetWorkerThreadQueue();
		++worker_count_;

		for (int i = 0; i < worker_count - 1; ++i)
		{
			workers_.emplace_back([this,worker_count]()
				{
					// 记录工作线程队列
					uint32_t index = worker_count_.load(std::memory_order_acquire);
					work_queues_[index] = GetWorkerThreadQueue();
					worker_count_.fetch_add(1, std::memory_order_release);

					//等待工作线程初始化完成
					while (worker_count_ != worker_count){};

					while (start_)
					{
						Job* job = GetJob();
						if (job)
						{
							Execute(job);
						}
					}
				});
		}

		//等待工作线程初始化完成
		while (worker_count_ != worker_count) {};
	}
}

void JobSystem::Stop()
{
	start_ = false;
	for(auto& worker : workers_)
	{
		if(worker.joinable())
		{
			worker.join();
		}
	}
}

Job* JobSystem::CreateJob(const JobFunction& function) const
{
	Job* job = AllocateJob();
	if(job)
	{
		job->function = function;
		job->parent = nullptr;
		job->unfinished_jobs = 1;

		return job;
	}

	return nullptr;
}

Job* JobSystem::CreateJobAsChild(Job* parent, const JobFunction& function) const
{
	assert(parent);

	Job* job = CreateJob(function);
	if(job)
	{
		++parent->unfinished_jobs;
		job->parent = parent;

		return job;
	}

	return nullptr;
}

void JobSystem::Run(Job* job) const
{
	WorkStealingQueue* queue = GetWorkerThreadQueue();
	queue->Push(job);
}

void JobSystem::Wait(const Job* job) const
{
	// 等待作业完成,同时可以做其他任何工作
	while(!HasJobCompleted(job))
	{
		Job* next_job = GetJob();
		if(next_job)
		{
			Execute(next_job);
		}
	}
}

void JobSystem::Finish(Job* job) const
{
	assert(job);

	int32_t unfinished_jobs = --job->unfinished_jobs;
	if(unfinished_jobs == 0 && job->parent)
	{
		Finish(job->parent);
	}
}

uint32_t JobSystem::GenerateRandomNumber(uint32_t min, uint32_t max) const
{
	std::uniform_int_distribution<uint32_t> u(min, max - 1);
	return u(random_engine_);
}

Job* JobSystem::GetJob() const
{
	WorkStealingQueue* queue = GetWorkerThreadQueue();

	Job* job = queue->Pop();
	if(job == nullptr)
	{
		//当前线程的工作队列是空的，尝试从其他队列中窃取
		uint32_t random_index = GenerateRandomNumber(0, worker_count_);
		WorkStealingQueue* steal_queue = work_queues_[random_index];
		if(steal_queue == queue)
		{
			std::this_thread::yield();
			return nullptr;
		}

		Job* stolen_job = steal_queue->Steal();
		if(stolen_job == nullptr)
		{
			
			std::this_thread::yield();
			return nullptr;
		}

		return stolen_job;
	}

	return job;
}

void JobSystem::Execute(Job* job) const
{
	job->function(job, job->data);
	Finish(job);
}

bool JobSystem::HasJobCompleted(const Job* job) const noexcept
{
	assert(job);

	return job->unfinished_jobs == 0;
}

Job* JobSystem::AllocateJob() const
{
	thread_local Job job_pool[kMaxJobCount];
	thread_local uint32_t allocated_jobs = 0;

	uint32_t index = allocated_jobs++;
	return &job_pool[index & (kMaxJobCount - 1)];
}

JobSystem::WorkStealingQueue* JobSystem::GetWorkerThreadQueue() const
{
	thread_local WorkStealingQueue queue;
	return &queue;
}
