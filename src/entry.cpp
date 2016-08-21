#include "com.hpp"
#include "control.hpp"

#include <csignal>

static void catch_interrupt (int sig) {
	assert(sig == SIGINT);
	if (!control::run_sem)
		std::terminate();
	else
		control::run_sem.store(false);
}

int main() {
	signal(SIGINT, catch_interrupt);
	
	try {
		com::init();
	} catch (...) {
		srcprintf_debug("unhandled exception during initialization");
		com::term();
		return -1;
	}
	
	try {
		while (control::run_sem) {
			com::frame();
		}
	} catch (...) {
		srcprintf_debug("unhandled runtime exception");
		com::term();
		return -1;
	}
	
	com::term();
	return 0;
}
