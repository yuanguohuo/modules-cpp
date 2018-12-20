#ifndef __MEMPOOL_CLASS_HELPER_HPP__
#define __MEMPOOL_CLASS_HELPER_HPP__

#define MEMPOOL_CLASS_HELPERS()                       \
  void *operator new(size_t size);                    \
  void *operator new[](size_t size) noexcept          \
  {                                                   \
    assert(0 == "no array new");                      \
    return nullptr;                                   \
  }                                                   \
  void  operator delete(void *);                      \
  void  operator delete[](void *)                     \
  {                                                   \
    assert(0 == "no array delete");                   \
  }

#define MEMPOOL_DEFINE_OBJECT_FACTORY(obj,factoryname,pool)            \
  namespace mempool                                                    \
  {                                                                    \
    namespace pool                                                     \
    {                                                                  \
      pool_allocator<obj> alloc_##factoryname = {true};                \
    }                                                                  \
  }                                                                    \
  void *obj::operator new(size_t size)                                 \
  {                                                                    \
    return mempool::pool::alloc_##factoryname.allocate(1);             \
  }                                                                    \
  void obj::operator delete(void *p)                                   \
  {                                                                    \
    return mempool::pool::alloc_##factoryname.deallocate((obj*)p, 1);  \
  }

#endif
