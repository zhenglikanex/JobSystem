#pragma once

#include <atomic>
#include <cassert>

class Job;

template<size_t N>
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

	bool IsEmpty() const { return top_ == bottom_; }

	void Push(Job* job)
	{
		//assert(bottom_ - top_ <= 4096);
		auto bottom = bottom_.load(std::memory_order_acquire);
		jobs_[bottom++ & kMask] = job;
		bottom_.store(bottom,std::memory_order_release);
	}

	Job* Pop()
	{
		auto bottom = bottom_.fetch_sub(1, std::memory_order_acquire);
		auto top = top_.load(std::memory_order_acquire);
		if(bottom <= top)
		{
			// �ָ���ǰ��ȥ��
			bottom_.store(bottom, std::memory_order_release);
			return nullptr;
		}

		auto job = jobs_[--bottom & kMask];
		if (top != bottom)
		{
			//�������ж��job,����ֱ�ӷ���
			//�����Steal()���в�������(steal()���ᱻ����̲߳���)
			return job;
		}

		// �������б����������߳���ȡ
		if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_release))
		{
			//CAS �ɹ��ˣ�����Ӯ������Steal()�ı�����
			//����������£���������bottom = top + 1��˫�˶�������Ϊ�淶�Ŀ�״̬��

			//CAS ʧ���ˣ������������Steal()�ı�����
			//����������£����Ƿ���һ������ҵ����������bottom = top + 1��
			//Ϊʲô����Ϊ���������ζ�Ų�����Steal()�����ɹ�������top = top + 1��
			//����������Ȼ���뽫˫�˶�������Ϊ��״̬��
			job = nullptr;
		}

		bottom_.store(top + 1, std::memory_order_release);
		return job;
	}

	Job* Steal()
	{
		auto top = top_.load(std::memory_order_acquire);
		auto bottom = bottom_.load(std::memory_order_acquire);
		
		if(bottom <= top)
		{
			return nullptr;
		}

		auto job = jobs_[top & kMask];

		// �������б����������߳���ȡ
		if(!top_.compare_exchange_strong(top,top + 1,std::memory_order_release))
		{
			return nullptr;
		}

		return job;
	}
private:
	std::atomic_uint32_t top_;
	std::atomic_uint32_t bottom_;
	Job* jobs_[N];
};