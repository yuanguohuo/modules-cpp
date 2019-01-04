#include <iostream>

#include "../../mempool.hpp"
#include "../dump.hpp"

MEMPOOL_DECLARE_FACTORY(double, double, module_foo)
MEMPOOL_DECLARE_FACTORY(char, char, module_foo)

MEMPOOL_DEFINE_FACTORY(double, double, module_foo)
MEMPOOL_DEFINE_FACTORY(char, char, module_foo)

int main()
{
  double * d = mempool::module_foo::alloc_double.allocate(3);
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:24
  //  items:3
  //  total-bytes:24 total-items:3
  //  c: [0 bytes, 0 items]
  //  d: [24 bytes, 3 items]

  char * s1 = mempool::module_foo::alloc_char.allocate(4);
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:28
  //  items:7
  //  total-bytes:28 total-items:7
  //  c: [4 bytes, 4 items]
  //  d: [24 bytes, 3 items]

  char * s2 = mempool::module_foo::alloc_char.allocate_aligned(3, 8);
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:31
  //  items:10
  //  total-bytes:31 total-items:10
  //  c: [7 bytes, 7 items]
  //  d: [24 bytes, 3 items]

  mempool::module_foo::alloc_char.deallocate_aligned(s2, 3);
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:28
  //  items:7
  //  total-bytes:28 total-items:7
  //  c: [4 bytes, 4 items]
  //  d: [24 bytes, 3 items]


  mempool::module_foo::alloc_char.deallocate(s1, 4);
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:24
  //  items:3
  //  total-bytes:24 total-items:3
  //  c: [0 bytes, 0 items]
  //  d: [24 bytes, 3 items]

  mempool::module_foo::alloc_double.deallocate(d, 3);
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:0
  //  items:0
  //  total-bytes:0 total-items:0
  //  c: [0 bytes, 0 items]
  //  d: [0 bytes, 0 items]

  return 0;
}
