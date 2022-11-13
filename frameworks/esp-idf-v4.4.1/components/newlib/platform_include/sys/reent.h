// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stddef.h>
#include <sys/types.h>

// #include_next<sys/reent.h>

/* How big the some arrays are.  */
#define _REENT_EMERGENCY_SIZE 25
#define _REENT_ASCTIME_SIZE 26
#define _REENT_SIGNAL_SIZE 24

#ifdef __cplusplus
extern "C" {
#endif

struct _reent
{
  int _errno;			/* local copy of errno */

//   /* FILE is a big struct and may change over time.  To try to achieve binary
//      compatibility with future versions, put stdin,stdout,stderr here.
//      These are pointers into member __sf defined below.  */
//   __FILE *_stdin, *_stdout, *_stderr;

  int  _inc;			/* used by tmpnam */
  char _emergency[_REENT_EMERGENCY_SIZE];

//   /* TODO */
//   int _unspecified_locale_info;	/* unused, reserved for locale stuff */
//   struct __locale_t *_locale;/* per-thread locale */

  int __sdidinit;		/* 1 means stdio has been init'd */

  void (*__cleanup) (struct _reent *);

  /* used by mprec routines */
  struct _Bigint *_result;
  int _result_k;
  struct _Bigint *_p5s;
  struct _Bigint **_freelist;

  /* used by some fp conversion routines */
  int _cvtlen;			/* should be size_t */
  char *_cvtbuf;

  union
    {
      struct
        {
          unsigned int _unused_rand;
          char * _strtok_last;
          char _asctime_buf[_REENT_ASCTIME_SIZE];
        //   struct __tm _localtime_buf;
          int _gamma_signgam;
          __extension__ unsigned long long _rand_next;
        //   struct _rand48 _r48;
        //   _mbstate_t _mblen_state;
        //   _mbstate_t _mbtowc_state;
        //   _mbstate_t _wctomb_state;
          char _l64a_buf[8];
          char _signal_buf[_REENT_SIGNAL_SIZE];
          int _getdate_err;
        //   _mbstate_t _mbrlen_state;
        //   _mbstate_t _mbrtowc_state;
        //   _mbstate_t _mbsrtowcs_state;
        //   _mbstate_t _wcrtomb_state;
        //   _mbstate_t _wcsrtombs_state;
	  int _h_errno;
        } _reent;
  /* Two next two fields were once used by malloc.  They are no longer
     used. They are used to preserve the space used before so as to
     allow addition of new reent fields and keep binary compatibility.   */
      struct
        {
#define _N_LISTS 30
          unsigned char * _nextf[_N_LISTS];
          unsigned int _nmalloc[_N_LISTS];
        } _unused;
    } _new;

# ifndef _REENT_GLOBAL_ATEXIT
  /* atexit stuff */
//   struct _atexit *_atexit;	/* points to head of LIFO stack */
//   struct _atexit _atexit0;	/* one guaranteed table, required by ANSI */
# endif

  /* signal info */
  void (**(_sig_func))(int);

  /* These are here last so that __FILE can grow without changing the offsets
     of the above members (on the off chance that future binary compatibility
     would be broken otherwise).  */
//   struct _glue __sglue;		/* root of glue chain */
# ifndef _REENT_GLOBAL_STDIO_STREAMS
//   __FILE __sf[3];  		/* first three file descriptors */
# endif
};

/*
 * All references to struct _reent are via this pointer.
 * Internally, newlib routines that need to reference it should use _REENT.
 */

#ifndef __ATTRIBUTE_IMPURE_PTR__
#define __ATTRIBUTE_IMPURE_PTR__
#endif

#if !defined(__DYNAMIC_REENT__) || defined(__SINGLE_THREAD__)
extern struct _reent *_impure_ptr __ATTRIBUTE_IMPURE_PTR__;
#endif
extern struct _reent * _global_impure_ptr __ATTRIBUTE_IMPURE_PTR__;

void _reclaim_reent (struct _reent *);

/* #define _REENT_ONLY define this to get only reentrant routines */

// #if defined(__DYNAMIC_REENT__) && !defined(__SINGLE_THREAD__)
#ifndef __getreent
  struct _reent * __getreent (void);
#endif
# define _REENT (__getreent())
// #else /* __SINGLE_THREAD__ || !__DYNAMIC_REENT__ */
// # define _REENT _impure_ptr
// #endif /* __SINGLE_THREAD__ || !__DYNAMIC_REENT__ */

#define _GLOBAL_REENT _global_impure_ptr

#ifdef _REENT_GLOBAL_ATEXIT
extern struct _atexit *_global_atexit; /* points to head of LIFO stack */
# define _GLOBAL_ATEXIT _global_atexit
#else
# define _GLOBAL_ATEXIT (_GLOBAL_REENT->_atexit)
#endif

/* This function is not part of the newlib API, it is defined in libc/stdio/local.h
 * There is no nice way to get __cleanup member populated while avoiding __sinit,
 * so extern declaration is used here.
 */
extern void _cleanup_r(struct _reent* r);

#ifdef __cplusplus
}
#endif
