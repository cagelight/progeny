#include "com.h"
#include "com_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

atomic bool com_close_semaphore;

bool com_init (void) {
	
	atomic_init(&com_close_semaphore, false);
	
	if (!com_xcb_init()) {
		com_print_fatal("XCB initialization failed");
		return false;
	}
	
	if (!com_vk_init()) {
		com_print_fatal("vulkan initialization failed");
		return false;
	}
	
	return true;
}

void com_run(void) {
	while (!atomic_load(&com_close_semaphore)) {
		
		com_xcb_frame();
		
		if (atomic_load(&xcb_close_semaphore))
			atomic_store(&com_close_semaphore, true);
	}
}

void com_term (void) {
	com_vk_term();
	com_xcb_term();
}

void com_print(char const * str) {
	printf("%s", str);
	printf("\n");
}
