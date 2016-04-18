#define thread_local _Thread_local
#define atomic _Atomic
#define ipause __asm volatile ("pause" ::: "memory")
#define erase( ptr ) ({ free(ptr); ptr = NULL; })
