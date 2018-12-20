#ifndef __MEMPOOL_HPP__
#define __MEMPOOL_HPP__

#include <map>
#include <set>
#include <list>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <typeinfo>
#include <assert.h>

#define NUM_SHARD_BITS 5
#define NUM_SHARDS (1<<NUM_SHARD_BITS)

namespace mempool
{
extern bool debug_mode;
void set_debug_mode(bool mode);


#define DEFINE_MEMORY_POOLS_HELPER(f)   \
  f(module_foo)                         \
  f(unittest1)                          \
  f(unittest2)                          \

#define P(x) mempool_##x,
  enum pool_index_t
  {
    DEFINE_MEMORY_POOLS_HELPER(P)
    num_pools
  };
#undef P

  struct shard_t
  {
    std::atomic<size_t> bytes = {0};
    std::atomic<size_t> items = {0};
    char __padding[128 - sizeof(std::atomic<size_t>)*2];
  } __attribute__ ((aligned (128)));

  static_assert(sizeof(shard_t) == 128, "shard_t should be cacheline-sized");

  struct stats_t
  {
    ssize_t items = 0;
    ssize_t bytes = 0;

    stats_t& operator+=(const stats_t& o)
    {
      items += o.items;
      bytes += o.bytes;
      return *this;
    }
  };

  struct type_t
  {
    const char *type_name;
    size_t item_size;
    std::atomic<ssize_t> items = {0};  // signed
  };

  struct type_info_hash
  {
    std::size_t operator()(const std::type_info& k) const
    {
      return k.hash_code();
    }
  };

  class pool_t
  {
    shard_t shard[NUM_SHARDS];

    mutable std::mutex lock;  // only used for type_map; 
    std::unordered_map<const char*, type_t> type_map;

    public:
    // How much this pool consumes. O(<num_shards>)
    size_t allocated_bytes() const;
    size_t allocated_items() const;

    void adjust_count(ssize_t items, ssize_t bytes);

    shard_t* pick_a_shard()
    {
      size_t me = (size_t)pthread_self();
      size_t i = (me >> 3) & ((1 << NUM_SHARD_BITS) - 1);
      return &shard[i];
    }

    type_t *get_type(const std::type_info& ti, size_t size)
    {
      std::lock_guard<std::mutex> l(lock);
      auto p = type_map.find(ti.name());
      if (p != type_map.end())
      {
        return &p->second;
      }
      type_t &t = type_map[ti.name()];
      t.type_name = ti.name();
      t.item_size = size;
      return &t;
    }
    void get_stats(stats_t *total, std::map<std::string, stats_t> *by_type) const;
  };

  pool_t& get_pool(pool_index_t ix);
  const char *get_pool_name(pool_index_t ix);

  template<pool_index_t pool_ix, typename T>
  class pool_allocator
  {
    pool_t *pool;
    type_t *type = nullptr;

    public:
    typedef pool_allocator<pool_ix, T> allocator_type;
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type * const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template<typename U> struct rebind
    {
      typedef pool_allocator<pool_ix,U> other;
    };

    void init(bool force_register)
    {
      pool = &get_pool(pool_ix);
      if (debug_mode || force_register)
      {
        type = pool->get_type(typeid(T), sizeof(T));
      }
    }

    pool_allocator(bool force_register=false)
    {
      init(force_register);
    }

    template<typename U>
    pool_allocator(const pool_allocator<pool_ix,U>&)
    {
      init(false);
    }

    T* allocate(size_t n, void *p = nullptr)
    {
      size_t total = sizeof(T) * n;
      shard_t *shard = pool->pick_a_shard();
      shard->bytes += total;
      shard->items += n;
      if (type)
      {
        type->items += n;
      }
      T* r = reinterpret_cast<T*>(new char[total]);
      return r;
    }

    void deallocate(T* p, size_t n)
    {
      size_t total = sizeof(T) * n;
      shard_t *shard = pool->pick_a_shard();
      shard->bytes -= total;
      shard->items -= n;
      if (type)
      {
        type->items -= n;
      }

      //memory allocated by new should be reclaimed by delete;
      delete[] reinterpret_cast<char*>(p);
    }

