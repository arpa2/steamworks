#ifndef HAVE_DLFUNC

#include <dlfcn.h>
#include "dlfunc.h"

/* Answer from StackOverflow.com, 1144107. */
/* copied from FreeBSD, lib/libc/gen/dlfunc.c */

dlfunc_t dlfunc(const void *handle, const void *symbol) {
	union {
		void *d;
		dlfunc_t f;
	} rv;
	rv.d = dlsym((void *)handle, (void *)symbol); /* Conversion warning */
	return rv.f;
}
#endif

