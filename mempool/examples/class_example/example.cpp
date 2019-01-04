#include <iostream>

#include "foo.hpp"
#include "../dump.hpp"

int main()
{
  std::cout<<"sizeof(Foo):"<<sizeof(Foo)<<std::endl;
  //output:
  //  sizeof(Foo):32

  //f1 is on stack, so there is no allocation;
  Foo f1;
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:0
  //  items:0
  //  total-bytes:0 total-items:0
  //  3Foo: [0 bytes, 0 items]

  //f1 uses std container types, rather than mempool::module_foo container types, so there is no allocation;
  f1.v.push_back(100);
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:0
  //  items:0
  //  total-bytes:0 total-items:0
  //  3Foo: [0 bytes, 0 items]

  //f2 is allocated from pool;
  Foo* f2 = new Foo();
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:32
  //  items:1
  //  total-bytes:32 total-items:1
  //  3Foo: [32 bytes, 1 items]

  //f2 uses std container types, rather than mempool::module_foo container types, so there is no allocation;
  f2->v.push_back(200);
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:32
  //  items:1
  //  total-bytes:32 total-items:1
  //  3Foo: [32 bytes, 1 items]
 
  delete f2;

  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:0
  //  items:0
  //  total-bytes:0 total-items:0
  //  3Foo: [0 bytes, 0 items]

  return 0;
}
