#include "foo.hpp"

MEMPOOL_DEFINE_FACTORY(float, float, module_foo)
MEMPOOL_DEFINE_FACTORY(int, int, module_foo)

MEMPOOL_DEFINE_OBJECT_FACTORY(Foo,foo,module_foo);

Foo::Foo(int n) : num(n), pf(NULL), pi(NULL)
{
  if (num>0)
  {
    pf = mempool::module_foo::alloc_float.allocate(n);
    pi = mempool::module_foo::alloc_int.allocate(n);
  }
}

Foo::~Foo()
{
  if (pf != NULL)
  {
    //delete[] pf;    //this is wrong !
    mempool::module_foo::alloc_float.deallocate(pf, num);
    pf = NULL;
  }
  if (pi != NULL)
  {
    //delete pi;     //this is wrong !
    mempool::module_foo::alloc_int.deallocate(pi, num);
    pi = NULL;
  }
}
