#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <assert.h>

//#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */

#include <string.h>
#include <iostream>
#include <sstream>
#include <deque>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include "threadpool_impl.hpp"

void ThreadPoolImpl::PthreadCall(const char* label, int result)
{
  if (result != 0)
  {
    fprintf(stderr, "pthread %s: %s\n", label, strerror(result));
    abort();
  }
}

struct ThreadPoolImpl::Impl
{
  public:
    Impl();
    ~Impl();

    void Submit(std::function<void()>&& schedule,	std::function<void()>&& unschedule, void* tag);
    int UnSchedule(void* tag);
    unsigned int GetQueueLen() const { return queue_len_.load(std::memory_order_relaxed); }

    void BGThread(size_t thread_id);
    void StartBGThreads();
    void WakeUpAllThreads() { bgsignal_.notify_all(); }
    void JoinThreads(bool wait_for_jobs_to_complete);

    void SetBackgroundThreadsInternal(int num, bool allow_reduce);
    int GetBackgroundThreads();

    void LowerIOPriority();
    void LowerCPUPriority();

    bool HasExcessiveThread() const { return static_cast<int>(bgthreads_.size()) > total_threads_limit_; }
    bool IsLastExcessiveThread(size_t thread_id) const { return HasExcessiveThread() && thread_id == bgthreads_.size() - 1; }
    bool IsExcessiveThread(size_t thread_id) const { return static_cast<int>(thread_id) >= total_threads_limit_; }

  private:
    static void* BGThreadWrapper(void* arg);

    bool low_io_priority_;
    bool low_cpu_priority_;

    int total_threads_limit_;
    bool exit_all_threads_;
    bool wait_for_jobs_to_complete_;

    struct BGItem
    {
      void* tag = nullptr;
      std::function<void()> function;
      std::function<void()> unschedFunction;
    };

    using BGQueue = std::deque<BGItem>;
    BGQueue       queue_;
    std::atomic_uint queue_len_;

    std::mutex               mu_;
    std::condition_variable  bgsignal_;
    std::vector<std::thread> bgthreads_;
};

inline
ThreadPoolImpl::Impl::Impl() :
  low_io_priority_(false),
  low_cpu_priority_(false),
  total_threads_limit_(0),
  exit_all_threads_(false),
  wait_for_jobs_to_complete_(false),
  queue_(),
  queue_len_(),
  mu_(),
  bgsignal_(),
  bgthreads_()
{}

inline
ThreadPoolImpl::Impl::~Impl()
{
  assert(bgthreads_.size() == 0U);
}

void ThreadPoolImpl::Impl::Submit(std::function<void()>&& schedule,	std::function<void()>&& unschedule, void* tag)
{
  std::lock_guard<std::mutex> lock(mu_);

  if (exit_all_threads_)
  {
    return;
  }

  StartBGThreads();

  queue_.push_back(BGItem());

  auto& item = queue_.back();
  item.tag             = tag;
  item.function        = std::move(schedule);
  item.unschedFunction = std::move(unschedule);

  queue_len_.store(static_cast<unsigned int>(queue_.size()), std::memory_order_relaxed);

  //an excessive thread will:
  //  1. exit if it's "the last excessive thread";
  //  2. continue to wait if it's NOT "the last excessive thread";
  //when it is notified, see function BGThread(); that's to say, an excessive will not do our newly-submitted job.
  //if there are excessive threads and if we notify only one thread, the notified thread might be an excessive thread,
  //and it will not do our job; as a result, we notify all thread in thise case; 
  if (!HasExcessiveThread())
  {
    bgsignal_.notify_one();
  }
  else
  {
    WakeUpAllThreads();
  }
}

int ThreadPoolImpl::Impl::UnSchedule(void* tag)
{
  int count = 0;
  std::vector<std::function<void()>> candidates;

  {
    std::lock_guard<std::mutex> lock(mu_);

    BGQueue::iterator it = queue_.begin();
    while (it != queue_.end())
    {
      if (tag == (*it).tag)
      {
        if (it->unschedFunction)
        {
          candidates.push_back(std::move(it->unschedFunction));
        }
        it = queue_.erase(it);
        count++;
      }
      else
      {
        ++it;
      }
    }
    queue_len_.store(static_cast<unsigned int>(queue_.size()), std::memory_order_relaxed);
  }

  for(auto& f : candidates)
  {
    f();
  }

  return count;
}

