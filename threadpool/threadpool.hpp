#ifndef __THREAD_POOL_HPP__
#define __THREAD_POOL_HPP__

#include<functional>

class ThreadPool
{
  public:
    virtual ~ThreadPool() {}

    virtual void SetBackgroundThreads(int num) = 0;
    virtual int GetBackgroundThreads() = 0;

    virtual unsigned int GetQueueLen() const = 0;
    virtual void SubmitJob(const std::function<void()>&) = 0;
    virtual void SubmitJob(std::function<void()>&&) = 0;

    virtual void JoinAllThreads() = 0;
    virtual void WaitForJobsAndJoinAllThreads() = 0;
};

extern ThreadPool* NewThreadPool(int num_threads);

#endif
