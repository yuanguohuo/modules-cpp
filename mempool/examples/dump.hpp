static void dump_pool(mempool::pool_index_t pool_id)
{
  std::cout<<"====================="<<mempool::get_pool_name(pool_id)<<"====================="<<std::endl;
  mempool::pool_t *pool = &mempool::get_pool(pool_id);

  mempool::stats_t total;
  std::map<std::string,mempool::stats_t> m;
  pool->get_stats(&total, &m);
  size_t usage = pool->allocated_bytes();
  size_t items = pool->allocated_items();

  std::cout<<"usage:"<<usage<<std::endl;
  std::cout<<"items:"<<items<<std::endl;
  std::cout<<"total-bytes:"<<total.bytes<<" total-items:"<<total.items<<std::endl;

  for (std::map<std::string,mempool::stats_t>::const_iterator itr=m.begin(); itr!=m.end();++itr)
  {
    std::cout<<itr->first<<": ["<<itr->second.bytes<<" bytes, "<<itr->second.items<<" items]"<<std::endl;
  }
}
