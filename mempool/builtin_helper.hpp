#ifndef __MEMPOOL_TYPE_HELPER_HPP__
#define __MEMPOOL_TYPE_HELPER_HPP__

#define MEMPOOL_DECLARE_FACTORY(obj, factoryname, pool)      \
  namespace mempool                                          \
  {                                                          \
    namespace pool                                           \
    {                                                        \
      extern pool_allocator<obj> alloc_##factoryname;        \
    }                                                        \
  }


#define MEMPOOL_DEFINE_FACTORY(obj, factoryname, pool)       \
  namespace mempool                                          \
  {                                                          \
    namespace pool                                           \
    {                                                        \
      pool_allocator<obj> alloc_##factoryname = {true};      \
    }                                                        \
  }

#endif
