/********************************************************************/
/*                                                                  */
/*  hi   Interpreter for Seed7 programs.                            */
/*  Copyright (C) 1990 - 2007  Thomas Mertes                        */
/*                                                                  */
/*  This program is free software; you can redistribute it and/or   */
/*  modify it under the terms of the GNU General Public License as  */
/*  published by the Free Software Foundation; either version 2 of  */
/*  the License, or (at your option) any later version.             */
/*                                                                  */
/*  This program is distributed in the hope that it will be useful, */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of  */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   */
/*  GNU General Public License for more details.                    */
/*                                                                  */
/*  You should have received a copy of the GNU General Public       */
/*  License along with this program; if not, write to the Free      */
/*  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,  */
/*  MA 02111-1307 USA                                               */
/*                                                                  */
/*  Module: Library                                                 */
/*  File: seed7/src/strlib.c                                        */
/*  Changes: 1991, 1992, 1993, 1994  Thomas Mertes                  */
/*  Content: All primitive actions for the string type.             */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"

#include "common.h"
#include "data.h"
#include "heaputl.h"
#include "flistutl.h"
#include "chclsutl.h"
#include "syvarutl.h"
#include "striutl.h"
#include "runerr.h"
#include "memory.h"
#include "str_rtl.h"

#undef EXTERN
#define EXTERN
#include "strlib.h"


static char *cstri_escape_sequence[] = {
    "\\000", "\\001", "\\002", "\\003", "\\004",
    "\\005", "\\006", "\\007", "\\b",   "\\t",
    "\\n",   "\\013", "\\f",   "\\r",   "\\016",
    "\\017", "\\020", "\\021", "\\022", "\\023",
    "\\024", "\\025", "\\026", "\\027", "\\030",
    "\\031", "\\032", "\\033", "\\034", "\\035",
    "\\036", "\\037"};



#ifdef WIDE_CHAR_STRINGS
#ifdef ANSI_C

static INLINE int strelem_memcmp (strelemtype *mem1, strelemtype *mem2,
    SIZE_TYPE number)
#else

static INLINE int strelem_memcmp (mem1, mem2, number)
strelemtype *mem1;
strelemtype *mem2;
SIZE_TYPE number;
#endif

  { /* strelem_memcmp */
    for (; number > 0; mem1++, mem2++, number--) {
      if (*mem1 != *mem2) {
        return(*mem1 - *mem2);
      } /* if */
    } /* for */
    return(0);
  } /* strelem_memcmp */



#ifdef ANSI_C

static INLINE strelemtype *search_strelem (strelemtype *mem, strelemtype ch,
    SIZE_TYPE number)
#else

static INLINE strelemtype *search_strelem (mem, ch, number)
strelemtype *mem;
strelemtype ch;
SIZE_TYPE number;
#endif

  { /* search_strelem */
    for (; number > 0; mem++, number--) {
      if (*mem == ch) {
        return(mem);
      } /* if */
    } /* for */
    return(NULL);
  } /* search_strelem */



#else

#define strelem_memcmp(mem1,mem2,len) memcmp(mem1, mem2, (len) * sizeof(strelemtype))
#define search_strelem memchr

#endif



#ifdef ANSI_C

static arraytype add_stri_to_array (strelemtype *stri_elems, memsizetype length,
    arraytype work_array, memsizetype *used_max_position)
#else

static arraytype add_stri_to_array (stri_elems, length,
    work_array, used_max_position)
strelemtype *stri_elems;
memsizetype length;
arraytype work_array;
memsizetype *used_max_position;
#endif

  {
    stritype new_stri;
    memsizetype position;

  /* add_stri_to_array */
    if (ALLOC_STRI(new_stri, length)) {
      COUNT_STRI(length);
      new_stri->size = length;
      memcpy(new_stri->mem, stri_elems,
          (SIZE_TYPE) length * sizeof(strelemtype));
      if (*used_max_position >= work_array->max_position) {
        if (!RESIZE_ARRAY(work_array, work_array->max_position,
            work_array->max_position + 256)) {
          FREE_STRI(new_stri, new_stri->size);
          new_stri = NULL;
        } else {
          COUNT3_ARRAY(work_array->max_position, work_array->max_position + 256);
          work_array->max_position += 256;
        } /* if */
      } /* if */
    } /* if */
    if (new_stri != NULL) {
      work_array->arr[*used_max_position].type_of = take_type(SYS_STRI_TYPE);
      work_array->arr[*used_max_position].descriptor.entity = NULL;
      work_array->arr[*used_max_position].value.strivalue = new_stri;
      INIT_CATEGORY_OF_VAR(&work_array->arr[*used_max_position], STRIOBJECT);
      (*used_max_position)++;
    } else {
      for (position = 0; position < *used_max_position; position++) {
        FREE_STRI(work_array->arr[position].value.strivalue,
            work_array->arr[position].value.strivalue->size);
      } /* for */
      FREE_ARRAY(work_array, work_array->max_position);
      work_array = NULL;
    } /* if */
    return(work_array);
  } /* add_stri_to_array */



#ifdef ANSI_C

static arraytype strChSplit (stritype main_stri, chartype delimiter)
#else

