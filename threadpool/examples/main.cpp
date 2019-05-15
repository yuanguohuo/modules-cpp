#include <iostream>
#include <unistd.h>
#include <thread>

void* f(void* arg)
{
  for (int i=0;i<10;i++)
  {
    std::cout << i << std::endl;
    usleep(100*1000);
  }
  return nullptr ;
}

int main()
{
    std::thread t(f, nullptr);
    t.detach();
    t.join();
    return 0;
}
