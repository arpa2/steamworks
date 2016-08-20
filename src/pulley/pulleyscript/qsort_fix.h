/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * This is a stub to swap around parameter order for qsort_r.
 *
 * QSORT_R() is a macro that takes FreeBSD parameter-order
 * and calls qsort_r() with the platform parameter-order.
 *
 * QSORT_CMP_FUN_DECL is a macro that wraps a function f as
 * _qsort_f which takes platform parameter-order; the function
 * f must take FreeBSD parameter-order.
 *
 * qsort_cmp_fun_t is the function type for the cmp-function
 * parameter to qsort_r.
 */
#ifdef QSORT_IS_GLIBC
#define QSORT_R(base, nmemb, size, thunk, func) qsort_r(base, nmemb, size, func, thunk)
#define QSORT_CMP_FUN_DECL(x) static int _qsort_##x(const void *left, const void *right, void *thunk) { return x(thunk, left, right); }
typedef int (*qsort_cmp_fun_t) (const void *left_leg, const void *right_leg, void *context);
#else
#define QSORT_R qsort_r
#define QSORT_CMP_FUN_DECL(x) static int _qsort_##x(void *thunk, const void *left, const void *right) { return x(thunk, left, right); }
typedef int (*qsort_cmp_fun_t) (void *context, const void *left_leg, const void *right_leg);
#endif

