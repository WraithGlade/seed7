/********************************************************************/
/*                                                                  */
/*  common.h      Basic type definitions and settings.              */
/*  Copyright (C) 1989 - 2005  Thomas Mertes                        */
/*                                                                  */
/*  This file is part of the Seed7 Runtime Library.                 */
/*                                                                  */
/*  The Seed7 Runtime Library is free software; you can             */
/*  redistribute it and/or modify it under the terms of the GNU     */
/*  Lesser General Public License as published by the Free Software */
/*  Foundation; either version 2.1 of the License, or (at your      */
/*  option) any later version.                                      */
/*                                                                  */
/*  The Seed7 Runtime Library is distributed in the hope that it    */
/*  will be useful, but WITHOUT ANY WARRANTY; without even the      */
/*  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR */
/*  PURPOSE.  See the GNU Lesser General Public License for more    */
/*  details.                                                        */
/*                                                                  */
/*  You should have received a copy of the GNU Lesser General       */
/*  Public License along with this program; if not, write to the    */
/*  Free Software Foundation, Inc., 59 Temple Place, Suite 330,     */
/*  Boston, MA 02111-1307 USA                                       */
/*                                                                  */
/*  Module: Seed7 Runtime Library                                   */
/*  File: seed7/src/common.h                                        */
/*  Changes: 1992, 1993, 1994, 2005  Thomas Mertes                  */
/*  Content: Basic type definitions and settings.                   */
/*                                                                  */
/********************************************************************/

typedef int booltype;

#ifdef FALSE
#undef FALSE
#endif
#define FALSE    ((booltype) 0)
#ifdef TRUE
#undef TRUE
#endif
#define TRUE     ((booltype) 1)

#define EXTERN          extern

#define WITH_FLOAT
#define WITH_REFERENCE
#define WITH_DRAW
#undef  WITH_STATISTIK
#define WITH_COMPILATION_INFO
#define WITH_TYPE_CHECK
#undef  WITH_ACTION_CHECK
#define WITH_PROTOCOL
#define USE_ALTERNATE_NEXT_CHARACTER
#define USE_INFILE_BUFFER
#define USE_CHUNK_ALLOCS
#undef  USE_ALTERNATE_CHUNK_ALLOCS
#define USE_VARIABLE_FORMATS
#undef  USE_MAXIMUM_MALLOC_CHECK
#define DO_HEAPSIZE_COMPUTATION
#undef  DO_HEAP_STATISTIK
#undef  DO_HEAP_LOG
#undef  DO_HEAP_CHECK
#define WIDE_CHAR_STRINGS
#undef  MMAP_ABLE_STRI


typedef float              floattype;
typedef long int           inttype;
typedef unsigned long int  chartype;
#ifdef WIDE_CHAR_STRINGS
typedef unsigned long int  strelemtype;
#else
typedef unsigned char      strelemtype;
#endif
typedef unsigned long int  uinttype;
typedef uinttype           bitsettype;
typedef unsigned short int bigdigittype;
typedef unsigned long int  doublebigdigittype;
typedef unsigned long int  memsizetype;
typedef signed char        smallpriortype;
typedef int                prior_type;

typedef unsigned char      uchartype;
typedef char *             cstritype;
typedef unsigned char *    ustritype;
typedef FILE *             filetype;

#define MAX_INTEGER 2147483647

#define MAX_DIV_10 ((inttype) (MAX_INTEGER / 10))
#define MAX_REM_10 ((inttype) (MAX_INTEGER % 10))

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000000
#endif

#define SOURCE_POSITION(POS_NR) __FILE__, __LINE__


#ifdef ANSI_C
typedef size_t SIZE_TYPE;
#else
typedef unsigned int SIZE_TYPE;
#endif

#ifdef C_PLUS_PLUS
#define INLINE inline
#else
#define INLINE
#endif


typedef int errinfotype;

/* errinfo values: */
#define OKAY_NO_ERROR 0
#define MEMORY_ERROR  1
#define NUMERIC_ERROR 2
#define RANGE_ERROR   3
#define IO_ERROR      4
#define CREATE_ERROR  5
#define DESTROY_ERROR 6
#define COPY_ERROR    7
#define IN_ERROR      8
#define FILE_ERROR    9


typedef struct setstruct     *settype;
typedef struct stristruct    *stritype;
typedef struct bstristruct   *bstritype;
typedef struct winstruct     *wintype;
typedef struct bigintstruct  *biginttype;

typedef struct setstruct {
    inttype min_position;
    inttype max_position;
    bitsettype bitset[1];
  } setrecord;

typedef struct stristruct {
    memsizetype size;
#ifdef MMAP_ABLE_STRI
    strelemtype *mem;
    strelemtype mem1[1];
#else
    strelemtype mem[1];
#endif
  } strirecord;

typedef struct bstristruct {
    memsizetype size;
#ifdef MMAP_ABLE_BSTRI
    uchartype *mem;
    uchartype mem1[1];
#else
    uchartype mem[1];
#endif
  } bstrirecord;

typedef struct winstruct {
    unsigned long usage_count;
    /* The rest of the structure is only accessable for the driver */
  } winrecord;

typedef struct bigintstruct {
    memsizetype size;
    /* bigdigittype sign; */
    bigdigittype bigdigits[1];
  } bigintrecord;
