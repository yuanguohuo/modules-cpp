#include <unistd.h>
#include <iostream>
#include "../threadpool.hpp"

void job(int jid)
{
  usleep(100000);
  std::cout << "job " << jid << std::endl;
}

int main()
{
  ThreadPool* pool = NewThreadPool(4);

  for (int i=0; i<16; i++)
  {
    pool->SubmitJob(std::bind(job, i));
  }

  pool->WaitForJobsAndJoinAllThreads();
  delete pool;

  return 0;
}
