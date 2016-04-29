#define NANO 1000000000
#define MICRO 1000000
#define MILLI 1000

#define thread_local _Thread_local
#define atomic _Atomic
#define ipause __asm volatile ("pause" ::: "memory")
#define erase( ptr ) ({ free(ptr); ptr = NULL; })
#define clamp(value, min, max) ({ value < min ? min : value > max ? max : value; })
