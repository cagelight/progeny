#include "com.h"
#include "com_internal.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

static void catch_interrupt (int sig) {
	assert(sig == SIGINT);
	if (atomic_load(&com_close_semaphore))
		abort();
	else
		atomic_store(&com_close_semaphore, true);
}

int main() {
	signal(SIGINT, catch_interrupt);
	if (com_init()) com_run();
    com_term();
	return 0;
}
