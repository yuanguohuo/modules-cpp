#include "mempool.hpp"

bool mempool::debug_mode = true;
void mempool::set_debug_mode(bool mode)
{
  debug_mode = mode;
}

mempool::pool_t& mempool::get_pool(mempool::pool_index_t ix)
{
  static mempool::pool_t table[num_pools];
  return table[ix];
}

const char* mempool::get_pool_name(mempool::pool_index_t ix)
{
#define P(x) #x,
  static const char *names[num_pools] =
  {
    DEFINE_MEMORY_POOLS_HELPER(P)
  };
#undef P
  return names[ix];
}

size_t mempool::pool_t::allocated_bytes() const
{
  ssize_t result = 0;
  for (size_t i = 0; i < NUM_SHARDS; ++i)
  {
    result += shard[i].bytes;
  }
  assert(result >= 0);
  return (size_t) result;
}

size_t mempool::pool_t::allocated_items() const
{
  ssize_t result = 0;
  for (size_t i = 0; i < NUM_SHARDS; ++i)
  {
    result += shard[i].items;
  }
  assert(result >= 0);
  return (size_t) result;
}

void mempool::pool_t::adjust_count(ssize_t items, ssize_t bytes)
{
  shard_t *shard = pick_a_shard();
  shard->items += items;
  shard->bytes += bytes;
}

void mempool::pool_t::get_stats(stats_t *total, std::map<std::string, stats_t> *by_type) const
{
  if (total!=NULL)
  {
    for (size_t i = 0; i < NUM_SHARDS; ++i)
    {
      total->items += shard[i].items;
      total->bytes += shard[i].bytes;
    }
  }

  if (debug_mode && (by_type!=NULL))
  {
    std::unique_lock<std::mutex> shard_lock(lock);
    for (auto &p : type_map)
    {
      std::string n = std::string(p.first);
      stats_t &s = (*by_type)[n];
      s.bytes = p.second.items * p.second.item_size;
      s.items = p.second.items;
    }
  }
}
