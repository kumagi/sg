#ifndef HDEBUGMODE
#define HDEBUGMODE


#define DEBUG_MODE
#ifdef DEBUG_MODE
#define DEBUG_OUT(...) fprintf(stderr,__VA_ARGS__)
#define DEBUG(x) x
#define REACH(x) fprintf(stderr,"reach line:%d %s",__LINE__, x)
#else
#define NDEBUG
#define DEBUG_OUT(...) 
#define DEBUG(...)
#define REACH(...)
#endif

#endif
