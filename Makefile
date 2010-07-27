CXX=g++
OPTS=-O4 -fexceptions -g
LD=-lmpio -lmsgpack -pthread -lboost_program_options
WARNS= -W -Wall -Wextra -Wformat=2 -Wstrict-aliasing=4 -Wcast-qual -Wcast-align \
	-Wwrite-strings -Wfloat-equal -Wpointer-arith -Wswitch-enum
HEADS=hash64.h hash32.h random64.h address.hpp sockets.hpp merdy_operations.h debug_mode.h dynamo_objects.hpp mercury_objects.hpp 

target:sg

sg:sg.o tcp_wrap.o socket_collection.o
	$(CXX) sg.o tcp_wrap.o socket_collection.o -o sg $(LD) $(OPTS)

socket_collection.o:socket_collection.hpp socket_collection.cc
	$(CXX) -c socket_collection.cc -o socket_collection.o $(OPTS) $(WARNS)