/********************************************************************/
/*                                                                  */
/*  chr_rtl.c     Primitive actions for the integer type.           */
/*  Copyright (C) 1989 - 2013  Thomas Mertes                        */
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
/*  Free Software Foundation, Inc., 51 Franklin Street,             */
/*  Fifth Floor, Boston, MA  02110-1301, USA.                       */
/*                                                                  */
/*  Module: Seed7 Runtime Library                                   */
/*  File: seed7/src/chr_rtl.c                                       */
/*  Changes: 1992, 1993, 1994, 2005, 2010  Thomas Mertes            */
/*  Content: Primitive actions for the char type.                   */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "common.h"
#include "data_rtl.h"
#include "heaputl.h"
#include "striutl.h"
#include "int_rtl.h"
#include "str_rtl.h"
#include "rtl_err.h"

#undef EXTERN
#define EXTERN
#include "chr_rtl.h"



stritype chrCLit (chartype character)

  {
    memsizetype len;
    stritype result;

  /* chrCLit */
    /* printf("chrCLit(%lu)\n", character); */
    if (character < 127) {
      if (character < ' ') {
        len = strlen(cstri_escape_sequence[character]);
        if (!ALLOC_STRI_SIZE_OK(result, len + 2)) {
          raise_error(MEMORY_ERROR);
        } else {
          result->size = len + 2;
          result->mem[0] = '\'';
          cstri_expand(&result->mem[1],
              cstri_escape_sequence[character], len);
          result->mem[len + 1] = '\'';
        } /* if */
      } else if (character == '\\' || character == '\'') {
        if (!ALLOC_STRI_SIZE_OK(result, (memsizetype) 4)) {
          raise_error(MEMORY_ERROR);
        } else {
          result->size = 4;
          result->mem[0] = '\'';
          result->mem[1] = (strelemtype) '\\';
          result->mem[2] = (strelemtype) character;
          result->mem[3] = '\'';
        } /* if */
      } else {
        if (!ALLOC_STRI_SIZE_OK(result, (memsizetype) 3)) {
          raise_error(MEMORY_ERROR);
        } else {
          result->size = 3;
          result->mem[0] = '\'';
          result->mem[1] = (strelemtype) character;
          result->mem[2] = '\'';
        } /* if */
      } /* if */
    } else {
      result = intStr((inttype) character);
    } /* if */
    return result;
  } /* chrCLit */



/**
 *  Compare two characters.
 *  @return -1, 0 or 1 if the first argument is considered to be
 *          respectively less than, equal to, or greater than the
 *          second.
 */
inline inttype chrCmp (chartype char1, chartype char2)

  { /* chrCmp */
    if (char1 < char2) {
      return -1;
    } else if (char1 > char2) {
      return 1;
    } else {
      return 0;
    } /* if */
  } /* chrCmp */



/**
 *  Reinterpret the generic parameters as chartype and call chrCmp.
 *  Function pointers in C programs generated by the Seed7 compiler
 *  may point to this function. This assures correct behaviour even
 *  when sizeof(rtlGenerictype) != sizeof(chartype).
 */
inttype chrCmpGeneric (const rtlGenerictype value1, const rtlGenerictype value2)

  { /* chrCmpGeneric */
    return chrCmp(((const_rtlObjecttype *) &value1)->value.charvalue,
                  ((const_rtlObjecttype *) &value2)->value.charvalue);
  } /* chrCmpGeneric */



void chrCpy (chartype *dest, chartype source)

  { /* chrCpy */
    *dest = source;
  } /* chrCpy */



/**
 *  Convert a character to lower case.
 *  The conversion uses the default Unicode case mapping,
 *  where each character is considered in isolation.
 *  Characters without case mapping are left unchanged.
 *  The mapping is independend from the locale. Individual
 *  character case mappings cannot be reversed, because some
 *  characters have multiple characters that map to them.
 *  @return the character converted to lower case.
 */
chartype chrLow (chartype ch)

  { /* chrLow */
    toLower(&ch, 1, &ch);
    return ch;
  } /* chrLow */



/**
 *  Create a string with one character.
 *  @return a string with the character 'ch'.
 */
stritype chrStr (chartype ch)

  {
    stritype result;

  /* chrStr */
    if (!ALLOC_STRI_SIZE_OK(result, (memsizetype) 1)) {
      raise_error(MEMORY_ERROR);
      return NULL;
    } else {
      result->size = 1;
      result->mem[0] = (strelemtype) ch;
      return result;
    } /* if */
  } /* chrStr */



/**
 *  Convert a character to upper case.
 *  The conversion uses the default Unicode case mapping,
 *  where each character is considered in isolation.
 *  Characters without case mapping are left unchanged.
 *  The mapping is independend from the locale. Individual
 *  character case mappings cannot be reversed, because some
 *  characters have multiple characters that map to them.
 *  @return the character converted to upper case.
 */
chartype chrUp (chartype ch)

  { /* chrUp */
    toUpper(&ch, 1, &ch);
    return ch;
  } /* chrUp */
