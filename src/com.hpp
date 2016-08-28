#pragma once

#ifdef _ASSERT_H
#error 
#endif
#ifndef PROGENY_DEBUG
#define NDEBUG
#endif
#include <assert.h>

#include <atomic>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <memory>

namespace com {
	
	void init();
	void term() noexcept;
	void frame();
	
	namespace test {
		void init();
		void term() noexcept;
		void frame();
	}
	
	std::string strf(char const * fmt, ...) noexcept;
	
	void print(char const *) noexcept;
	inline void print(std::string str) noexcept { print(str.c_str()); }
	template <typename T> void print(T t) noexcept { print(std::to_string(t)); }
	template <typename ... T> void printf(char const * fmt, T ... t) noexcept { print(strf(fmt, t ...)); }
	
	struct exception {
		std::string msg;
		
		exception() = delete;
		exception(char const * str) : msg(str) {}
		exception(std::string & str) : msg(str) {}
		exception(std::string && str) : msg(str) {}
	};
	
	inline void erase(void * & ptr) {
		if (ptr) {
			free(ptr);
			ptr = nullptr;
		}
	}
}

#define srcprintf(lev, fmt, ...) com::print(com::strf("%s (%s, line %u): %s", #lev, __PRETTY_FUNCTION__, __LINE__, com::strf(fmt, ##__VA_ARGS__).c_str()).c_str())
#define srcprintf_error(fmt, ...) com::print(com::strf("ERROR (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, com::strf(fmt, ##__VA_ARGS__).c_str()).c_str())
#define srcthrow(fmt, ...) throw com::exception(com::strf("ERROR (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, com::strf(fmt, ##__VA_ARGS__).c_str()))
#ifdef PROGENY_DEBUG
#define srcprintf_debug(fmt, ...) com::print(com::strf("DEBUG (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, com::strf(fmt, ##__VA_ARGS__).c_str()).c_str())
#else
#define srcprintf_debug(fmt, ...)
#endif

#define throw_fatal throw -1
