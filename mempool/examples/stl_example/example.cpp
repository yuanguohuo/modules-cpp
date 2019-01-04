#include <iostream>

#include "../../mempool.hpp"
#include "../dump.hpp"

int main()
{
  {
    mempool::module_foo::vector<char> v;
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:0
    //  items:0
    //  total-bytes:0 total-items:0
    //  c: [0 bytes, 0 items]

    v.push_back('a');
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:1
    //  items:1
    //  total-bytes:1 total-items:1
    //  c: [1 bytes, 1 items]
  }

  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:0
  //  items:0
  //  total-bytes:0 total-items:0
  //  c: [0 bytes, 0 items]

  {
    mempool::module_foo::map<char,int> m;
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:0
    //  items:0
    //  total-bytes:0 total-items:0
    //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
    //  c: [0 bytes, 0 items]

    m['a'] = 97;
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:40
    //  items:1
    //  total-bytes:40 total-items:1
    //  St13_Rb_tree_nodeISt4pairIKciEE: [40 bytes, 1 items]
    //  c: [0 bytes, 0 items]

    m['b'] = 98;
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:80
    //  items:2
    //  total-bytes:80 total-items:2
    //  St13_Rb_tree_nodeISt4pairIKciEE: [80 bytes, 2 items]
    //  c: [0 bytes, 0 items]

    m['b'] = 98;   //insert duplicated key, no extra space allocated;
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:80
    //  items:2
    //  total-bytes:80 total-items:2
    //  St13_Rb_tree_nodeISt4pairIKciEE: [80 bytes, 2 items]
    //  c: [0 bytes, 0 items]
  }

  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:0
  //  items:0
  //  total-bytes:0 total-items:0
  //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
  //  c: [0 bytes, 0 items]

  {
    mempool::module_foo::set<int> s;
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:0
    //  items:0
    //  total-bytes:0 total-items:0
    //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
    //  St13_Rb_tree_nodeIiE: [0 bytes, 0 items]
    //  c: [0 bytes, 0 items]

    s.insert(10);
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:40
    //  items:1
    //  total-bytes:40 total-items:1
    //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
    //  St13_Rb_tree_nodeIiE: [40 bytes, 1 items]
    //  c: [0 bytes, 0 items]

    s.insert(20);
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:80
    //  items:2
    //  total-bytes:80 total-items:2
    //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
    //  St13_Rb_tree_nodeIiE: [80 bytes, 2 items]
    //  c: [0 bytes, 0 items]

    s.insert(10);   //insert duplicated element, no extra space allocated;
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:80
    //  items:2
    //  total-bytes:80 total-items:2
    //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
    //  St13_Rb_tree_nodeIiE: [80 bytes, 2 items]
    //  c: [0 bytes, 0 items]
  }

  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:0
  //  items:0
  //  total-bytes:0 total-items:0
  //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
  //  St13_Rb_tree_nodeIiE: [0 bytes, 0 items]
  //  c: [0 bytes, 0 items]

  {
    mempool::module_foo::unordered_map<int,char> m1;
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:88
    //  items:11
    //  total-bytes:88 total-items:11
    //  NSt8__detail10_Hash_nodeISt4pairIKicELb0EEE: [0 bytes, 0 items]
    //  PNSt8__detail15_Hash_node_baseE: [88 bytes, 11 items]
    //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
    //  St13_Rb_tree_nodeIiE: [0 bytes, 0 items]
    //  St4pairIKicE: [0 bytes, 0 items]
    //  c: [0 bytes, 0 items]

    m1[97]='a';
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:104
    //  items:12
    //  total-bytes:104 total-items:12
    //  NSt8__detail10_Hash_nodeISt4pairIKicELb0EEE: [16 bytes, 1 items]
    //  PNSt8__detail15_Hash_node_baseE: [88 bytes, 11 items]
    //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
    //  St13_Rb_tree_nodeIiE: [0 bytes, 0 items]
    //  St4pairIKicE: [0 bytes, 0 items]
    //  c: [0 bytes, 0 items]

    m1[98]='b';
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:120
    //  items:13
    //  total-bytes:120 total-items:13
    //  NSt8__detail10_Hash_nodeISt4pairIKicELb0EEE: [32 bytes, 2 items]
    //  PNSt8__detail15_Hash_node_baseE: [88 bytes, 11 items]
    //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
    //  St13_Rb_tree_nodeIiE: [0 bytes, 0 items]
    //  St4pairIKicE: [0 bytes, 0 items]
    //  c: [0 bytes, 0 items]

    m1[97]='a';   //insert duplicated key, no extra space allocated;
    dump_pool(mempool::module_foo::id);
    //output:
    //  usage:120
    //  items:13
    //  total-bytes:120 total-items:13
    //  NSt8__detail10_Hash_nodeISt4pairIKicELb0EEE: [32 bytes, 2 items]
    //  PNSt8__detail15_Hash_node_baseE: [88 bytes, 11 items]
    //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
    //  St13_Rb_tree_nodeIiE: [0 bytes, 0 items]
    //  St4pairIKicE: [0 bytes, 0 items]
    //  c: [0 bytes, 0 items]
  }

  dump_pool(mempool::module_foo::id);
  //output:
  //  usage:0
  //  items:0
  //  total-bytes:0 total-items:0
  //  NSt8__detail10_Hash_nodeISt4pairIKicELb0EEE: [0 bytes, 0 items]
  //  PNSt8__detail15_Hash_node_baseE: [0 bytes, 0 items]
  //  St13_Rb_tree_nodeISt4pairIKciEE: [0 bytes, 0 items]
  //  St13_Rb_tree_nodeIiE: [0 bytes, 0 items]
  //  St4pairIKicE: [0 bytes, 0 items]
  //  c: [0 bytes, 0 items]

  return 0;
}
