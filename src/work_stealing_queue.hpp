#pragma once

#include <atomic>

template<class T,size_t N>
class WorkStealingQueue
{
public:
	static constexpr size_t kMask = N - 1;
	static_assert((N & kMask) == 0, "!");

	WorkStealingQueue()
		: top_(0)
		, bottom_(0)
		, jobs_{}
	{
	}

	//可能不准,但是无影响
	bool IsEmpty() const { return top_ >= bottom_; }

	void Push(T job)
	{
		auto bottom = bottom_.load(std::memory_order_acquire);
		jobs_[bottom++ & kMask] = job;
		bottom_.store(bottom,std::memory_order_release);
	}

	T Pop()
	{
		auto bottom = bottom_.fetch_sub(1, std::memory_order_acquire);
		auto top = top_.load(std::memory_order_acquire);
		if(bottom <= top)
		{
			// 恢复提前减去的
			bottom_.store(bottom, std::memory_order_release);
			return nullptr;
		}

		auto job = jobs_[--bottom & kMask];
		if (top != bottom)
		{
			//队列中有多个job,可以直接返回
			//不会跟Steal()中有并发竞争(steal()不会被多个线程并发)
			return job;
		}

		// 工作队列被其他并发线程窃取
		if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_release))
		{
			//CAS 成功了，我们赢得了与Steal()的比赛。
			//在这种情况下，我们设置bottom = top + 1将双端队列设置为规范的空状态。

			//CAS 失败了，我们输掉了与Steal()的比赛。
			//在这种情况下，我们返回一个空作业，但仍设置bottom = top + 1。
			//为什么？因为输掉比赛意味着并发的Steal()操作成功设置了top = top + 1，
			//所以我们仍然必须将双端队列设置为空状态。
			job = nullptr;
		}

		bottom_.store(top + 1, std::memory_order_release);
		return job;
	}

	T Steal()
	{
		auto top = top_.load(std::memory_order_acquire);
		auto bottom = bottom_.load(std::memory_order_acquire);
		
		if(bottom <= top)
		{
			return nullptr;
		}

		auto job = jobs_[top & kMask];

		// 工作队列被其他并发线程窃取
		if(!top_.compare_exchange_strong(top,top + 1,std::memory_order_release))
		{
			return nullptr;
		}

		return job;
	}
private:
	std::atomic_int32_t top_;
	std::atomic_int32_t bottom_;
	T jobs_[N];
};