/********************************************************************/
/*                                                                  */
/*  hi   Interpreter for Seed7 programs.                            */
/*  Copyright (C) 1990 - 2000  Thomas Mertes                        */
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
/*  License along with this program; if not, write to the           */
/*  Free Software Foundation, Inc., 51 Franklin Street,             */
/*  Fifth Floor, Boston, MA  02110-1301, USA.                       */
/*                                                                  */
/*  Module: Library                                                 */
/*  File: seed7/src/cmdlib.c                                        */
/*  Changes: 1994  Thomas Mertes                                    */
/*  Content: Primitive actions for various commands.                */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "common.h"
#include "data.h"
#include "heaputl.h"
#include "flistutl.h"
#include "syvarutl.h"
#include "striutl.h"
#include "runerr.h"
#include "memory.h"
#include "dir_rtl.h"
#include "str_rtl.h"
#include "cmd_rtl.h"

#undef EXTERN
#define EXTERN
#include "cmdlib.h"



#ifdef ANSI_C

#ifdef USE_CDECL
static int _cdecl cmp_mem (char *strg1, char *strg2)
#else
static int cmp_mem (void const *strg1, void const *strg2)
#endif
#else

static int cmp_mem (strg1, strg2)
char *strg1;
char *strg2;
#endif

  { /* cmp_mem */
    return(strCompare(
        ((objecttype) strg1)->value.strivalue,
        ((objecttype) strg2)->value.strivalue));
  } /* cmp_mem */



#ifdef ANSI_C

static arraytype read_dir (dirtype directory)
#else

static arraytype read_dir (directory)
dirtype directory;
#endif

  {
    arraytype dir_array;
    arraytype resized_dir_array;
    memsizetype max_array_size;
    memsizetype used_array_size;
    memsizetype position;
    stritype stri1;
    booltype okay;

  /* read_dir */
    max_array_size = 256;
    if (ALLOC_ARRAY(dir_array, max_array_size)) {
      used_array_size = 0;
      stri1 = dirRead(directory);
      okay = TRUE;
      while (okay && stri1 != NULL) {
        if (used_array_size >= max_array_size) {
          resized_dir_array = REALLOC_ARRAY(dir_array,
              max_array_size, max_array_size + 256);
          if (resized_dir_array == NULL) {
            okay = FALSE;
          } else {
            dir_array = resized_dir_array;
            COUNT3_ARRAY(max_array_size, max_array_size + 256);
            max_array_size += 256;
          } /* if */
        } /* if */
        if (okay) {
          dir_array->arr[(int) used_array_size].type_of = take_type(SYS_STRI_TYPE);
          dir_array->arr[(int) used_array_size].descriptor.property = NULL;
          dir_array->arr[(int) used_array_size].value.strivalue = stri1;
          INIT_CATEGORY_OF_VAR(&dir_array->arr[(int) used_array_size],
              STRIOBJECT);
          used_array_size++;
          stri1 = dirRead(directory);
        } /* if */
      } /* while */
      if (okay) {
        resized_dir_array = REALLOC_ARRAY(dir_array,
            max_array_size, used_array_size);
        if (resized_dir_array == NULL) {
          okay = FALSE;
        } else {
          dir_array = resized_dir_array;
          COUNT3_ARRAY(max_array_size, used_array_size);
          dir_array->min_position = 1;
          dir_array->max_position = used_array_size;
        } /* if */
      } /* if */
      if (!okay) {
        for (position = 0; position < used_array_size; position++) {
          FREE_STRI(dir_array->arr[(int) position].value.strivalue,
              dir_array->arr[(int) position].value.strivalue->size);
        } /* for */
        FREE_ARRAY(dir_array, max_array_size);
        dir_array = NULL;
      } /* if */
    } /* if */
    return(dir_array);
  } /* read_dir */



#ifdef ANSI_C

objecttype cmd_big_filesize (listtype arguments)
#else

objecttype cmd_big_filesize (arguments)
listtype arguments;
#endif

  { /* cmd_big_filesize */
    isit_stri(arg_1(arguments));
    return(bld_bigint_temp(
        cmdBigFileSize(take_stri(arg_1(arguments)))));
  } /* cmd_big_filesize */



#ifdef ANSI_C

objecttype cmd_chdir (listtype arguments)
#else

objecttype cmd_chdir (arguments)
listtype arguments;
#endif

  { /* cmd_chdir */
    isit_stri(arg_1(arguments));
    cmdChdir(take_stri(arg_1(arguments)));
    return(SYS_EMPTY_OBJECT);
  } /* cmd_chdir */



#ifdef ANSI_C

objecttype cmd_config_value (listtype arguments)
#else

