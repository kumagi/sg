#include <msgpack.hpp>
#include <vector>
#include "sg_types.hpp"

namespace msg{

enum operation{
  INFORM,
  SET,
  SET_OK,
  SEARCH,
  FOUND,
  NOTFOUND,
  RANGE,
  RANGE_FOUND,
  RANGE_NOTFOUND,
  LINK,
  TREAT,
  INTRODUCE,
};

typedef msgpack::type::tuple<operation> Operation;
typedef msgpack::type::tuple<operation,machine_t> Inform;
typedef msgpack::type::tuple<operation,key,value> Set;
typedef msgpack::type::tuple<operation,key> SetOk;
typedef msgpack::type::tuple<operation,key,int,machine_t> Search;//operation, key, targetLevel, origin
typedef msgpack::type::tuple<operation,key,value> Found;
typedef msgpack::type::tuple<operation,key> NotFound;
typedef msgpack::type::tuple<operation,range,machine_t> Range;
typedef msgpack::type::tuple<operation,range,std::vector<std::pair<key,value> >,std::vector<machine_t> > RangeFound;
typedef msgpack::type::tuple<operation,range,machine_t> RangeNotFound;
typedef msgpack::type::tuple<operation,key,int,key,machine_t> Link; // operation,targetKey,targetLevel, origin
typedef msgpack::type::tuple<operation,key,key,machine_t,membership_vector> Treat; // operation,targetKey,originKey, origin, originVector
typedef msgpack::type::tuple<operation,key,key,machine_t,membership_vector> Introduce;// operation,targetKey,originKey, origin, originVector
}

#define MSGPACK_ENUM_DEFINE(X) namespace msgpack{	\
	inline X& operator>> (object o, X& v){ \
	v = (X)type::detail::convert_integer<int>(o); return v;\
	}\
	template <typename Stream>\
	inline packer<Stream>& operator<< (packer<Stream>& o, const X& v)\
	{ o.pack_int(v); return o; }\
	}

MSGPACK_ENUM_DEFINE(msg::operation);
/*
namespace msgpack{
inline msg::operation& operator>> (object o, enum msg::operation& v){
  v = (enum msg::operation)type::detail::convert_integer<int>(o); return v;
}
}// namespace msgpack

*/