static arraytype strChSplit (main_stri, delimiter)
stritype main_stri;
chartype delimiter;
#endif

  {
    memsizetype used_max_position;
    strelemtype *search_start;
    strelemtype *search_end;
    strelemtype *found_pos;
    memsizetype pos;
    arraytype result_array;

  /* strChSplit */
    if (ALLOC_ARRAY(result_array, 256)) {
      result_array->min_position = 1;
      result_array->max_position = 256;
      used_max_position = 0;
      search_start = main_stri->mem;
      search_end = &main_stri->mem[main_stri->size];
      while ((found_pos = (strelemtype *) search_strelem(search_start,
          delimiter, (SIZE_TYPE) (search_end - search_start))) != NULL &&
          result_array != NULL) {
        result_array = add_stri_to_array(search_start,
            (memsizetype) (found_pos - search_start), result_array,
            &used_max_position);
        search_start = found_pos + 1;
      } /* while */
      if (result_array != NULL) {
        result_array = add_stri_to_array(search_start,
            (memsizetype) (search_end - search_start), result_array,
            &used_max_position);
        if (result_array != NULL) {
          if (!RESIZE_ARRAY(result_array, result_array->max_position,
              used_max_position)) {
            for (pos = 0; pos < used_max_position; pos++) {
              FREE_STRI(result_array->arr[pos].value.strivalue,
                  result_array->arr[pos].value.strivalue->size);
            } /* for */
            FREE_ARRAY(result_array, result_array->max_position);
            result_array = NULL;
          } else {
            COUNT3_ARRAY(result_array->max_position, used_max_position);
            result_array->max_position = used_max_position;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
    if (result_array == NULL) {
      raise_error(MEMORY_ERROR);
    } /* if */
    return(result_array);
  } /* strChSplit */



#ifdef ANSI_C

static arraytype strSplit (stritype main_stri, stritype delimiter)
#else

static arraytype strSplit (main_stri, delimiter)
stritype main_stri;
stritype delimiter;
#endif

  {
    memsizetype delimiter_size;
    strelemtype *delimiter_mem;
    strelemtype ch_1;
    memsizetype used_max_position;
    strelemtype *search_start;
    strelemtype *segment_start;
    strelemtype *search_end;
    strelemtype *found_pos;
    memsizetype pos;
    arraytype result_array;

  /* strSplit */
    if (ALLOC_ARRAY(result_array, 256)) {
      result_array->min_position = 1;
      result_array->max_position = 256;
      used_max_position = 0;
      delimiter_size = delimiter->size;
      delimiter_mem = delimiter->mem;
      ch_1 = delimiter_mem[0];
      search_start = main_stri->mem;
      segment_start = search_start;
      if (delimiter_size != 0 && main_stri->size >= delimiter_size) {
        search_end = &main_stri->mem[main_stri->size - delimiter_size + 1];
        while ((found_pos = (strelemtype *) search_strelem(search_start,
            ch_1, (SIZE_TYPE) (search_end - search_start))) != NULL &&
            result_array != NULL) {
          if (memcmp(found_pos, delimiter_mem,
              (SIZE_TYPE) delimiter_size * sizeof(strelemtype)) == 0) {
            result_array = add_stri_to_array(segment_start,
                (memsizetype) (found_pos - segment_start), result_array,
                &used_max_position);
            search_start = found_pos + delimiter_size;
            segment_start = search_start;
          } else {
            search_start = found_pos + 1;
          } /* if */
        } /* while */
      } /* if */
      if (result_array != NULL) {
        result_array = add_stri_to_array(segment_start,
            (memsizetype) (&main_stri->mem[main_stri->size] - segment_start), result_array,
            &used_max_position);
        if (result_array != NULL) {
          if (!RESIZE_ARRAY(result_array, result_array->max_position,
              used_max_position)) {
            for (pos = 0; pos < used_max_position; pos++) {
              FREE_STRI(result_array->arr[pos].value.strivalue,
                  result_array->arr[pos].value.strivalue->size);
            } /* for */
            FREE_ARRAY(result_array, result_array->max_position);
            result_array = NULL;
          } else {
            COUNT3_ARRAY(result_array->max_position, used_max_position);
            result_array->max_position = used_max_position;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
    if (result_array == NULL) {
      raise_error(MEMORY_ERROR);
    } /* if */
    return(result_array);
  } /* strSplit */



#ifdef OUT_OF_ORDER
#ifdef ANSI_C

arraytype str1Split (stritype main_stri, stritype delimiter)
#else

arraytype str1Split (main_stri, delimiter)
stritype main_stri;
stritype delimiter;
#endif

  {
    arraytype result;

  /* str1Split */
    if (delimiter->size == 1) {
      result = strChSplit(main_stri, delimiter->mem[0]);
    } else {
      result = strSplit(main_stri, delimiter);
    } /* if */
    return(result);
  } /* str1Split */
#endif



#ifdef ANSI_C

objecttype str_append (listtype arguments)
#else

objecttype str_append (arguments)
listtype arguments;
#endif

  {
    objecttype str_variable;
    stritype str_to;
    stritype str_from;
    memsizetype new_size;

  /* str_append */
    str_variable = arg_1(arguments);
    isit_stri(str_variable);
    is_variable(str_variable);
    str_to = take_stri(str_variable);
    isit_stri(arg_3(arguments));
    str_from = take_stri(arg_3(arguments));
/*
    printf("str_to (%lx) %d = ", str_to, str_to->size);
    prot_stri(str_to);
    printf("\n");
    printf("str_from (%lx) %d = ", str_from, str_from->size);
    prot_stri(str_from);
    printf("\n");
*/
    if (str_from->size != 0) {
      new_size = str_to->size + str_from->size;
      if (!RESIZE_STRI(str_to, str_to->size, new_size)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT3_STRI(str_to->size, new_size);
      str_variable->value.strivalue = str_to;
      memcpy(&str_to->mem[str_to->size], str_from->mem,
          (SIZE_TYPE) str_from->size * sizeof(strelemtype));
      str_to->size = new_size;
/*
      printf("new str_to (%lx) %d = ", str_to, str_to->size);
      prot_stri(str_to);
      printf("\n");
      printf("new str_variable (%lx): ", take_stri(str_variable));
      trace1(str_variable);
      printf("\n");
*/
    } /* if */
    return(SYS_EMPTY_OBJECT);
  } /* str_append */



#ifdef ANSI_C

objecttype str_cat (listtype arguments)
#else

objecttype str_cat (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    stritype str2;
    memsizetype str1_size;
    memsizetype result_size;
    stritype result;

  /* str_cat */
    isit_stri(arg_1(arguments));
    isit_stri(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    str2 = take_stri(arg_3(arguments));
    str1_size = str1->size;
    result_size = str1_size + str2->size;
    if (TEMP_OBJECT(arg_1(arguments))) {
      result = str1;
      if (!RESIZE_STRI(result, str1_size, result_size)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT3_STRI(str1_size, result_size);
      result->size = result_size;
      memcpy(&result->mem[str1_size], str2->mem,
          (SIZE_TYPE) str2->size * sizeof(strelemtype));
      arg_1(arguments)->value.strivalue = NULL;
      return(bld_stri_temp(result));
    } else {
      if (!ALLOC_STRI(result, result_size)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } else {
        COUNT_STRI(result_size);
        result->size = result_size;
        memcpy(result->mem, str1->mem,
            (SIZE_TYPE) str1_size * sizeof(strelemtype));
        memcpy(&result->mem[str1_size], str2->mem,
            (SIZE_TYPE) str2->size * sizeof(strelemtype));
        return(bld_stri_temp(result));
      } /* if */
    } /* if */
  } /* str_cat */



#ifdef ANSI_C

objecttype str_chsplit (listtype arguments)
#else

objecttype str_chsplit (arguments)
listtype arguments;
#endif

  { /* str_chsplit */
    isit_stri(arg_1(arguments));
    isit_char(arg_2(arguments));
    return(bld_array_temp(
        strChSplit(take_stri(arg_1(arguments)), take_char(arg_2(arguments)))));
  } /* str_chsplit */



#ifdef ANSI_C

objecttype str_clit (listtype arguments)
#else

objecttype str_clit (arguments)
listtype arguments;
#endif

  {
    register strelemtype character;
    register memsizetype position;
    stritype str1;
    memsizetype length;
    memsizetype pos;
    SIZE_TYPE len;
    stritype result;

  /* str_clit */
    isit_stri(arg_1(arguments));
    str1 = take_stri(arg_1(arguments));
    length = str1->size;
    if (!ALLOC_STRI(result, (memsizetype) (4 * length + 2))) {
      return(raise_exception(SYS_MEM_EXCEPTION));
    } /* if */
    COUNT_STRI(4 * length + 2);
    result->mem[0] = (strelemtype) '"';
    pos = 1;
    for (position = 0; position < length; position++) {
      character = (int) str1->mem[position];
      /* The following comparison uses 255 instead of '\377',       */
      /* because chars might be signed and this can produce wrong   */
      /* code when '\377' is sign extended.                         */
      if (character > 255) {
        result->mem[pos] = (strelemtype) '?';
        pos++;
      } else if (no_escape_char(character)) {
        result->mem[pos] = (strelemtype) character;
        pos++;
      } else if (character < ' ') {
        len = strlen(cstri_escape_sequence[character]);
        stri_expand(&result->mem[pos],
            cstri_escape_sequence[character], len);
        pos = pos + len;
      } else if (character <= '~') {
        result->mem[pos] = (strelemtype) '\\';
        result->mem[pos + 1] = (strelemtype) character;
        pos = pos + 2;
      } else if (character == '\177') {
        stri_expand(&result->mem[pos],
            "\\177", (SIZE_TYPE) 4);
        pos = pos + 4;
      } else {
        result->mem[pos] = (strelemtype) character;
        pos++;
      } /* if */
    } /* for */
    result->mem[pos] = (strelemtype) '"';
    result->size = pos + 1;
    if (!RESIZE_STRI(result, (memsizetype) (4 * length + 2),
        (memsizetype) (pos + 1))) {
      FREE_STRI(result, (memsizetype) (4 * length + 2));
      return(raise_exception(SYS_MEM_EXCEPTION));
    } else {
      COUNT3_STRI(4 * length + 2, pos + 1);
      return(bld_stri_temp(result));
    } /* if */
  } /* str_clit */



#ifdef ANSI_C

objecttype str_cmp (listtype arguments)
#else

objecttype str_cmp (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    stritype str2;
    inttype result;

  /* str_cmp */
    isit_stri(arg_1(arguments));
    isit_stri(arg_2(arguments));
    str1 = take_stri(arg_1(arguments));
    str2 = take_stri(arg_2(arguments));
    if (str1->size < str2->size) {
      if (strelem_memcmp(str1->mem, str2->mem,
          (SIZE_TYPE) str1->size) <= 0) {
        result = -1;
      } else {
        result = 1;
      } /* if */
    } else if (str1->size > str2->size) {
      if (strelem_memcmp(str1->mem, str2->mem,
          (SIZE_TYPE) str2->size) >= 0) {
        result = 1;
      } else {
        result = -1;
      } /* if */
    } else {
      result = strelem_memcmp(str1->mem, str2->mem,
          (SIZE_TYPE) str1->size);
      if (result > 0) {
        result = 1;
      } else if (result < 0) {
        result = -1;
      } /* if */
    } /* if */
    return(bld_int_temp(result));
  } /* str_cmp */



#ifdef ANSI_C

objecttype str_cpy (listtype arguments)
#else

objecttype str_cpy (arguments)
listtype arguments;
#endif

  {
    objecttype str_to;
    objecttype str_from;
    memsizetype new_size;
    stritype new_str;

  /* str_cpy */
    str_to = arg_1(arguments);
    str_from = arg_3(arguments);
    isit_stri(str_to);
    isit_stri(str_from);
    is_variable(str_to);
    new_str = take_stri(str_to);
    if (TEMP_OBJECT(str_from)) {
      FREE_STRI(new_str, new_str->size);
      str_to->value.strivalue = take_stri(str_from);
      str_from->value.strivalue = NULL;
    } else {
      new_size = take_stri(str_from)->size;
      if (new_str->size != new_size) {
        if (!ALLOC_STRI(new_str, new_size)) {
          return(raise_exception(SYS_MEM_EXCEPTION));
        } else {
          COUNT_STRI(new_size);
          FREE_STRI(take_stri(str_to), take_stri(str_to)->size);
          str_to->value.strivalue = new_str;
          new_str->size = new_size;
        } /* if */
      } /* if */
      memcpy(new_str->mem, take_stri(str_from)->mem,
          (SIZE_TYPE) new_size * sizeof(strelemtype));
    } /* if */
    return(SYS_EMPTY_OBJECT);
  } /* str_cpy */



#ifdef ANSI_C

objecttype str_create (listtype arguments)
#else

objecttype str_create (arguments)
listtype arguments;
#endif

  {
    objecttype str_to;
    objecttype str_from;
    memsizetype new_size;
    stritype new_str;

  /* str_create */
    str_to = arg_1(arguments);
    str_from = arg_3(arguments);
    isit_stri(str_from);
    SET_CATEGORY_OF_OBJ(str_to, STRIOBJECT);
    if (TEMP_OBJECT(str_from)) {
      str_to->value.strivalue = take_stri(str_from);
      str_from->value.strivalue = NULL;
    } else {
/*    printf("str_create %d !!!\n", in_file.line); */
      new_size = take_stri(str_from)->size;
      if (!ALLOC_STRI(new_str, new_size)) {
        str_to->value.strivalue = NULL;
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT_STRI(new_size);
      str_to->value.strivalue = new_str;
      new_str->size = new_size;
      memcpy(new_str->mem, take_stri(str_from)->mem,
          (SIZE_TYPE) new_size * sizeof(strelemtype));
    } /* if */
    return(SYS_EMPTY_OBJECT);
  } /* str_create */



#ifdef ANSI_C

objecttype str_destr (listtype arguments)
#else

objecttype str_destr (arguments)
listtype arguments;
#endif

  {
    stritype old_string;

  /* str_destr */
    isit_stri(arg_1(arguments));
    old_string = take_stri(arg_1(arguments));
    if (old_string != NULL) {
      FREE_STRI(old_string, old_string->size);
      arg_1(arguments)->value.strivalue = NULL;
    } /* if */
    return(SYS_EMPTY_OBJECT);
  } /* str_destr */



#ifdef ANSI_C

objecttype str_elemcpy (listtype arguments)
#else

objecttype str_elemcpy (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    memsizetype position;

  /* str_elemcpy */
    isit_stri(arg_1(arguments));
    isit_int(arg_4(arguments));
    isit_char(arg_6(arguments));
    is_variable(arg_1(arguments));
    str1 = take_stri(arg_1(arguments));
    position = (memsizetype) take_int(arg_4(arguments));
    if (position >= 1 && position <= str1->size) {
      str1->mem[position - 1] = (strelemtype) take_char(arg_6(arguments));
    } else {
      return(raise_exception(SYS_RNG_EXCEPTION));
    } /* if */
    return(SYS_EMPTY_OBJECT);
  } /* str_elemcpy */



#ifdef ANSI_C

objecttype str_eq (listtype arguments)
#else

objecttype str_eq (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    stritype str2;

  /* str_eq */
    isit_stri(arg_1(arguments));
    isit_stri(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    str2 = take_stri(arg_3(arguments));
    if (str1->size == str2->size && memcmp(str1->mem, str2->mem,
        (SIZE_TYPE) str1->size * sizeof(strelemtype)) == 0) {
      return(SYS_TRUE_OBJECT);
    } else {
      return(SYS_FALSE_OBJECT);
    } /* if */
  } /* str_eq */



#ifdef ANSI_C

objecttype str_ge (listtype arguments)
#else

objecttype str_ge (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    stritype str2;
    objecttype result;

  /* str_ge */
    isit_stri(arg_1(arguments));
    isit_stri(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    str2 = take_stri(arg_3(arguments));
    if (str1->size >= str2->size) {
      if (strelem_memcmp(str1->mem, str2->mem,
          (SIZE_TYPE) str2->size) >= 0) {
        result = SYS_TRUE_OBJECT;
      } else {
        result = SYS_FALSE_OBJECT;
      } /* if */
    } else {
      if (strelem_memcmp(str1->mem, str2->mem,
          (SIZE_TYPE) str1->size) > 0) {
        result = SYS_TRUE_OBJECT;
      } else {
        result = SYS_FALSE_OBJECT;
      } /* if */
    } /* if */
    return(result);
  } /* str_ge */



#ifdef ANSI_C

objecttype str_gt (listtype arguments)
#else

objecttype str_gt (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    stritype str2;
    objecttype result;

  /* str_gt */
    isit_stri(arg_1(arguments));
    isit_stri(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    str2 = take_stri(arg_3(arguments));
    if (str1->size > str2->size) {
      if (strelem_memcmp(str1->mem, str2->mem,
          (SIZE_TYPE) str2->size) >= 0) {
        result = SYS_TRUE_OBJECT;
      } else {
        result = SYS_FALSE_OBJECT;
      } /* if */
    } else {
      if (strelem_memcmp(str1->mem, str2->mem,
          (SIZE_TYPE) str1->size) > 0) {
        result = SYS_TRUE_OBJECT;
      } else {
        result = SYS_FALSE_OBJECT;
      } /* if */
    } /* if */
    return(result);
  } /* str_gt */



#ifdef ANSI_C

objecttype str_hashcode (listtype arguments)
#else

objecttype str_hashcode (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    inttype result;

  /* str_hashcode */
    isit_stri(arg_1(arguments));
    str1 = take_stri(arg_1(arguments));
    if (str1->size == 0) {
      result = 0;
    } else {
      result = str1->mem[0] << 5 ^ str1->size << 3 ^ str1->mem[str1->size - 1];
    } /* if */
    return(bld_int_temp(result));
  } /* str_hashcode */



#ifdef ANSI_C

objecttype str_head (listtype arguments)
#else

objecttype str_head (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    inttype stop;
    memsizetype length;
    memsizetype result_size;
    stritype result;

  /* str_head */
    isit_stri(arg_1(arguments));
    isit_int(arg_4(arguments));
    str1 = take_stri(arg_1(arguments));
    stop = take_int(arg_4(arguments));
    length = str1->size;
    if (stop >= 1 && length >= 1) {
      if (length <= (memsizetype) stop) {
        result_size = length;
      } else {
        result_size = (memsizetype) stop;
      } /* if */
      if (TEMP_OBJECT(arg_1(arguments))) {
        result = str1;
        if (!RESIZE_STRI(result, length, result_size)) {
          return(raise_exception(SYS_MEM_EXCEPTION));
        } /* if */
        COUNT3_STRI(length, result_size);
        result->size = result_size;
        arg_1(arguments)->value.strivalue = NULL;
      } else {
        if (!ALLOC_STRI(result, result_size)) {
          return(raise_exception(SYS_MEM_EXCEPTION));
        } /* if */
        COUNT_STRI(result_size);
        result->size = result_size;
        memcpy(result->mem, str1->mem,
            (SIZE_TYPE) result_size * sizeof(strelemtype));
      } /* if */
    } else {
      if (!ALLOC_STRI(result, (memsizetype) 0)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT_STRI(0);
      result->size = 0;
    } /* if */
    return(bld_stri_temp(result));
  } /* str_head */



#ifdef ANSI_C

objecttype str_idx (listtype arguments)
#else

objecttype str_idx (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    memsizetype position;

  /* str_idx */
    isit_stri(arg_1(arguments));
    isit_int(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    position = (memsizetype) take_int(arg_3(arguments));
    if (position >= 1 && position <= str1->size) {
      return(bld_char_temp((chartype) str1->mem[position - 1]));
    } else {
      return(bld_char_temp('\0'));
    } /* if */
  } /* str_idx */



#ifdef ANSI_C

objecttype str_ipos (listtype arguments)
#else

objecttype str_ipos (arguments)
listtype arguments;
#endif

  { /* str_ipos */
    isit_stri(arg_1(arguments));
    isit_stri(arg_2(arguments));
    return(bld_int_temp(
        strIpos(take_stri(arg_1(arguments)), take_stri(arg_2(arguments)),
                take_int(arg_3(arguments)))));
  } /* str_ipos */



#ifdef ANSI_C

objecttype str_le (listtype arguments)
#else

objecttype str_le (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    stritype str2;
    objecttype result;

  /* str_le */
    isit_stri(arg_1(arguments));
    isit_stri(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    str2 = take_stri(arg_3(arguments));
    if (str1->size <= str2->size) {
      if (strelem_memcmp(str1->mem, str2->mem,
          (SIZE_TYPE) str1->size) <= 0) {
        result = SYS_TRUE_OBJECT;
      } else {
        result = SYS_FALSE_OBJECT;
      } /* if */
    } else {
      if (strelem_memcmp(str1->mem, str2->mem,
          (SIZE_TYPE) str2->size) < 0) {
        result = SYS_TRUE_OBJECT;
      } else {
        result = SYS_FALSE_OBJECT;
      } /* if */
    } /* if */
    return(result);
  } /* str_le */



#ifdef ANSI_C

objecttype str_lit (listtype arguments)
#else

objecttype str_lit (arguments)
listtype arguments;
#endif

  { /* str_lit */
    isit_stri(arg_1(arguments));
    return(bld_stri_temp(strLit(
        take_stri(arg_1(arguments)))));
  } /* str_lit */



#ifdef ANSI_C

objecttype str_lng (listtype arguments)
#else

objecttype str_lng (arguments)
listtype arguments;
#endif

  { /* str_lng */
    isit_stri(arg_1(arguments));
    return(bld_int_temp((inttype) take_stri(arg_1(arguments))->size));
  } /* str_lng */



#ifdef ANSI_C

objecttype str_low (listtype arguments)
#else

objecttype str_low (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    memsizetype length;
    memsizetype pos;
    stritype result;

  /* str_low */
    isit_stri(arg_1(arguments));
    str1 = take_stri(arg_1(arguments));
    length = str1->size;
    if (!ALLOC_STRI(result, length)) {
      return(raise_exception(SYS_MEM_EXCEPTION));
    } else {
      COUNT_STRI(length);
      result->size = length;
      for (pos = 0; pos < length; pos++) {
#ifdef WIDE_CHAR_STRINGS
        if (((int) str1->mem[pos]) >= 'A' && ((int) str1->mem[pos]) <= 'Z') {
          result->mem[pos] = (strelemtype) (((int) str1->mem[pos]) - 'A' + 'a');
        } else {
          result->mem[pos] = str1->mem[pos];
        } /* if */
#else
        result->mem[pos] = (strelemtype) tolower((int) str1->mem[pos]);
#endif
      } /* for */
      return(bld_stri_temp(result));
    } /* if */
  } /* str_low */



#ifdef ANSI_C

objecttype str_lpad (listtype arguments)
#else

objecttype str_lpad (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    inttype pad_size;
    memsizetype f_size;
    memsizetype length;
    stritype result;

  /* str_lpad */
    isit_stri(arg_1(arguments));
    isit_int(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    pad_size = take_int(arg_3(arguments));
    length = str1->size;
    if (pad_size > (inttype) length) {
      f_size = (memsizetype) pad_size;
      if (!ALLOC_STRI(result, f_size)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT_STRI(f_size);
      result->size = f_size;
#ifdef WIDE_CHAR_STRINGS
      {
        strelemtype *elem = result->mem;
        memsizetype len = f_size - length;

        while (len--) {
          *elem++ = (strelemtype) ' ';
        } /* while */
      }
#else
      memset(result->mem, ' ', (SIZE_TYPE) (f_size - length));
#endif
      memcpy(&result->mem[f_size - length], str1->mem,
          (SIZE_TYPE) length * sizeof(strelemtype));
    } else {
      if (TEMP_OBJECT(arg_1(arguments))) {
        result = str1;
        arg_1(arguments)->value.strivalue = NULL;
      } else {
        if (!ALLOC_STRI(result, length)) {
          return(raise_exception(SYS_MEM_EXCEPTION));
        } /* if */
        COUNT_STRI(length);
        result->size = length;
        memcpy(result->mem, str1->mem,
            (SIZE_TYPE) length * sizeof(strelemtype));
      } /* if */
    } /* if */
    return(bld_stri_temp(result));
  } /* str_lpad */



#ifdef ANSI_C

objecttype str_lt (listtype arguments)
#else

objecttype str_lt (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    stritype str2;
    objecttype result;

  /* str_lt */
    isit_stri(arg_1(arguments));
    isit_stri(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    str2 = take_stri(arg_3(arguments));
    if (str1->size < str2->size) {
      if (strelem_memcmp(str1->mem, str2->mem,
          (SIZE_TYPE) str1->size) <= 0) {
        result = SYS_TRUE_OBJECT;
      } else {
        result = SYS_FALSE_OBJECT;
      } /* if */
    } else {
      if (strelem_memcmp(str1->mem, str2->mem,
          (SIZE_TYPE) str2->size) < 0) {
        result = SYS_TRUE_OBJECT;
      } else {
        result = SYS_FALSE_OBJECT;
      } /* if */
    } /* if */
    return(result);
  } /* str_lt */



#ifdef ANSI_C

objecttype str_mult (listtype arguments)
#else

objecttype str_mult (arguments)
listtype arguments;
#endif

  {
    inttype factor;
    stritype str1;
    memsizetype len;
    inttype number;
    strelemtype *result_pointer;
    memsizetype result_size;
    stritype result;

  /* str_mult */
    isit_stri(arg_1(arguments));
    isit_int(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    factor = take_int(arg_3(arguments));
    if (factor < 0) {
      return(raise_exception(SYS_RNG_EXCEPTION));
    } else {
      len = str1->size;
      result_size = ((memsizetype) factor) * len;
      if (!ALLOC_STRI(result, result_size)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } else {
        COUNT_STRI(result_size);
        result->size = result_size;
        if (len != 0) {
          if (len == 1) {
#ifdef WIDE_CHAR_STRINGS
            result_pointer = result->mem;
            for (number = factor; number > 0; number--) {
              *result_pointer++ = str1->mem[0];
            } /* for */
#else
            memset(result->mem, (int) str1->mem[0], (SIZE_TYPE) factor);
#endif
          } else {
            result_pointer = result->mem;
            for (number = factor; number > 0; number--) {
              memcpy(result_pointer, str1->mem,
                  (SIZE_TYPE) len * sizeof(strelemtype));
              result_pointer += (SIZE_TYPE) len;
            } /* for */
          } /* if */
        } /* if */
        return(bld_stri_temp(result));
      } /* if */
    } /* if */
  } /* str_mult */



#ifdef ANSI_C

objecttype str_ne (listtype arguments)
#else

objecttype str_ne (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    stritype str2;

  /* str_ne */
    isit_stri(arg_1(arguments));
    isit_stri(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    str2 = take_stri(arg_3(arguments));
    if (str1->size != str2->size || memcmp(str1->mem, str2->mem,
        (SIZE_TYPE) str1->size * sizeof(strelemtype)) != 0) {
      return(SYS_TRUE_OBJECT);
    } else {
      return(SYS_FALSE_OBJECT);
    } /* if */
  } /* str_ne */



#ifdef ANSI_C

objecttype str_pos (listtype arguments)
#else

objecttype str_pos (arguments)
listtype arguments;
#endif

  { /* str_pos */
    isit_stri(arg_1(arguments));
    isit_stri(arg_2(arguments));
    return(bld_int_temp(
        strPos(take_stri(arg_1(arguments)), take_stri(arg_2(arguments)))));
  } /* str_pos */



#ifdef ANSI_C

objecttype str_range (listtype arguments)
#else

objecttype str_range (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    inttype start;
    inttype stop;
    memsizetype length;
    memsizetype result_size;
    stritype result;

  /* str_range */
    isit_stri(arg_1(arguments));
    isit_int(arg_3(arguments));
    isit_int(arg_5(arguments));
    str1 = take_stri(arg_1(arguments));
    start = take_int(arg_3(arguments));
    stop = take_int(arg_5(arguments));
    length = str1->size;
    if (stop >= 1 && stop >= start && start <= ((inttype) length) &&
        length >= 1) {
      if (start < 1) {
        start = 1;
      } /* if */
      if (stop > (inttype) length) {
        stop = (inttype) length;
      } /* if */
      result_size = (memsizetype) (stop - start + 1);
      if (!ALLOC_STRI(result, result_size)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT_STRI(result_size);
      /* Reversing the order of the following two statements    */
      /* causes an "Internal Compiler Error" with MSC 6.0       */
      /* when using the -Ozacegilt optimisation option in the   */
      /* large memory model (-AL). Note that the order of the   */
      /* two statements make no difference to the logic of the  */
      /* program.                                               */
      memcpy(result->mem, &str1->mem[start - 1],
          (SIZE_TYPE) result_size * sizeof(strelemtype));
      result->size = result_size;
    } else {
      if (!ALLOC_STRI(result, (memsizetype) 0)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT_STRI(0);
      result->size = 0;
    } /* if */
    return(bld_stri_temp(result));
  } /* str_range */



#ifdef ANSI_C

objecttype str_repl (listtype arguments)
#else

objecttype str_repl (arguments)
listtype arguments;
#endif

  { /* str_repl */
    isit_stri(arg_1(arguments));
    isit_stri(arg_2(arguments));
    isit_stri(arg_3(arguments));
    return(bld_stri_temp(
        strRepl(take_stri(arg_1(arguments)),
                take_stri(arg_2(arguments)),
                take_stri(arg_3(arguments)))));
  } /* str_repl */



#ifdef ANSI_C

objecttype str_rpad (listtype arguments)
#else

objecttype str_rpad (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    inttype pad_size;
    memsizetype f_size;
    memsizetype length;
    stritype result;

  /* str_rpad */
    isit_stri(arg_1(arguments));
    isit_int(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    pad_size = take_int(arg_3(arguments));
    length = str1->size;
    if (pad_size > (inttype) length) {
      f_size = (memsizetype) pad_size;
      if (!ALLOC_STRI(result, f_size)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT_STRI(f_size);
      result->size = f_size;
      memcpy(result->mem, str1->mem,
          (SIZE_TYPE) length * sizeof(strelemtype));
#ifdef WIDE_CHAR_STRINGS
      {
        strelemtype *elem = &result->mem[length];
        memsizetype len = f_size - length;

        while (len--) {
          *elem++ = (strelemtype) ' ';
        } /* while */
      }
#else
      memset(&result->mem[length], ' ',
          (SIZE_TYPE) (f_size - length));
#endif
    } else {
      if (TEMP_OBJECT(arg_1(arguments))) {
        result = str1;
        arg_1(arguments)->value.strivalue = NULL;
      } else {
        if (!ALLOC_STRI(result, length)) {
          return(raise_exception(SYS_MEM_EXCEPTION));
        } /* if */
        COUNT_STRI(length);
        result->size = length;
        memcpy(result->mem, str1->mem,
            (SIZE_TYPE) length * sizeof(strelemtype));
      } /* if */
    } /* if */
    return(bld_stri_temp(result));
  } /* str_rpad */



#ifdef ANSI_C

objecttype str_rpos (listtype arguments)
#else

objecttype str_rpos (arguments)
listtype arguments;
#endif

  { /* str_rpos */
    isit_stri(arg_1(arguments));
    isit_stri(arg_2(arguments));
    return(bld_int_temp(
        strRpos(take_stri(arg_1(arguments)), take_stri(arg_2(arguments)))));
  } /* str_rpos */



#ifdef ANSI_C

objecttype str_split (listtype arguments)
#else

objecttype str_split (arguments)
listtype arguments;
#endif

  { /* str_split */
    isit_stri(arg_1(arguments));
    isit_stri(arg_2(arguments));
    return(bld_array_temp(
        strSplit(take_stri(arg_1(arguments)), take_stri(arg_2(arguments)))));
  } /* str_split */



#ifdef ANSI_C

objecttype str_str (listtype arguments)
#else

objecttype str_str (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    stritype result;

  /* str_str */
    isit_stri(arg_1(arguments));
    str1 = take_stri(arg_1(arguments));
    if (TEMP_OBJECT(arg_1(arguments))) {
      result = str1;
      arg_1(arguments)->value.strivalue = NULL;
      return(bld_stri_temp(result));
    } else {
      if (!ALLOC_STRI(result, str1->size)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } else {
        COUNT_STRI(str1->size);
        result->size = str1->size;
        memcpy(result->mem, str1->mem,
            (SIZE_TYPE) str1->size * sizeof(strelemtype));
        return(bld_stri_temp(result));
      } /* if */
    } /* if */
  } /* str_str */



#ifdef ANSI_C

objecttype str_substr (listtype arguments)
#else

objecttype str_substr (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    inttype start;
    inttype len;
    memsizetype length;
    memsizetype result_size;
    stritype result;

  /* str_substr */
    isit_stri(arg_1(arguments));
    isit_int(arg_3(arguments));
    isit_int(arg_5(arguments));
    str1 = take_stri(arg_1(arguments));
    start = take_int(arg_3(arguments));
    len = take_int(arg_5(arguments));
    length = str1->size;
    if (len >= 1 && start + len > 1 && start <= ((inttype) length) &&
        length >= 1) {
      if (start < 1) {
        len += start - 1;
        start = 1;
      } /* if */
      if (start + len - 1 > (inttype) length) {
        result_size = (memsizetype) (length - start + 1);
      } else {
        result_size = (memsizetype) len;
      } /* if */
      if (!ALLOC_STRI(result, result_size)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT_STRI(result_size);
      memcpy(result->mem, &str1->mem[start - 1],
          (SIZE_TYPE) result_size * sizeof(strelemtype));
      result->size = result_size;
    } else {
      if (!ALLOC_STRI(result, (memsizetype) 0)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT_STRI(0);
      result->size = 0;
    } /* if */
    return(bld_stri_temp(result));
  } /* str_substr */



#ifdef ANSI_C

objecttype str_tail (listtype arguments)
#else

objecttype str_tail (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    inttype start;
    memsizetype length;
    memsizetype result_size;
    stritype result;

  /* str_tail */
    isit_stri(arg_1(arguments));
    isit_int(arg_3(arguments));
    str1 = take_stri(arg_1(arguments));
    start = take_int(arg_3(arguments));
    length = str1->size;
    if (start <= (inttype) length && length >= 1) {
      if (start < 1) {
        start = 1;
      } /* if */
      result_size = length - ((memsizetype) start) + 1;
      if (!ALLOC_STRI(result, result_size)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT_STRI(result_size);
      /* Reversing the order of the following two statements    */
      /* causes an "Internal Compiler Error" with MSC 6.0       */
      /* when using the -Ozacegilt optimisation option in the   */
      /* large memory model (-AL). Note that the order of the   */
      /* two statements make no difference to the logic of the  */
      /* program.                                               */
      memcpy(result->mem, &str1->mem[start - 1],
          (SIZE_TYPE) result_size * sizeof(strelemtype));
      result->size = result_size;
    } else {
      if (!ALLOC_STRI(result, (memsizetype) 0)) {
        return(raise_exception(SYS_MEM_EXCEPTION));
      } /* if */
      COUNT_STRI(0);
      result->size = 0;
    } /* if */
    return(bld_stri_temp(result));
  } /* str_tail */



#ifdef ANSI_C

objecttype str_trim (listtype arguments)
#else

objecttype str_trim (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    memsizetype start;
    memsizetype length;
    stritype result;

  /* str_trim */
    isit_stri(arg_1(arguments));
    str1 = take_stri(arg_1(arguments));
    start = 0;
    length = str1->size;
    if (length >= 1) {
      while (start < length && str1->mem[start] <= ' ') {
        start++;
      } /* while */
      while (length > start && str1->mem[length - 1] <= ' ') {
        length--;
      } /* while */
      length -= start;
    } /* if */
    if (!ALLOC_STRI(result, length)) {
      return(raise_exception(SYS_MEM_EXCEPTION));
    } else {
      COUNT_STRI(length);
      result->size = length;
      memcpy(result->mem, &str1->mem[start],
          (SIZE_TYPE) length * sizeof(strelemtype));
      return(bld_stri_temp(result));
    } /* if */
  } /* str_trim */



#ifdef ANSI_C

objecttype str_up (listtype arguments)
#else

objecttype str_up (arguments)
listtype arguments;
#endif

  {
    stritype str1;
    memsizetype length;
    memsizetype pos;
    stritype result;

  /* str_up */
    isit_stri(arg_1(arguments));
    str1 = take_stri(arg_1(arguments));
    length = str1->size;
    if (!ALLOC_STRI(result, length)) {
      return(raise_exception(SYS_MEM_EXCEPTION));
    } else {
      COUNT_STRI(length);
      result->size = length;
      for (pos = 0; pos < length; pos++) {
#ifdef WIDE_CHAR_STRINGS
        if (((int) str1->mem[pos]) >= 'a' && ((int) str1->mem[pos]) <= 'z') {
          result->mem[pos] = (strelemtype) (((int) str1->mem[pos]) - 'a' + 'A');
        } else {
          result->mem[pos] = str1->mem[pos];
        } /* if */
#else
        result->mem[pos] = (strelemtype) toupper((int) str1->mem[pos]);
#endif
      } /* for */
      return(bld_stri_temp(result));
    } /* if */
  } /* str_up */



#ifdef ANSI_C

objecttype str_value (listtype arguments)
#else

objecttype str_value (arguments)
listtype arguments;
#endif

  {
    objecttype obj_arg;
    stritype str1;
    stritype result;

  /* str_value */
    isit_reference(arg_3(arguments));
    obj_arg = take_reference(arg_3(arguments));
    isit_stri(obj_arg);
    str1 = take_stri(obj_arg);

    if (!ALLOC_STRI(result, str1->size)) {
      return(raise_exception(SYS_MEM_EXCEPTION));
    } else {
      COUNT_STRI(str1->size);
      result->size = str1->size;
      memcpy(result->mem, str1->mem,
          (SIZE_TYPE) (result->size * sizeof(strelemtype)));
      return(bld_stri_temp(result));
    } /* if */
  } /* str_value */
