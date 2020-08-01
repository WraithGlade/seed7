/********************************************************************/
/*                                                                  */
/*  s7   Seed7 interpreter                                          */
/*  Copyright (C) 1990 - 2010  Thomas Mertes                        */
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
/*  Module: Compiler data reflection                                */
/*  File: seed7/src/typ_data.c                                      */
/*  Changes: 1993, 1994, 1999, 2000, 2008, 2010  Thomas Mertes      */
/*  Content: Primitive actions for the type type.                   */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "common.h"
#include "data.h"
#include "data_rtl.h"
#include "heaputl.h"
#include "datautl.h"
#include "typeutl.h"
#include "striutl.h"
#include "rtl_err.h"

#undef EXTERN
#define EXTERN
#include "typ_data.h"


#define TYPE_TABLE_INCREMENT 127

#if 0
typedef struct typestruct     *typetype;
typedef const struct typestruct     *const_typetype;

typedef struct typestruct {
    /* objecttype match_obj; */
    typetype meta;
    typetype func_type;
    typetype varfunc_type;
    typetype result_type;
    booltype is_varfunc_type;
    /* typelisttype interfaces; */
    /* identtype name; */
    /*
    objecttype create_call_obj;
    objecttype destroy_call_obj;
    objecttype copy_call_obj;
    objecttype ord_call_obj;
    objecttype in_call_obj;
    */
  } typerecord;
#endif



#if 0
inttype typHashCode (typetype type1)

  { /* typHashCode */
    return (inttype) type1;
  } /* typHashCode */



objecttype typHashcode (listtype arguments)

  { /* typHashcode */
    isit_type(arg_1(arguments));
    return bld_int_temp((inttype) take_type(arg_1(arguments)));
  } /* typHashcode */
#endif



inttype typCmpGeneric (rtlGenerictype type1, rtlGenerictype type2)

  { /* typCmpGeneric */
    if (type1 < type2) {
      return -1;
    } else if (type1 > type2) {
      return 1;
    } else {
      return 0;
    } /* if */
  } /* typCmpGeneric */



void typCpy (typetype *dest, typetype source)

  { /* typCpy */
    *dest = source;
  } /* typCpy */



typetype typCreate (typetype type_from)

  { /* typCreate */
    return type_from;
  } /* typCreate */



void typDestr (typetype old_type)

  { /* typDestr */
  } /* typDestr */



typetype typFunc (typetype basic_type)

  {
    typetype result;

  /* typFunc */
    if ((result = get_func_type(NULL, basic_type)) == NULL) {
      raise_error(MEMORY_ERROR);
      return NULL;
    } else {
      return result;
    } /* if */
  } /* typFunc */



booltype typIsDerived (typetype any_type)

  { /* typIsDerived */
    return any_type->meta != NULL;
  } /* typIsDerived */



booltype typIsFunc (typetype any_type)

  { /* typIsFunc */
    return any_type->result_type != NULL && !any_type->is_varfunc_type;
  } /* typIsFunc */



booltype typIsVarfunc (typetype any_type)

  { /* typIsVarfunc */
    return any_type->result_type != NULL && any_type->is_varfunc_type;
  } /* typIsVarfunc */



objecttype typMatchobj (typetype actual_type)

  { /* typMatchobj */
    return actual_type->match_obj;
  } /* typMatchobj */



typetype typMeta (typetype any_type)

  { /* typMeta */
    if (any_type->meta == NULL) {
      raise_error(RANGE_ERROR);
      return NULL;
    } else {
      return any_type->meta;
    } /* if */
  } /* typMeta */



inttype typNum (typetype actual_type)

  {
    static unsigned int table_size = 0;
    static unsigned int table_used = 0;
    static typetype *type_table = NULL;
    register typetype *actual_type_ptr;
    unsigned int result;

  /* typNum */
    if (actual_type == NULL) {
      result = 0;
    } else {
      table_used++;
      if (table_used > table_size) {
        if (type_table == NULL) {
          (void) ALLOC_TABLE(type_table, typetype, table_used + TYPE_TABLE_INCREMENT);
        } else {
          type_table = REALLOC_TABLE(type_table, typetype,
              table_size, table_used + TYPE_TABLE_INCREMENT);
          if (type_table != NULL) {
            COUNT3_TABLE(typetype, table_size, table_used + TYPE_TABLE_INCREMENT);
          } /* if */
        } /* if */
        if (type_table == NULL) {
          table_size = 0;
          table_used = 0;
          raise_error(MEMORY_ERROR);
          return 0;
        } /* if */
        table_size = table_used + TYPE_TABLE_INCREMENT;
      } /* if */
      type_table[table_used - 1] = actual_type;
      actual_type_ptr = type_table;
      while (*actual_type_ptr != actual_type) {
        actual_type_ptr++;
      } /* while */
      result = (unsigned int) (actual_type_ptr - type_table);
      if (result != table_used - 1) {
        table_used--;
      } /* if */
      result++;
    } /* if */
    /* printf("typ_num: %lx %lx %lu\n", arg_1(arguments), actual_type, result); */
    return (inttype) result;
  } /* typNum */



typetype typResult (typetype any_type)

  { /* typResult */
    if (any_type->result_type == NULL) {
      raise_error(RANGE_ERROR);
      return NULL;
    } else {
      return any_type->result_type;
    } /* if */
  } /* typResult */



stritype typStr (typetype type_arg)

  {
    const_cstritype stri;
    stritype result;

  /* typStr */
    if (type_arg->name != NULL) {
      stri = id_string(type_arg->name);
/*  } else if type_arg->result_type != NULL &&
        type_arg->result_type->name != NULL) {
      stri = id_string(type_arg->result_type->name); */
    } else {
      stri = "*ANONYM_TYPE*";
    } /* if */
    result = cstri_to_stri(stri);
    if (result == NULL) {
      raise_error(MEMORY_ERROR);
    } /* if */
    return result;
  } /* typStr */



typetype typVarfunc (typetype basic_type)

  {
    typetype result;

  /* typVarfunc */
    if ((result = get_varfunc_type(NULL, basic_type)) == NULL) {
      raise_error(MEMORY_ERROR);
      return NULL;
    } else {
      return result;
    } /* if */
  } /* typVarfunc */
