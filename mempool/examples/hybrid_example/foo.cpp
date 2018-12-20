#include "foo.hpp"

MEMPOOL_DEFINE_OBJECT_FACTORY(Foo,foo,module_foo);

Foo::Foo() : a(100)
{
}
