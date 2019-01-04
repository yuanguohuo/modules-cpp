#include <iostream>

#include "foo.hpp"
#include "../dump.hpp"

int main()
{
  std::cout<<"sizeof(Foo):"<<sizeof(Foo)<<std::endl;
  //output:
  //  sizeof(Foo):88

  {
    //f1 is on stack, so it's not allocated from pool; but its members 'pf' and 'pi' are allocated;
    Foo f1(2);
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:16
    //  items:4
    //  total-bytes:16 total-items:4
    //  3Foo: [0 bytes, 0 items]
    //  f: [8 bytes, 2 items]
    //  i: [8 bytes, 2 items]
    
    //f1's member 'v1' is a mempool::module_foo container; 
    f1.v1.push_back(10);
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:20
    //  items:5
    //  total-bytes:20 total-items:5
    //  3Foo: [0 bytes, 0 items]
    //  f: [8 bytes, 2 items]
    //  i: [12 bytes, 3 items]

    //f1's member 'v2' is NOT a mempool::module_foo container; 
    f1.v2.push_back(10);
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:20
    //  items:5
    //  total-bytes:20 total-items:5
    //  3Foo: [0 bytes, 0 items]
    //  f: [8 bytes, 2 items]
    //  i: [12 bytes, 3 items]
  }

  //all objects allocated are deallocated now;
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:0
  //  items:0
  //  total-bytes:0 total-items:0
  //  3Foo: [0 bytes, 0 items]
  //  f: [0 bytes, 0 items]
  //  i: [0 bytes, 0 items]

  //f2 is allocated from pool, so do its members 'pf' and 'pi';
  Foo* f2 = new Foo(3);
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:112
  //  items:7
  //  total-bytes:112 total-items:7
  //  3Foo: [88 bytes, 1 items]
  //  f: [12 bytes, 3 items]
  //  i: [12 bytes, 3 items]

  //f2's member 'v1' is a mempool::module_foo container; 
  f2->v1.push_back(100);
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:116
  //  items:8
  //  total-bytes:116 total-items:8
  //  3Foo: [88 bytes, 1 items]
  //  f: [12 bytes, 3 items]
  //  i: [16 bytes, 4 items]

  //f2's member 'v2' is NOT a mempool::module_foo container; 
  f2->v2.push_back(100);
  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:116
  //  items:8
  //  total-bytes:116 total-items:8
  //  3Foo: [88 bytes, 1 items]
  //  f: [12 bytes, 3 items]
  //  i: [16 bytes, 4 items]

  delete f2;

  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:0
  //  items:0
  //  total-bytes:0 total-items:0
  //  3Foo: [0 bytes, 0 items]
  //  f: [0 bytes, 0 items]
  //  i: [0 bytes, 0 items]

  return 0;
}
