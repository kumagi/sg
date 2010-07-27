#include <stdio.h>
#include <pthread.h>
#include <map>
#include <set>

#include <boost/program_options.hpp>
#include <boost/functional/hash.hpp>
#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>

#include <msgpack.hpp>
#include <mp/wavy.h>

#include "tcp_wrap.h"

#include "debug_mode.h"

#include "machine.hpp"
#include "socket_collection.hpp"

#include "sg_messages.hpp"

template<typename Derived>
class singleton : private boost::noncopyable {
public:
  static Derived& instance() { static Derived the_inst; return the_inst; }
protected:
  singleton(){}
  ~singleton(){}
};

struct settings: public singleton<settings>{
  enum status_{
    ready,
    unjoined,
  };
  int verbose;
  std::string interface;
  unsigned short myport;
  int myip;
  int maxlevel;
  int status;
  settings():verbose(0),interface("eth0"),myport(11011),myip(get_myip()),maxlevel(16){}
};

class sg_node{
  value value_;
  std::vector<boost::shared_ptr<const neighbor> > next_keys[2]; // left=0, right=1
public:
  enum direction{
    left = 0,
    right = 1,
  };
  explicit sg_node(const value& _value, const int level = settings::instance().maxlevel)
    :value_(_value){
    next_keys[left].reserve(level);
    next_keys[right].reserve(level);
  }
  sg_node(const sg_node& o)
    :value_(o.value_){
    next_keys[left].reserve(o.next_keys[left].size());
    next_keys[right].reserve(o.next_keys[right].size());
  }
  
  const std::string& get_value()const{ return value_;}
  void set_value(const value& v){value_ = v;}
  const std::vector<boost::shared_ptr<const neighbor> >* neighbors()const{
    return next_keys;
  }
private:
  sg_node();
};
typedef std::pair<key,sg_node> kvp;

int string_distance(const std::string& lhs, const std::string& rhs){
  int len = lhs.length();
  for(int i=0;i<len;++i){
    if(lhs[i] != rhs[i]) return std::abs(lhs[i] - rhs[i]);
  }
  return 0;
}
struct shared_object: public singleton<shared_object>{
  // all of its member should be multi-thread free
  socket_collection sockets;
  std::map<key,sg_node> storage;
  mp::wavy::loop* lo;
};

static mutex connect_lock;
int prepare(const machine_t& target){
  socket_collection& sc = shared_object::instance().sockets;
  if(sc.is_exist(target)){
    return sc.socket_at(target);
  }else{
    // optimistic lock
    mutex::scoped_lock connect_lock_r(connect_lock);
    if(sc.is_exist(target)){
      int newfd = create_tcpsocket();
      connect_ip_port(newfd, target.ip(), target.port());
      sc.insert_pair(newfd,target);
      return newfd;
    }else{
      return sc.socket_at(target);
    }
  }
}

class tuple_buff{
  std::size_t size_;
  char* data_;
  std::size_t alloc;
public:
  tuple_buff(int _size = 4096)
    :size_(_size),data_((char*)malloc(_size)),alloc(_size){}
  void write(const char* buf, unsigned int len){
    if(alloc - size_ < len) {
      expand_buffer(len);
    }
    ::memcpy(data_ + size_, buf, len);
    size_ += len;
  }
  void dump(FILE* fd)const{
    for(int i=0;i<size_;++i){
      fprintf(fd,"%02X",data_[i]&0xff);
    }
  }
  const void* data()const{ return data_; }
  int size()const{ return size_;}
  int length()const{ return size();}
  ~tuple_buff(){
    if(data_){ free(data_);}
  }
private:
  void expand_buffer(size_t len){
    size_t nsize = (alloc) ? alloc * 2 : MSGPACK_SBUFFER_INIT_SIZE;
    while(nsize < size_ + len) { nsize *= 2; }
    void* tmp = realloc(data_, nsize);
    if(!tmp) { throw std::bad_alloc(); }
    
    data_ = (char*)tmp;
    alloc = nsize;
  }
};

