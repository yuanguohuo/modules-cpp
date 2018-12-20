#include <iostream>

#include "foo.hpp"
#include "../dump.hpp"

int main()
{
  std::cout<<"sizeof(Foo):"<<sizeof(Foo)<<std::endl;

  Foo f1;
  dump_pool(mempool::module_foo::id);
  
  f1.v.push_back(10);
  dump_pool(mempool::module_foo::id);

  Foo* f2 = new Foo();
  dump_pool(mempool::module_foo::id);

  f2->v.push_back(100);
  dump_pool(mempool::module_foo::id);

  f2->v.push_back(101);
  dump_pool(mempool::module_foo::id);

  delete f2;
  dump_pool(mempool::module_foo::id);

  return 0;
}
