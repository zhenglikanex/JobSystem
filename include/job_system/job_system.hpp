#pragma once

#include <cassert>
#include <cstdint>
#include <thread>
#include <random>
#include <new>
#include <mutex>

#include "job.hpp"
#include "work_stealing_queue.hpp"

class JobSystem
{
public:
	static constexpr uint32_t kMaxJobCount = 32768;
	static_assert((kMaxJobCount& (kMaxJobCount - 1)) == 0, "!");

	using WorkStealingQueue = WorkStealingQueue<Job*,kMaxJobCount>;

	JobSystem(const JobSystem&) = delete;
	JobSystem(JobSystem&&) = delete;
	JobSystem& operator=(const JobSystem&) = delete;
	JobSystem& operator=(JobSystem&&) = delete;

	static JobSystem& Get();

	void Start(uint32_t worker_count);
	void Stop();
	void Run(Job* job) const;
	void Wait(const Job* job) const;

	Job* CreateJob(const JobFunction& function) const;
	Job* CreateJob(JobFunction&& function) const;
	Job* CreateJobAsChild(Job* parent, const JobFunction& function) const;
	Job* CreateJobAsChild(Job* parent, JobFunction&& function) const;

	void AddContinuation(Job* ancestor, Job* continuation) const;

	template<class T,class S>
	Job* ParallelFor(T* data,uint32_t count,const std::function<void(T*,uint32_t)>& function,const S& splitter)
	{
		return CreateJob(std::bind(&JobSystem::ParallelForJob<T,S>,this,std::placeholders::_1,data,count,function,splitter));
	}
private:
	JobSystem();

	template<class T,class S>
	void ParallelForJob(Job* job,T* data,uint32_t count,const std::function<void(T*, uint32_t)>& function,const S& splitter)
	{
		if(count > splitter)
		{
			uint32_t left_count = count / 2;
			Job* left = CreateJobAsChild(job, std::bind(&JobSystem::ParallelForJob<T, S>, this, std::placeholders::_1, data, left_count,function,splitter));
			Run(left);

			uint32_t right_count = count - left_count;
			Job* right = CreateJobAsChild(job, std::bind(&JobSystem::ParallelForJob<T, S>, this, std::placeholders::_1, data + left_count, right_count,function,splitter));
			Run(right);
		}
		else
		{
			function(data, count);
		}
	}

	void Finish(Job* job) const;

	uint32_t GenerateRandomNumber(uint32_t min, uint32_t max) const;

	Job* GetJob() const;

	void Execute(Job* job) const;

	bool HasJobCompleted(const Job* job) const noexcept;

	Job* AllocateJob() const;

	WorkStealingQueue* GetWorkerThreadQueue() const;

	std::mutex mutex_;
	std::atomic_bool start_;
	std::atomic_uint32_t worker_count_;
	std::vector<std::thread> workers_;
	std::vector<WorkStealingQueue*> work_queues_;
	mutable std::default_random_engine random_engine_;
};

inline JobSystem::JobSystem()
	: start_(false)
	, worker_count_(0)
	, random_engine_(static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()))
{
}

inline JobSystem& JobSystem::Get()
{
	static JobSystem job_system;
	return job_system;
}

inline void JobSystem::Start(uint32_t worker_count)
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
			workers_.emplace_back([this, worker_count]()
				{
					// 记录工作线程队列
					{
						std::unique_lock loc(mutex_);
						work_queues_[worker_count_++] = GetWorkerThreadQueue();
					}

					//等待工作线程初始化完成
					while (worker_count_ != worker_count) {};

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
		while (worker_count_ != worker_count) {}
	}
}

inline void JobSystem::Stop()
{
	start_ = false;
	for (auto& worker : workers_)
	{
		if (worker.joinable())
		{
			worker.join();
		}
	}
}

inline Job* JobSystem::CreateJob(const JobFunction& function) const
{
	Job* job = AllocateJob();
	if (job)
	{
		job->function = function;
		job->parent = nullptr;
		job->unfinished_jobs.store(1,std::memory_order_relaxed);
		job->continuation_count.store(0, std::memory_order_relaxed);

		return job;
	}

	return nullptr;
}