    T* allocate_aligned(size_t n, size_t align, void *p = nullptr)
    {
      size_t total = sizeof(T) * n;
      shard_t *shard = pool->pick_a_shard();
      shard->bytes += total;
      shard->items += n;
      if (type)
      {
        type->items += n;
      }

      char *ptr;
      int rc = ::posix_memalign((void**)(void*)&ptr, align, total);
      if (rc)
      {
        throw std::bad_alloc();
      }

      T* r = reinterpret_cast<T*>(ptr);
      return r;
    }

    void deallocate_aligned(T* p, size_t n)
    {
      size_t total = sizeof(T) * n;
      shard_t *shard = pool->pick_a_shard();
      shard->bytes -= total;
      shard->items -= n;
      if (type)
      {
        type->items -= n;
      }

      //memory allocated by posix_memalign should be reclaimed by free;
      ::free(p);
    }

    void destroy(T* p)
    {
      p->~T();
    }

    template<class U>
    void destroy(U *p)
    {
      p->~U();
    }

    void construct(T* p, const T& val)
    {
      ::new ((void *)p) T(val);
    }

    template<class U, class... Args> void construct(U* p,Args&&... args)
    {
      ::new((void *)p) U(std::forward<Args>(args)...);
    }

    bool operator==(const pool_allocator&) const { return true; }
    bool operator!=(const pool_allocator&) const { return false; }
  };


#define P(x)                                                              \
  namespace x                                                             \
  {                                                                       \
    static const mempool::pool_index_t id = mempool::mempool_##x;         \
                                                                          \
    template<typename v>                                                  \
    using pool_allocator = mempool::pool_allocator<id,v>;                 \
                                                                          \
    using string = std::basic_string<                                     \
                         char,                                            \
                         std::char_traits<char>,                          \
                         pool_allocator<char>                             \
                        >;                                                \
                                                                          \
    template<typename k,typename v, typename cmp = std::less<k> >         \
    using map = std::map<                                                 \
                         k,                                               \
                         v,                                               \
                         cmp,                                             \
                         pool_allocator<std::pair<const k,v>>             \
                        >;                                                \
                                                                          \
    template<typename k,typename v, typename cmp = std::less<k> >         \
    using multimap = std::multimap<                                       \
                         k,                                               \
                         v,                                               \
                         cmp,                                             \
                         pool_allocator<std::pair<const k, v>>            \
                        >;                                                \
                                                                          \
    template<typename k, typename cmp = std::less<k> >                    \
    using set = std::set<                                                 \
                         k,                                               \
                         cmp,                                             \
                         pool_allocator<k>                                \
                        >;                                                \
                                                                          \
    template<typename v>                                                  \
    using list = std::list<                                               \
                         v,                                               \
                         pool_allocator<v>                                \
                        >;                                                \
                                                                          \
    template<typename v>                                                  \
    using vector = std::vector<                                           \
                         v,                                               \
                         pool_allocator<v>                                \
                        >;                                                \
                                                                          \
    template<typename k,                                                  \
             typename v,                                                  \
             typename h=std::hash<k>,                                     \
             typename eq = std::equal_to<k>>                              \
    using unordered_map = std::unordered_map<                             \
                         k,                                               \
                         v,                                               \
                         h,                                               \
                         eq,                                              \
                         pool_allocator<std::pair<const k,v>>             \
                        >;                                                \
                                                                          \
    inline size_t allocated_bytes() {                                     \
      return mempool::get_pool(id).allocated_bytes();                     \
    }                                                                     \
    inline size_t allocated_items() {                                     \
      return mempool::get_pool(id).allocated_items();                     \
    }                                                                     \
  };
DEFINE_MEMORY_POOLS_HELPER(P)
#undef P
};

#include "builtin_helper.hpp"
#include "class_helper.hpp"

#endif
