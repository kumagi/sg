#include <string>
#include <iostream>
#include <boost/program_options.hpp>

#include <msgpack.hpp>
#include "machine.hpp"
#include "sg_messages.hpp"
#include "tcp_wrap.h"

struct settings{
  int myip,target_ip;
  unsigned short myport,target_port;
  settings():myip(get_myip()),target_ip(aton("127.0.0.1")),myport(9000),target_port(10000){}
} s;

int main(int argc, char** argv){
  namespace po = boost::program_options;
  // parse options
  po::options_description opt("options");
  std::string master;
  opt.add_options()
    ("help,h", "view help")
    ("verbose,v", "verbose mode")
    ("port,p",po::value<unsigned short>
     (&s.myport)->default_value(11011), "my port number")
    ("address,a",po::value<std::string>
     (&master)->default_value("127.0.0.1"), "master's address");

  po::variables_map vm;
  store(parse_command_line(argc,argv,opt), vm);
  notify(vm);
  if(vm.count("help")){
    std::cerr << opt << std::endl;
    return 0;
  }

  int fd = create_tcpsocket();
  connect_ip_port(fd, aton("127.0.0.1"), s.target_port);
  
  msg::Search query (msg::SEARCH, "key", 8, machine(s.myip,s.myport));
  msgpack::sbuffer sb;
  msgpack::pack(sb, query);
  size_t sendsize = write(fd, sb.data(), sb.size());
  assert(sendsize == sb.size());
  
  {
    char buff[256];
    ssize_t readsize = read(fd, buff, 256);
    msgpack::unpacked up;
    msgpack::unpack(&up, buff, readsize);
    msgpack::object o(up);
    //    msg::NotFound ans = o.as();
  }

}