void ThreadPoolImpl::Impl::BGThread(size_t thread_id)
{
	bool low_io_priority = false;
	bool low_cpu_priority = false;

  //"excessive threads" appears when:
  //   1. "total_threads_limit_" is decremented by
  //      ThreadPoolImpl::SetBackgroundThreads(int num) ---->
  //      ThreadPoolImpl::Impl::SetBackgroundThreadsInternal(num, true)
  //   2. JoinThreads, where total_threads_limit_ is set to 0;
  //in these cases, the "excessive threads" will terminate one by one, from "the last excessive thead" to the first one.

	while (true)
  {
		std::unique_lock<std::mutex> lock(mu_);

    //current thread will continue to wait if either of the conditions holds (break the condition into two);
    //  1. not exit all threads AND not "the last excessive thread" AND queue is empty. 
    //or
    //  2. not exit all threads AND not "the last excessive thread" AND is an excessive thread
    //
    //in the other perspective, current thread will continue to run if:
    //  1. exit all threads (to run so that current thread can do the jobs, if wait_for_jobs_to_complete_, and then exit)
    //or
    //  2. is "the last excessive thread" (to run so that current thread has a chance to exit)
    //or
    //  3. queue is not empty AND is an excessive thread (to run so that current thread can do the jobs)
		while (!exit_all_threads_ && !IsLastExcessiveThread(thread_id) && (queue_.empty() || IsExcessiveThread(thread_id)))
    {
			bgsignal_.wait(lock);
		}

		if (exit_all_threads_)
    {
      //no jobs or no need to complete them;
			if (!wait_for_jobs_to_complete_ || queue_.empty())
      {
				break;
			}
		} 
    else if (IsLastExcessiveThread(thread_id))
    {
      //current thread is "the last excessive thread", exit.
      //excessive threads terminate in the reverse order of generation, that is "the last excessive thread" terminates
      //first, then the one before it becomes "the last excessive thread" then terminates, and so forth... and when the
      //last terminates, it wakes up others, so that the one before it can terminate.
      std::cout << "last excessive thread " << thread_id << " is about to exit ..." << std::endl;
			auto& terminating_thread = bgthreads_.back();
			terminating_thread.detach();
			bgthreads_.pop_back();

			if (HasExcessiveThread())
      {
				WakeUpAllThreads();
			}

      std::cout << "last excessive thread " << thread_id << " exited" << std::endl;
			break;
		}

    //do one job
		auto func = std::move(queue_.front().function);
		queue_.pop_front();

		queue_len_.store(static_cast<unsigned int>(queue_.size()), std::memory_order_relaxed);

    //Yuanguo: it seems there is a bug here,
    //   when we change low_cpu_priority_ from false to true, the CPU priority is decreased as expected; but when we
    //   change it from true to false, the CPU priority is decreased again, instead of increased; and it's the same 
    //   for io priority;
    //Yuanguo: ok, there is only LowerCPUPriority/LowerIOPriority, no HigherCPUPriority/HigherIOPriority.
		bool decrease_io_priority = (low_io_priority != low_io_priority_);
		bool decrease_cpu_priority = (low_cpu_priority != low_cpu_priority_);
		lock.unlock();

    if (decrease_cpu_priority)
    {
      setpriority(PRIO_PROCESS, 
                  0,    //0 means the current thread;
                  19);  //19 means the lowest priority possible;
      low_cpu_priority = true;
    }

		if (decrease_io_priority)
    {
#define IOPRIO_CLASS_SHIFT (13)
#define IOPRIO_PRIO_VALUE(class, data) (((class) << IOPRIO_CLASS_SHIFT) | data)
			// Put schedule into IOPRIO_CLASS_IDLE class (lowest)
			// These system calls only have an effect when used in conjunction
			// with an I/O scheduler that supports I/O priorities. As at
			// kernel 2.6.17 the only such scheduler is the Completely
			// Fair Queuing (CFQ) I/O scheduler.
			// To change scheduler:
			//  echo cfq > /sys/block/<device_name>/queue/scheduler
			// Tunables to consider:
			//  /sys/block/<device_name>/queue/slice_idle
			//  /sys/block/<device_name>/queue/slice_sync
			syscall(SYS_ioprio_set, 
              1,  //IOPRIO_WHO_PROCESS
					    0,  //0 means current thread
					    IOPRIO_PRIO_VALUE(3, 0));
			low_io_priority = true;
		}
		func();
	}
}

struct BGThreadMetadata
{
  ThreadPoolImpl::Impl* thread_pool_;
  size_t thread_id_;

  BGThreadMetadata(ThreadPoolImpl::Impl* thread_pool, size_t thread_id) : 
    thread_pool_(thread_pool), 
    thread_id_(thread_id)
  {}
};

void* ThreadPoolImpl::Impl::BGThreadWrapper(void* arg)
{
  BGThreadMetadata* meta = reinterpret_cast<BGThreadMetadata*>(arg);
  size_t thread_id = meta->thread_id_;
  ThreadPoolImpl::Impl* tp = meta->thread_pool_;
  delete meta;
  tp->BGThread(thread_id);
  return nullptr;
}

