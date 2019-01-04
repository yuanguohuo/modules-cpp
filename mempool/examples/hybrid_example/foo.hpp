#include <vector>
#include "../../mempool.hpp"

MEMPOOL_DECLARE_FACTORY(float, float, module_foo)
MEMPOOL_DECLARE_FACTORY(int, int, module_foo)

class Foo 
{
  public:
    MEMPOOL_CLASS_HELPERS();

    Foo(int n_ = 0);
    ~Foo();

    int num;
    float* pf;
    int* pi;
    mempool::module_foo::vector<int> v1;
    std::vector<int> v2;
};