class skipgraph : public mp::wavy::handler {
  mp::wavy::loop* lo_;
  msgpack::unpacker m_pac;
public:
  skipgraph(int _fd,mp::wavy::loop* _lo):
    mp::wavy::handler(_fd),lo_(_lo){ }
  void event_handle(int fd, const msgpack::object& obj, mp::shared_ptr<msgpack::zone>* z){
    msg::Operation message(obj);
    msg::operation operation = message.get<0>();
    switch(operation){
    case msg::INFORM: sg_inform(fd,obj,z); break;
    case msg::SET:   sg_set(fd,obj,z); break;
    case msg::SEARCH: sg_search(fd,obj,z); break;
    case msg::FOUND:  sg_found(fd,obj,z); break;
    case msg::NOTFOUND:sg_notfound(fd,obj,z); break;
    case msg::RANGE:  sg_range(fd,obj,z); break;
    case msg::RANGE_FOUND: sg_range_found(fd,obj,z); break;
    case msg::RANGE_NOTFOUND: sg_range_notfound(fd,obj,z); break;
    case msg::LINK:  sg_link(fd,obj,z); break;
    case msg::TREAT:  sg_treat(fd,obj,z); break;
    case msg::INTRODUCE:  sg_introduce(fd,obj,z); break;
    }
  }
  void on_read(mp::wavy::event& e)
  {
    try{
      while(true) {
	if(m_pac.execute()) {
	  msgpack::object msg = m_pac.data();
	  mp::shared_ptr<msgpack::zone> z( m_pac.release_zone() );
	  m_pac.reset();

	  e.more(); //e.next();
     
	  //DEBUG(std::cout << "object received: " << msg << std::endl);
	  event_handle(fd(), msg, &z);
	  return;
	}
    
	m_pac.reserve_buffer(8*1024);

	int read_len = ::read(fd(), m_pac.buffer(), m_pac.buffer_capacity());
	if(read_len <= 0) {
	  if(read_len == 0) {
	    perror("closed"); throw mp::system_error(errno, "connection closed");
	  }
	  if(errno == EAGAIN || errno == EINTR) { return; }
	  else { perror("read"); throw mp::system_error(errno, "read error"); }
	}
	m_pac.buffer_consumed(read_len);
      }
    }catch(msgpack::type_error& e) {
      fprintf(stderr,"msgpack: type error");
      throw;
    }catch(...){
      fprintf(stderr,"fd:%d ",fd());;
      perror("exception ");
      e.remove();
    }
  }
private:
  void sg_inform(int fd, const msgpack::object& obj,mp::shared_ptr<msgpack::zone>* z){
    REACH("inform:");
    const msg::Inform inform(obj);
    const machine_t& machine(inform.get<1>());
  }
  void sg_set(int fd, const msgpack::object& obj,mp::shared_ptr<msgpack::zone>* z){
    REACH("set:");
    const msg::Set set(obj); // deserialize
    const key& target_key(set.get<1>());
    const value& target_value(set.get<2>());
    std::map<key,sg_node>& storage = shared_object::instance().storage;
    int& maxlevel = settings::instance().maxlevel;
    storage.insert(kvp(target_key,sg_node(target_value)));
    // FIXME: answer [set ok]
    //lo_->write(fd,);
  }
  void sg_search(int fd, const msgpack::object& obj,mp::shared_ptr<msgpack::zone>* z){
    REACH("search:");
    const msg::Search search(obj); // deserialize
    const key& target_key(search.get<1>());
    const int& target_level(search.get<2>());
    const machine_t& origin(search.get<3>());
    std::map<key,sg_node>& storage = shared_object::instance().storage;
    int& maxlevel = settings::instance().maxlevel;
  
    std::map<key,sg_node>::iterator node = storage.find(target_key);
    if(node == storage.end()){ // this node does not have that key value pair
      node = storage.lower_bound(target_key);
      if(node == storage.end()){
	node = storage.upper_bound(target_key);
	assert(node != storage.end() && "I dont have any key");
      }else{
	const key& found_key = node->first;
	int org_distance = string_distance(found_key,target_key);
	bool target_is_right = true;
	++node;
	if((node != storage.end()) && 
	   org_distance > string_distance(node->first,target_key)){
	  --node;
	}else{
	  target_is_right = false;
	}
	// search target
	sg_node::direction left_or_right = target_is_right 
	  ? sg_node::right 
	  : sg_node::left;
	int i; // decleartion is out of loop for after checking
	for(i = node->second.neighbors()[left_or_right].size()-1;
	    i>0; --i){
	  if(node->second.neighbors()[left_or_right][i].get() != NULL &&
	     (left_or_right == sg_node::left &&   
	      !(node->second.neighbors()[sg_node::left][i]->get_key() < target_key))
	     ||
	     (left_or_right == sg_node::right &&
	      !(target_key < node->second.neighbors()[sg_node::left][i]->get_key()))
	  ){
	    const neighbor& target = *node->second.neighbors()[left_or_right][i].get();
	    const machine_t& address = target.get_machine();
	    mp::shared_ptr<tuple_buff> sendbuff(new tuple_buff());
	    const msg::Search query(msg::SEARCH,target_key,i, origin);
	    msgpack::pack(*sendbuff,query);
	    
	    // address resolve
	    int target_fd = prepare(address);
	    // asynchronous send
	    lo_->write(target_fd, sendbuff->data(), sendbuff->size(), sendbuff); 
	    break;
	  }
	}
	if(i<0){ // that key must not exist
	  
	}
      }
    }else{ // I have that key value pair!
      
    }
  }
  void sg_found(int fd, const msgpack::object& obj,mp::shared_ptr<msgpack::zone>* z){
    REACH("found:");
    const msg::Found found(obj); // deserialize
    const key& target_key(found.get<1>());
    const value& target_value(found.get<2>());
  }
  void sg_notfound(int fd, const msgpack::object& obj,mp::shared_ptr<msgpack::zone>* z){
    REACH("notfound:");
    const msg::Found notfound(obj); // deserialize
    const key& target_key(notfound.get<1>());
  }
  void sg_range(int fd, const msgpack::object& obj,mp::shared_ptr<msgpack::zone>* z){
    REACH("range:");
    const msg::Range range_search(obj); // deserialize
    const range& target_range(range_search.get<1>());
    const machine_t& origin(range_search.get<2>());
  }
  void sg_range_found(int fd, const msgpack::object& obj,mp::shared_ptr<msgpack::zone>* z){
    REACH("range_found:");
    const msg::RangeFound range_found(obj); // deserialize
    const range& target_range(range_found.get<1>());
    const std::vector<std::pair<key,value> >& kvps(range_found.get<2>());
    const std::vector<machine_t>& passed(range_found.get<3>());
  }
  void sg_range_notfound(int fd, const msgpack::object& obj,mp::shared_ptr<msgpack::zone>* z){
    REACH("range_notfound:");
    const msg::RangeNotFound range_notfound(obj); // deserialize
    const range& target_range(range_notfound.get<1>());
    const machine_t& machine_t(range_notfound.get<2>());
  }
  void sg_link(int fd, const msgpack::object& obj,mp::shared_ptr<msgpack::zone>* z){
    REACH("link:");
    const msg::Link link(obj);
    const key& target_key(link.get<1>());
    const int& target_level(link.get<2>());
    const key& origin_key(link.get<3>());
    const machine_t& origin(link.get<4>());
  }
  void sg_treat(int fd, const msgpack::object& obj,mp::shared_ptr<msgpack::zone>* z){
    REACH("treat:");
    const msg::Treat treat(obj);
    const key& target_key(treat.get<1>());
    const key& origin_key(treat.get<2>());
    const machine_t& origin(treat.get<3>());
    const membership_vector& vector(treat.get<4>());
  }
  void sg_introduce(int fd, const msgpack::object& obj,mp::shared_ptr<msgpack::zone>* z){
    REACH("introduce:");
    const msg::Treat introduce(obj);
    const key& target_key(introduce.get<1>());
    const key& origin_key(introduce.get<2>());
    const machine_t& origin(introduce.get<3>());
    const membership_vector& vector(introduce.get<4>());
  }
};