objecttype cmd_config_value (arguments)
listtype arguments;
#endif

  { /* cmd_config_value */
    isit_stri(arg_1(arguments));
    return(bld_stri_temp(cmdConfigValue(take_stri(arg_1(arguments)))));
  } /* cmd_config_value */



#ifdef ANSI_C

objecttype cmd_copy (listtype arguments)
#else

objecttype cmd_copy (arguments)
listtype arguments;
#endif

  { /* cmd_copy */
    isit_stri(arg_1(arguments));
    isit_stri(arg_2(arguments));
    cmdCopy(take_stri(arg_1(arguments)), take_stri(arg_2(arguments)));
    return(SYS_EMPTY_OBJECT);
  } /* cmd_copy */



#ifdef ANSI_C

objecttype cmd_filesize (listtype arguments)
#else

objecttype cmd_filesize (arguments)
listtype arguments;
#endif

  { /* cmd_filesize */
    isit_stri(arg_1(arguments));
    return(bld_int_temp(
        cmdFileSize(take_stri(arg_1(arguments)))));
  } /* cmd_filesize */



#ifdef ANSI_C

objecttype cmd_filetype (listtype arguments)
#else

objecttype cmd_filetype (arguments)
listtype arguments;
#endif

  { /* cmd_filetype */
    isit_stri(arg_1(arguments));
    return(bld_int_temp(
        cmdFileType(take_stri(arg_1(arguments)))));
  } /* cmd_filetype */



#ifdef ANSI_C

objecttype cmd_getcwd (listtype arguments)
#else

objecttype cmd_getcwd (arguments)
listtype arguments;
#endif

  { /* cmd_getcwd */
    return(bld_stri_temp(cmdGetcwd()));
  } /* cmd_getcwd */



#ifdef ANSI_C

objecttype cmd_ls (listtype arguments)
#else

objecttype cmd_ls (arguments)
listtype arguments;
#endif

  {
    stritype dir_name;
    dirtype directory;
    arraytype result;

  /* cmd_ls */
    isit_stri(arg_1(arguments));
    dir_name = take_stri(arg_1(arguments));
    if ((directory = dirOpen(dir_name)) != NULL) {
      result = read_dir(directory);
      dirClose(directory);
      if (result == NULL) {
        return(raise_with_arguments(SYS_MEM_EXCEPTION, arguments));
      } else {
        qsort((void *) result->arr,
            (size_t) (result->max_position - result->min_position + 1),
            sizeof(objectrecord), &cmp_mem);
        return(bld_array_temp(result));
      } /* if */
    } else {
      return(raise_with_arguments(SYS_FIL_EXCEPTION, arguments));
    } /* if */
  } /* cmd_ls */



#ifdef ANSI_C

objecttype cmd_mkdir (listtype arguments)
#else

objecttype cmd_mkdir (arguments)
listtype arguments;
#endif

  { /* cmd_mkdir */
    isit_stri(arg_1(arguments));
    cmdMkdir(take_stri(arg_1(arguments)));
    return(SYS_EMPTY_OBJECT);
  } /* cmd_mkdir */



#ifdef ANSI_C

objecttype cmd_move (listtype arguments)
#else

objecttype cmd_move (arguments)
listtype arguments;
#endif

  { /* cmd_move */
    isit_stri(arg_1(arguments));
    isit_stri(arg_2(arguments));
    cmdMove(take_stri(arg_1(arguments)), take_stri(arg_2(arguments)));
    return(SYS_EMPTY_OBJECT);
  } /* cmd_move */



#ifdef ANSI_C

objecttype cmd_remove (listtype arguments)
#else

objecttype cmd_remove (arguments)
listtype arguments;
#endif

  { /* cmd_remove */
    isit_stri(arg_1(arguments));
    cmdRemove(take_stri(arg_1(arguments)));
    return(SYS_EMPTY_OBJECT);
  } /* cmd_remove */



#ifdef ANSI_C

objecttype cmd_sh (listtype arguments)
#else

objecttype cmd_sh (arguments)
listtype arguments;
#endif

  { /* cmd_sh */
    isit_stri(arg_1(arguments));
    cmdSh(take_stri(arg_1(arguments)));
    return(SYS_EMPTY_OBJECT);
  } /* cmd_sh */



#ifdef ANSI_C

objecttype cmd_symlink (listtype arguments)
#else

objecttype cmd_symlink (arguments)
listtype arguments;
#endif

  { /* cmd_symlink */
    isit_stri(arg_1(arguments));
    isit_stri(arg_2(arguments));
    cmdSymlink(take_stri(arg_1(arguments)), take_stri(arg_2(arguments)));
    return(SYS_EMPTY_OBJECT);
  } /* cmd_symlink */
