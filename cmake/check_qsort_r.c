/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * Test program to discover the order of parameters to qsort_r().
 * On FreeBSD, the order is:
 *
 *      void qsort_r(void *base, size_t nmemb, size_t size, void *thunk,
 *                   int (*compar)(void *, const void *, const void *));
 * 
 * i.e. thunk, then compar-func. On Linux (glibc), the order is:
 * 
 *      void qsort_r(void *base, size_t nmemb, size_t size,
 *                  int (*compar)(const void *, const void *, void *),
 *                  void *arg);
 * 
 * i.e. compar-func, then thunk (arg).
 * 
 * This program returns 0 if it's the Linux order, or 1 if it's the 
 * FreeBSD parameter-order.
 */
#include <stdlib.h>
#include <stdio.h>

int compar_linux(const void *p1, const void *p2, void *arg)
{
  return -(*((int *)p1) - 1);
}

int compar_freebsd(void *thunk, const void *p1, const void *p2)
{
  return *((int *)p1) - 1 ;
}

int main(int argc, char **argv)
{
  int arr[] = { 0, 1, 2 };
  
  /* On FreeBSD, the function is compar_freebsd, and compar_linux is passed in
   * as thunk; on Linux / glibc, the function is compar_linux and compar_freebsd
   * is passed in as arg.
   */
  qsort_r(arr, 3, sizeof(int), compar_linux, compar_freebsd);
  
  printf ("%d%d%d\n", arr[0], arr[1], arr[2]);
  return arr[0] == 2 ? 0 /* linux */ : 1 /* freebsd */;
}
