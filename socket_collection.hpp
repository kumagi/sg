#ifndef SOCKET_COLLECTION_HPP__
#define SOCKET_COLLECTION_HPP__
#include <boost/unordered_map.hpp>
#include <boost/noncopyable.hpp>
#include "machine.hpp"
/*
inline std::size_t hash_value(const machine_t& m){
  return boost::hash_value(m.ip()) + boost::hash_value(m.port());
}
*/

class socket_collection :private boost::noncopyable{
  boost::unordered_map<int,machine_t> sockets;
  typedef boost::unordered_map<int,machine_t>::iterator sock_iterator;
  typedef std::pair<int,machine_t> node;
  const machine_t dummy;
public:
  socket_collection():dummy(){};
  // searching
  int socket_at(const machine_t& m);
  const machine_t& machine_at(const int s);
  inline bool is_exist(int s){
    return sockets.end() != sockets.find(s);
  }
  inline bool is_exist(const machine_t& m){
    return socket_at(m) != 0;
  }
  inline int size()const{ return sockets.size(); }
  // insert
  bool insert_pair(int fd, const machine_t& m){
    sockets.insert(node(fd,m));
  }
  // erase
  bool erase_socket(int fd){
    return sockets.erase(fd);
  }
};
#endif
