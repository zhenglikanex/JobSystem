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

	using WorkStealingQueue = WorkStealingQueue<kMaxJobCount>;

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
	Job* CreateJobAsChild(Job* parent, const JobFunction& function) const;
private:
	JobSystem();

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