#include "sg_types.hpp"
#include <stdio.h>

int hash_value(const neighbor& n){
  return boost::hash_value(n.key_) + hash_value(n.place_);
}

namespace{
  void  dump(uint64_t vec){
    const char* bits = reinterpret_cast<const char*>(&vec);
    for(int i=7;i>=0;--i){
      fprintf(stderr,"%02x",(unsigned char)(255&bits[i]));
    }
  }
}

#include <stdio.h>
int membership_vector::match(const membership_vector& o)const{
  const uint64_t matched = ~(vector ^ o.vector);
  uint64_t bit = 1;
  int cnt = 0;
  while((matched & bit) && cnt < 64){
    bit *= 2;
    cnt++;
  }
  return cnt;
}
void membership_vector::dump()const{
  const char* bits = reinterpret_cast<const char*>(&vector);
  for(int i=7;i>=0;--i){
    fprintf(stderr,"%02x",(unsigned char)255&bits[i]);
  }
}

