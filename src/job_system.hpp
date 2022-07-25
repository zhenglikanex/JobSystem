#pragma once

#include <cstdint>
#include <thread>
#include <random>

#include "job.hpp"
#include "work_stealing_queue.hpp"

class JobSystem
{
public:
	static constexpr uint32_t kMaxJobCount = 4096;
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
	//Job* CreateJob(const JobFunction& function, const void* data);

	template<class T>
	Job* CreateJob(const JobFunction& function,T&& data)
	{
		Job* job = CreateJob(function);
		if(job)
		{
			if constexpr (sizeof(T) <= Job::kDataCapacity)
			{
				*reinterpret_cast<std::remove_reference_t<T>*>((&job->data)) = std::move(data);
			}
			else
			{

			}
		}

		return job;
	}

	Job* CreateJobAsChild(Job* parent, const JobFunction& function) const;
	//Job* CreateJobAsChild(Job* parent, const JobFunction& function,const void* data) const;

	/*template<class T>
	Job* CreateJobAsChild(Job* parent, const JobFunction& function,T&& data)
	{
		
	}*/

	/*template<class T,class S>
	Job* ParallelFor(T* data,uint32_t count,std::function<void(T*,uint32_t)>& function,const S& splitter)
	{
		ParallelForJobData<T, S> job_data = { data,count,function };
		return CreateJob(&ParallelForJob, &job_data);
	}*/
private:
	JobSystem();

	/*template<class T,class S>
	void ParallelForJob(Job* job,const void* job_data)
	{
		const ParallelForJobData<T, S>* data = static_cast<const ParallelForJobData<T, S>*>(job_data);
		if(data->count > 256)
		{
			uint32_t left_count = data->count / 2;
			ParallelForJobData<T, S> left_data = { data->data,left_count,data->function };
			Job* left = CreateJobAsChildJob(job,&ParallelForJob, &left_data);
			Run(left);

			uint32_t right_count = data->count - left_count;
			ParallelForJobData<T, S> right_data = { data->data + left_count,right_count,data->function };
			Job* right = CreateJobAsChildJob(job, &ParallelForJob, &right_data);
			Run(right);
		}
		else
		{
			data->function(data->data, data->count);
		}
	}*/

	void Finish(Job* job) const;

	uint32_t GenerateRandomNumber(uint32_t min, uint32_t max) const;

	Job* GetJob() const;

	void Execute(Job* job) const;

	bool HasJobCompleted(const Job* job) const noexcept;

	Job* AllocateJob() const;

	WorkStealingQueue* GetWorkerThreadQueue() const;

	std::atomic_bool start_;
	std::atomic_uint32_t worker_count_;
	std::vector<std::thread> workers_;
	std::vector<WorkStealingQueue*> work_queues_;
	mutable std::default_random_engine random_engine_;
};