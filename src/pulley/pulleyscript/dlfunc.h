#ifndef HAVE_DLFUNC
#ifdef __cplusplus
extern "C" {
#endif
/* Answer from StackOverflow.com, 1144107. */
/* copied from FreeBSD, lib/libc/gen/dlfunc.c */
struct __dlfunc_arg { int __dlfunc_dummy; } ;
typedef void (*dlfunc_t)(struct __dlfunc_arg);

dlfunc_t dlfunc(const void *handle, const void *symbol);
#ifdef __cplusplus
}
#endif
#endif

