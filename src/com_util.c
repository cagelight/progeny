#include "com.h"

#include <stdarg.h>
#include <stdio.h>

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
