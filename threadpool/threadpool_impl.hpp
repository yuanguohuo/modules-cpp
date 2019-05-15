#ifndef __THREAD_POOL_IMPL_HPP__
#define __THREAD_POOL_IMPL_HPP__

#include <memory>
#include "threadpool.hpp"

class ThreadPoolImpl : public ThreadPool
{
  public:
    static void PthreadCall(const char* label, int result);
    struct Impl;

  public:
    ThreadPoolImpl();
    ~ThreadPoolImpl();

    ThreadPoolImpl(ThreadPoolImpl&&) = delete;
    ThreadPoolImpl& operator=(ThreadPoolImpl&&) = delete;

    void SetBackgroundThreads(int num) override;
    int GetBackgroundThreads() override;
    unsigned int GetQueueLen() const override;
    void SubmitJob(const std::function<void()>&) override;
    void SubmitJob(std::function<void()>&&) override;
    void JoinAllThreads() override;
    void WaitForJobsAndJoinAllThreads() override;

    void IncBackgroundThreadsIfNeeded(int num);

    void LowerIOPriority();
    void LowerCPUPriority();

    void Schedule(void (*function)(void* arg1), void* arg, void* tag, void (*unschedFunction)(void* arg));
    int UnSchedule(void* tag);

  private:
    std::unique_ptr<Impl>   impl_;
};

#endif
