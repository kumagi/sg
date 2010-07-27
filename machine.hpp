#ifndef MACHINE_HPP__
#define MACHINE_HPP__
#include <msgpack.hpp>

class machine{
public:
  typedef int ip_t;
  typedef unsigned short port_t;

  ip_t ip_;
  port_t port_;
  machine():ip_(0),port_(0){};
  machine(const machine& o):ip_(o.ip_),port_(o.port_){};
  machine(int _ip, unsigned short _port):ip_(_ip),port_(_port){};

  // getter
  inline ip_t ip()const{ return ip_;}
  inline port_t port()const{ return port_;};

  // setter
  inline void set_ip(ip_t _ip){ ip_ = _ip;}
  inline void set_port(port_t _port){ port_ = _port;}
  inline void set(const machine& org){ ip_ = org.ip_; port_ = org.port_;}
  inline machine& operator=(const machine& rhs){
    ip_ = rhs.ip_;
    port_ = rhs.port_;
    return *this;
  }

  // operators
  inline bool operator==(const machine& rhs)const{
    return ip_ == rhs.ip_ && port_ == rhs.port_;
  }
  inline bool operator!=(const machine& rhs)const{
    return !operator==(rhs);
  }
  inline bool is_invalid()const;
  MSGPACK_DEFINE(ip_,port_);
};
typedef struct machine machine_t;
#endif