void ThreadPoolImpl::Impl::StartBGThreads()
{
  //assume that total_threads_limit_ = bgthreads_.size() = 10,  current threads are 0,1,...9;
  //if total_threads_limit_ is increased to 15, then threads 10,11,...,14 are created/started;
  while ((int)bgthreads_.size() < total_threads_limit_)
  {
    std::thread p_t(&BGThreadWrapper, new BGThreadMetadata(this, bgthreads_.size()));

    // Set the thread name to aid debugging
#if defined(_GNU_SOURCE) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 12)
    auto th_handle = p_t.native_handle();
    std::ostringstream thread_name_stream;
    thread_name_stream << "thread-" << bgthreads_.size();
    pthread_setname_np(th_handle, thread_name_stream.str().c_str());
#endif
#endif

    bgthreads_.push_back(std::move(p_t));
  }
}

void ThreadPoolImpl::Impl::JoinThreads(bool wait_for_jobs_to_complete)
{
  std::unique_lock<std::mutex> lock(mu_);
  //exit_all_threads_ must be false; if it is true, we are alreadying joining, it's illegal to join again.
  assert(!exit_all_threads_);

  wait_for_jobs_to_complete_ = wait_for_jobs_to_complete;
  exit_all_threads_ = true;  //we are joining...

  //prevent threads from being recreated right after they're joined, in case
  //the user is concurrently submitting jobs.
  //Yuanguo: it seems there is a bug here, when we set total_threads_limit_ to 0, all threads become "excessive", and
  //  they will terminate one by one. if there are "a lot" jobs, it is possible that all threads have terminated without
  //  finishing the jobs;
  total_threads_limit_ = 0;

  lock.unlock();

  bgsignal_.notify_all();

  for (auto& th : bgthreads_) {
    th.join();
  }

  bgthreads_.clear();

  exit_all_threads_ = false;
  wait_for_jobs_to_complete_ = false;
}

void ThreadPoolImpl::Impl::SetBackgroundThreadsInternal(int num, bool allow_reduce)
{
  std::unique_lock<std::mutex> lock(mu_);
  if (exit_all_threads_)
  {
    lock.unlock();
    return;
  }

  if (num > total_threads_limit_ || (num < total_threads_limit_ && allow_reduce))
  {
    total_threads_limit_ = std::max(0, num);
    WakeUpAllThreads();  //decreased: some threads become excessive now, let them terminate ...
    StartBGThreads();    //increased: create some more threads ...
  }
}

int ThreadPoolImpl::Impl::GetBackgroundThreads()
{
  std::unique_lock<std::mutex> lock(mu_);
  return total_threads_limit_;
}

void ThreadPoolImpl::Impl::LowerIOPriority()
{
  std::lock_guard<std::mutex> lock(mu_);
  low_io_priority_ = true;
}
void ThreadPoolImpl::Impl::LowerCPUPriority()
{
  std::lock_guard<std::mutex> lock(mu_);
  low_cpu_priority_ = true;
}

ThreadPoolImpl::ThreadPoolImpl() : impl_(new Impl()) {}
ThreadPoolImpl::~ThreadPoolImpl() {}

void ThreadPoolImpl::SetBackgroundThreads(int num) { impl_->SetBackgroundThreadsInternal(num, true); }
void ThreadPoolImpl::IncBackgroundThreadsIfNeeded(int num ) { impl_->SetBackgroundThreadsInternal(num, false); }
int ThreadPoolImpl::GetBackgroundThreads() { return impl_->GetBackgroundThreads(); }
unsigned int ThreadPoolImpl::GetQueueLen() const { return impl_->GetQueueLen(); }
void ThreadPoolImpl::SubmitJob(const std::function<void()>& job)
{
  auto copy(job);
  impl_->Submit(std::move(copy), std::function<void()>(), nullptr);
}
void ThreadPoolImpl::SubmitJob(std::function<void()>&& job)
{
  impl_->Submit(std::move(job), std::function<void()>(), nullptr);
}
void ThreadPoolImpl::JoinAllThreads() { impl_->JoinThreads(false); }
void ThreadPoolImpl::WaitForJobsAndJoinAllThreads() { impl_->JoinThreads(true); }

void ThreadPoolImpl::LowerIOPriority() { impl_->LowerIOPriority(); }
void ThreadPoolImpl::LowerCPUPriority() { impl_->LowerCPUPriority(); }

void ThreadPoolImpl::Schedule(void (*function)(void* arg1), void* arg, void* tag, void (*unschedFunction)(void* arg))
{
  if (unschedFunction == nullptr)
  {
    impl_->Submit(std::bind(function, arg), std::function<void()>(), tag);
  }
  else
  {
    impl_->Submit(std::bind(function, arg), std::bind(unschedFunction, arg), tag);
  }
}
int ThreadPoolImpl::UnSchedule(void* tag)
{
  return impl_->UnSchedule(tag);
}

ThreadPool* NewThreadPool(int num_threads)
{
  ThreadPoolImpl* thread_pool = new ThreadPoolImpl();
  thread_pool->SetBackgroundThreads(num_threads);
  return thread_pool;
}