inline Job* JobSystem::CreateJob(JobFunction&& function) const
{
	Job* job = AllocateJob();
	if (job)
	{
		job->function = std::move(function);
		job->parent = nullptr;
		job->unfinished_jobs.store(1, std::memory_order_relaxed);
		job->continuation_count.store(0, std::memory_order_relaxed);

		return job;
	}

	return nullptr;
}

inline Job* JobSystem::CreateJobAsChild(Job* parent, const JobFunction& function) const
{
	assert(parent);

	Job* job = CreateJob(function);
	if (job)
	{
		parent->unfinished_jobs.fetch_add(1, std::memory_order_relaxed);
		job->parent = parent;

		return job;
	}

	return nullptr;
}

inline Job* JobSystem::CreateJobAsChild(Job* parent, JobFunction&& function) const
{
	assert(parent);

	Job* job = CreateJob(function);
	if (job)
	{
		parent->unfinished_jobs.fetch_add(1, std::memory_order_relaxed);
		job->parent = parent;

		return job;
	}

	return nullptr;
}

inline void JobSystem::AddContinuation(Job* ancestor, Job* continuation) const
{
	auto index = ancestor->continuation_count.fetch_add(1, std::memory_order_relaxed);
	ancestor->continuations[index] = continuation;
}

inline void JobSystem::Run(Job* job) const
{
	WorkStealingQueue* queue = GetWorkerThreadQueue();
	queue->Push(job);
}

inline void JobSystem::Wait(const Job* job) const
{
	// 等待作业完成,同时可以做其他任何工作
	while (!HasJobCompleted(job))
	{
		Job* next_job = GetJob();
		if (next_job)
		{
			Execute(next_job);
		}
	}
}

inline void JobSystem::Finish(Job* job) const
{
	assert(job);

	int32_t unfinished_jobs = job->unfinished_jobs.fetch_sub(1, std::memory_order_relaxed);
	if (--unfinished_jobs == 0)
	{
		if (job->parent)
		{
			Finish(job->parent);
		}

		for (int32_t i = 0; i < job->continuation_count; ++i)
		{
			Run(job->continuations[i]);
		}
	}
}

inline uint32_t JobSystem::GenerateRandomNumber(uint32_t min, uint32_t max) const
{
	if(!work_queues_[0]->IsEmpty())
	{
		return 0;
	}

	std::uniform_int_distribution<uint32_t> u(min, max - 1);
	return u(random_engine_);
}

inline Job* JobSystem::GetJob() const
{
	WorkStealingQueue* queue = GetWorkerThreadQueue();

	Job* job = queue->Pop();
	if (job == nullptr)
	{
		//当前线程的工作队列是空的，尝试从其他队列中窃取
		uint32_t random_index = GenerateRandomNumber(0, worker_count_);
		WorkStealingQueue* steal_queue = work_queues_[random_index];
		if (steal_queue == queue)
		{
			std::this_thread::yield();
			return nullptr;
		}

		Job* stolen_job = steal_queue->Steal();
		if (stolen_job == nullptr)
		{

			std::this_thread::yield();
			return nullptr;
		}

		return stolen_job;
	}

	return job;
}

inline void JobSystem::Execute(Job* job) const
{
	job->function(job);
	job->function = nullptr;
	Finish(job);
}

inline bool JobSystem::HasJobCompleted(const Job* job) const noexcept
{
	assert(job);

	return job->unfinished_jobs.load(std::memory_order_relaxed) == 0;
}

inline Job* JobSystem::AllocateJob() const
{
	thread_local Job job_pool[kMaxJobCount];
	thread_local uint32_t allocated_jobs = 0;

	uint32_t index = allocated_jobs++;
	return &job_pool[index & (kMaxJobCount - 1)];
}

inline JobSystem::WorkStealingQueue* JobSystem::GetWorkerThreadQueue() const
{
	thread_local WorkStealingQueue queue;
	return &queue;
}