void on_accepted(int fd, int err, mp::wavy::loop* lo)
{
  fprintf(stderr,"accept %d %d\n",fd,err);
  if(fd < 0) {
    perror("accept error");
    exit(1);
  }

  try {
    int on = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
    char address_pack[256];
    int total_read = 0;
    while(1){ // receiving naming pack
      int read_len = read(fd,&address_pack[total_read],256);
    }
    mp::shared_ptr<skipgraph> p = lo->add_handler<skipgraph>(fd,lo);
  } catch (...) {
    fprintf(stderr,"listening socket error");
    ::close(fd);
    return;
  }
}

namespace po = boost::program_options;
int main(int argc, char** argv)
{
  srand(time(NULL));
  settings& s = settings::instance();
  shared_object& so = shared_object::instance();
  // parse options
 
  po::options_description opt("options");
  std::string master;
  opt.add_options()
    ("help,h", "view help")
    ("verbose,v", "verbose mode")
    ("interface,i",po::value<std::string>
     (&s.interface)->default_value("eth0"), "my interface")
    ("port,p",po::value<unsigned short>
     (&s.myport)->default_value(11011), "my port number")
    ("address,a",po::value<std::string>
     (&master)->default_value("127.0.0.1"), "master's address");
 
  s.myip = get_myip_interface2(s.interface.c_str());
 
  po::variables_map vm;
  store(parse_command_line(argc,argv,opt), vm);
  notify(vm);
  if(vm.count("help")){
    std::cerr << opt << std::endl;
    return 0;
  }
  
  // set options
  if(vm.count("verbose")){
    s.verbose++;
  }

  mp::wavy::loop lo;
 
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_port = htons(11211);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  lo.listen(PF_INET, SOCK_STREAM, 0,
	    (struct sockaddr*)&addr, sizeof(addr),
	    mp::bind(&on_accepted, mp::placeholders::_1, mp::placeholders::_2, &lo)); 
  lo.run(4);
}
