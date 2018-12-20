#include "../../mempool.hpp"
class Foo 
{
  public:
    MEMPOOL_CLASS_HELPERS();

    Foo();

    int a;
    mempool::module_foo::vector<int> v;
};
