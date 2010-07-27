#ifndef SG_TYPES_HPP_
#define SG_TYPES_HPP_
#include <pthread.h>
#include <string>
#include <boost/noncopyable.hpp>


typedef std::string key;
typedef std::string value;

class range{
	key begin_,end_;
	bool border_begin_,border_end_; // true = contain, false = not
public:
	// construction
	range():begin_(),end_(),border_begin_(false),border_end_(false){}
	range(const key& _begin, const key& _end, bool _border_begin,bool _border_end)
		:begin_(_begin),end_(_end),border_begin_(_border_begin),border_end_(_border_end){}
	range(const range& org)
		:begin_(org.begin_),end_(org.end_),
		 border_begin_(org.border_begin_),border_end_(org.border_end_){}
	range& operator=(const range& rhs){
		begin_ = rhs.begin_;
		end_ = rhs.end_;
		border_begin_ = rhs.border_begin_;
		border_end_ = rhs.border_end_;
		return *this;
	}
	// checker
	bool contain(const key& t)const{
		if(begin_ == t && border_begin_) return true;
		if(end_ == t && border_end_) return true;
		if(begin_ < t && t < end_) return true;
		return false;
	}
	MSGPACK_DEFINE(begin_,end_,border_begin_,border_end_);
};

class neighbor:public boost::noncopyable{
  key key_;
  machine_t place_;
public:
  neighbor(const key& _key,const machine_t& _place)
    :key_(_key),place_(_place){}
  const key& get_key()const{
    return key_;
  }
  const machine_t& get_machine()const{
    return place_;
  }
};

struct membership_vector{
  uint64_t vector;
  membership_vector(uint64_t v):vector(v){}
  membership_vector():vector(){}
  membership_vector(const membership_vector& org):vector(org.vector){}
  MSGPACK_DEFINE(vector); // serialize and deserialize ok
};

class mutex : public boost::noncopyable{
  pthread_mutex_t mut_;
public:
  friend class scoped_lock;
  class scoped_lock : public boost::noncopyable{
    pthread_mutex_t& target;
    bool locked;
  public:
    scoped_lock(mutex& org)
      :target(org.mut_)
    {
      pthread_mutex_lock(&target);
      locked=true;
    }
    ~scoped_lock(){
      if(locked){
	pthread_mutex_unlock(&target);
      }
    }
    void unlock(){
      pthread_mutex_unlock(&target);
      locked=false;
    }
    void relock(){
      pthread_mutex_lock(&target);
      locked=true;
    }
  };
  mutex(){pthread_mutex_init(&mut_,0);}
  ~mutex(){pthread_mutex_destroy(&mut_);}
};
#endif
