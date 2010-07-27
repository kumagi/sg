
#define BOOST_TEST_MODULE
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
//#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>

#include <stdlib.h>
#include <time.h>

#include "sg_types.hpp"

//BOOST_AUTO_TEST_SUITE(vector_test)

BOOST_AUTO_TEST_CASE(constructor)
{
  membership_vector vec;
  BOOST_CHECK_EQUAL(vec.vector,0);
  membership_vector vec2(0xdeadbeef);
  BOOST_CHECK_EQUAL(vec2.vector,0xdeadbeef);
  membership_vector vec3(vec2);
  BOOST_CHECK_EQUAL(vec3.vector,0xdeadbeef);
}
BOOST_AUTO_TEST_CASE(max_match)
{
  for(int i=0;i<64;i++){
    membership_vector vec(1LLU<<i);
    BOOST_CHECK_EQUAL
      (vec.match(membership_vector(1LLU<<i)), membership_vector::vectormax);
  }
}
BOOST_AUTO_TEST_CASE(one_bit)
{
  membership_vector vec(2);
  BOOST_CHECK_EQUAL(vec.match(membership_vector(1)), 0);
  BOOST_CHECK_EQUAL(vec.match(membership_vector(0)), 1);
  BOOST_CHECK_EQUAL(vec.match(membership_vector(6)), 2);
  BOOST_CHECK_EQUAL(vec.match(membership_vector(10)),3);
}
BOOST_AUTO_TEST_CASE(shift_bit)
{
  for(int i = 0; i < 64; ++i){
    membership_vector vec(1LLU<<i);
    BOOST_CHECK_EQUAL(vec.vector,1LLU<<i);
    membership_vector other(3LLU<<i);
    BOOST_CHECK_EQUAL(vec.match(other), i+1);
  }
}

//BOOST_AUTO_TEST_SUITE_END()
