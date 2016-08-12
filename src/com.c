#include "com.h"
#include "com_internal.h"
#include "com_vk.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <time.h>

//============================== VAS =============================
//	Thread safe function for quick string formatting.
//	Allows nesting to an extent.

#define VAS_BUFLEN 1024 //Should be a size large enough for "general use"
#define VAS_BUFNUM 8 //Corresponds to maximum nesting depth

thread_local char vbufs [VAS_BUFNUM][VAS_BUFLEN];
thread_local int vbufn = 0;

char * vas(char const * fmt, ...) {
	va_list va;
	va_start(va, fmt);
	char * vbuf = vbufs[vbufn++];
	vsnprintf(vbuf, VAS_BUFLEN, fmt, va);
	va_end(va);
	if (vbufn == VAS_BUFNUM) vbufn = 0;
	return vbuf;
}

//================================================================

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
		//com_vk_swap();
		
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
