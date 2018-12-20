#include <vector>
#include "../../mempool.hpp"

class Foo 
{
  public:
    MEMPOOL_CLASS_HELPERS();

    Foo();

    int a;
    std::vector<int> v;
};
