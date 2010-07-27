#include "socket_collection.hpp"
int socket_collection::socket_at(const machine_t& m){
	for(sock_iterator it = sockets.begin();
			it != sockets.end();
			++it)
		{
			if(it->second == m){
				return it->first;
			}
		}
	return 0;
}
  
const machine_t& socket_collection::machine_at(const int s){
	sock_iterator it = sockets.find(s);
	if(it != sockets.end()){
		return it->second;
	}
	else{
		return dummy;
	}
}
