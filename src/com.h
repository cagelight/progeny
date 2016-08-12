#ifndef PROGENY_COM_H
#define PROGENY_COM_H

#ifdef _ASSERT_H
#error 
#endif
#ifndef PROGENY_DEBUG
#define NDEBUG
#endif
#include <assert.h>

#define thread_local _Thread_local
#define atomic _Atomic
#define static_assert _Static_assert

#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)
#define unu __attribute__ ((unused))

typedef unsigned int uint;

#include <jemalloc/jemalloc.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>

#define NANO 1000000000
#define MICRO 1000000
#define MILLI 1000

#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define CLAMP(value, min, max) ({ __typeof__ (value) _v = (value); __typeof__ (value) _min = (min); __typeof__ (value) _max = (max); _v > _max ? _max : _v < _min ? _min : _v; })

#define ipause __asm volatile ("pause" ::: "memory")
#define erase( ptr ) ({ free(ptr); ptr = NULL; })

char * vas(char const * fmt, ...);

bool com_init (void);
void com_run(void);
void com_term (void);

void com_print(char const *);
#define com_printf(str, ...) com_print(vas(str, ##__VA_ARGS__))

#define com_print_level(str, lev) com_print(vas("%s (%s, line %u): %s", #lev, __func__, __LINE__, str))
#define com_printf_level(str, lev, ...) com_print(vas("%s (%s, line %u): %s", #lev, __func__, __LINE__, vas(str, ##__VA_ARGS__)))

#define com_print_info(str) com_print_level(str, INFO)
#define com_print_warning(str) com_print_level(str, WARNING)
#define com_print_error(str) com_print_level(str, ERROR)
#define com_print_fatal(str) com_print_level(str, FATAL)

#define com_printf_info(str, ...) com_printf_level(str, INFO, ##__VA_ARGS__)
#define com_printf_warning(str, ...) com_printf_level(str, WARNING, ##__VA_ARGS__)
#define com_printf_error(str, ...) com_printf_level(str, ERROR, ##__VA_ARGS__)
#define com_printf_fatal(str, ...) com_printf_level(str, FATAL, ##__VA_ARGS__)

#ifdef PROGENY_DEBUG
#define com_print_debug(str) com_print_level(str, DEBUG)
#define com_printf_debug(str, ...) com_printf_level(str, DEBUG, ##__VA_ARGS__)
#else
#define com_print_debug(str)
#define com_printf_debug(str, ...)
#endif

#endif//PROGENY_COM_H
