/********************************************************************/
/*                                                                  */
/*  big_rtl.c     Functions for bigInteger without helping library. */
/*  Copyright (C) 1989 - 2009  Thomas Mertes                        */
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
/*  File: seed7/src/big_rtl.c                                       */
/*  Changes: 2005, 2006, 2008, 2009  Thomas Mertes                  */
/*  Content: Functions for bigInteger without helping library.      */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "common.h"
#include "heaputl.h"
#include "striutl.h"
#include "int_rtl.h"
#include "rtl_err.h"


#ifdef INT64TYPE
#define BIGDIGIT_SIZE 32
#else
#define BIGDIGIT_SIZE 16
#endif

#define KARATSUBA_THRESHOLD 32


/* Defines to describe a bigdigit:                                  */
/* BIGDIGIT_MASK              All bits in a bigdigit are set.       */
/* BIGDIGIT_SIGN              The highest bit of a bigdigit is set. */
/* POWER_OF_10_IN_BIGDIGIT    The biggest power of 10 which fits    */
/*                            in a bigdigit.                        */
/* DECIMAL_DIGITS_IN_BIGDIGIT The number of zero digits in          */
/*                            POWER_OF_10_IN_BIGDIGIT.              */


#if BIGDIGIT_SIZE == 8

typedef uint8type          bigdigittype;
typedef uint16type         doublebigdigittype;
#define BIGDIGIT_MASK                    0xFF
#define BIGDIGIT_SIGN                    0x80
#define BIGDIGIT_SIZE_MASK                0x7
#define BIGDIGIT_LOG2_SIZE                  3
#define POWER_OF_10_IN_BIGDIGIT           100
#define DECIMAL_DIGITS_IN_BIGDIGIT          2

#elif BIGDIGIT_SIZE == 16

typedef uint16type         bigdigittype;
typedef uint32type         doublebigdigittype;
#define BIGDIGIT_MASK                  0xFFFF
#define BIGDIGIT_SIGN                  0x8000
#define BIGDIGIT_SIZE_MASK                0xF
#define BIGDIGIT_LOG2_SIZE                  4
#define POWER_OF_10_IN_BIGDIGIT         10000
#define DECIMAL_DIGITS_IN_BIGDIGIT          4

#elif BIGDIGIT_SIZE == 32

typedef uint32type         bigdigittype;
typedef uint64type         doublebigdigittype;
#define BIGDIGIT_MASK              0xFFFFFFFF
#define BIGDIGIT_SIGN              0x80000000
#define BIGDIGIT_SIZE_MASK               0x1F
#define BIGDIGIT_LOG2_SIZE                  5
#define POWER_OF_10_IN_BIGDIGIT    1000000000
#define DECIMAL_DIGITS_IN_BIGDIGIT          9

#endif


#define IS_NEGATIVE(digit) ((digit) & BIGDIGIT_SIGN)


typedef struct rtlBigintstruct  *rtlBiginttype;
typedef const struct rtlBigintstruct  *const_rtlBiginttype;

typedef struct rtlBigintstruct {
    memsizetype size;
    bigdigittype bigdigits[1];
  } rtlBigintrecord;

SIZE_TYPE sizeof_bigdigittype = sizeof(bigdigittype);
SIZE_TYPE sizeof_rtlBigintrecord = sizeof(rtlBigintrecord);


#define SIZ_RTLBIG(len)     ((sizeof(rtlBigintrecord) - sizeof(bigdigittype)) + (len) * sizeof(bigdigittype))

#define ALLOC_BIG(var,len)       (ALLOC_HEAP(var, rtlBiginttype, SIZ_RTLBIG(len))?CNT1_BIG(len, SIZ_RTLBIG(len)), TRUE:FALSE)
#define FREE_BIG(var,len)        (CNT2_BIG(len, SIZ_RTLBIG(len)) FREE_HEAP(var, SIZ_RTLBIG(len)))
#define REALLOC_BIG(var,ln1,ln2) REALLOC_HEAP(var, rtlBiginttype, SIZ_RTLBIG(ln2))
#define COUNT3_BIG(len1,len2)    CNT3(CNT2_BIG(len1, SIZ_RTLBIG(len1)) CNT1_BIG(len2, SIZ_RTLBIG(len2)))



#ifdef ANSI_C

void bigGrow (rtlBiginttype *const big_variable, const const_rtlBiginttype big2);
inttype bigLowestSetBit (const const_rtlBiginttype big1);
rtlBiginttype bigLShift (const const_rtlBiginttype big1, const inttype lshift);
void bigLShiftAssign (rtlBiginttype *const big_variable, inttype lshift);
rtlBiginttype bigMinus (const const_rtlBiginttype big1);
rtlBiginttype bigRem (const const_rtlBiginttype big1, const const_rtlBiginttype big2);
void bigRShiftAssign (rtlBiginttype *const big_variable, inttype rshift);
rtlBiginttype bigSbtr (const const_rtlBiginttype big1, const const_rtlBiginttype big2);
void bigShrink (rtlBiginttype *const big_variable, const const_rtlBiginttype big2);

#else

void bigGrow ();
inttype bigLowestSetBit ();
rtlBiginttype bigLShift ();
void bigLShiftAssign ();
rtlBiginttype bigMinus ();
rtlBiginttype bigRem ();
void bigRShiftAssign ();
rtlBiginttype bigSbtr ();
void bigShrink ();

#endif



#ifdef ANSI_C

cstritype bigHexCStri (const_rtlBiginttype big1)
#else

cstritype bigHexCStri (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    memsizetype byteCount;
    cstritype buffer;
    cstritype result;

  /* bigHexCStri */
    if (big1 != NULL && big1->size > 0) {
      if (!ALLOC_CSTRI(result, big1->size * sizeof(bigdigittype) * 2 + 3)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        buffer = result;
        sprintf(buffer, "16#");
        buffer += 3;
        pos = big1->size - 1;
#if BIGDIGIT_SIZE == 8
        sprintf(buffer, "%02hhX", big1->bigdigits[pos]);
#elif BIGDIGIT_SIZE == 16
        sprintf(buffer, "%04hX", big1->bigdigits[pos]);
#elif BIGDIGIT_SIZE == 32
        sprintf(buffer, "%08lX", big1->bigdigits[pos]);
#endif
        if (IS_NEGATIVE(big1->bigdigits[pos])) {
          byteCount = sizeof(bigdigittype);
          while (byteCount > 1 && memcmp(buffer, "FF", 2) == 0 &&
            ((buffer[2] >= '8' && buffer[2] <= '9') ||
             (buffer[2] >= 'A' && buffer[2] <= 'F'))) {
            memmove(buffer, &buffer[2], strlen(&buffer[2]) + 1);
            byteCount--;
          } /* while */
        } else {
          byteCount = sizeof(bigdigittype);
          while (byteCount > 1 && memcmp(buffer, "00", 2) == 0 &&
            buffer[2] >= '0' && buffer[2] <= '7') {
            memmove(buffer, &buffer[2], strlen(&buffer[2]) + 1);
            byteCount--;
          } /* while */
        } /* if */
        buffer += strlen(buffer);
        while (pos > 0) {
          pos--;
#if BIGDIGIT_SIZE == 8
          sprintf(buffer, "%02hhX", big1->bigdigits[pos]);
#elif BIGDIGIT_SIZE == 16
          sprintf(buffer, "%04hX", big1->bigdigits[pos]);
#elif BIGDIGIT_SIZE == 32
          sprintf(buffer, "%08lX", big1->bigdigits[pos]);
#endif
          buffer += sizeof(bigdigittype) * 2;
        } /* while */
      } /* if */
    } else {
      if (big1 == NULL) {
        buffer = " *NULL_BIGINT* ";
      } else {
        buffer = " *ZERO_SIZE_BIGINT* ";
      } /* if */
      if (!ALLOC_CSTRI(result, strlen(buffer))) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        strcpy(result, buffer);
      } /* if */
    } /* if */
    return(result);
  } /* bigHexCStri */



/**
 *  Remove leading zero (or BIGDIGIT_MASK) digits from a
 *  signed big integer.
 */
#ifdef ANSI_C

static rtlBiginttype normalize (rtlBiginttype big1)
#else

static rtlBiginttype normalize (arg1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    bigdigittype digit;

  /* normalize */
    pos = big1->size;
    if (pos >= 2) {
      pos--;
      digit = big1->bigdigits[pos];
      if (digit == BIGDIGIT_MASK) {
        do {
          pos--;
          digit = big1->bigdigits[pos];
        } while (pos > 0 && digit == BIGDIGIT_MASK);
        if (!IS_NEGATIVE(digit)) {
          pos++;
        } /* if */
      } else if (digit == 0) {
        do {
          pos--;
          digit = big1->bigdigits[pos];
        } while (pos > 0 && digit == 0);
        if (IS_NEGATIVE(digit)) {
          pos++;
        } /* if */
      } /* if */
      pos++;
      if (big1->size != pos) {
        big1 = REALLOC_BIG(big1, big1->size, pos);
        if (big1 == NULL) {
          raise_error(MEMORY_ERROR);
        } else {
          COUNT3_BIG(big1->size, pos);
          big1->size = pos;
        } /* if */
      } /* if */
    } /* if */
    return(big1);
  } /* normalize */



#ifdef ANSI_C

static void negate_positive_big (const rtlBiginttype big1)
#else

static void negate_positive_big (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;

  /* negate_positive_big */
    pos = 0;
    do {
      carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (pos < big1->size);
  } /* negate_positive_big */



#ifdef ANSI_C

static void positive_copy_of_negative_big (const rtlBiginttype dest,
    const const_rtlBiginttype big1)
#else

static void positive_copy_of_negative_big (dest, big1)
rtlBiginttype dest;
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;

  /* positive_copy_of_negative_big */
    pos = 0;
    do {
      carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
      dest->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (pos < big1->size);
    if (IS_NEGATIVE(dest->bigdigits[pos - 1])) {
      dest->bigdigits[pos] = 0;
      pos++;
    } /* if */
    dest->size = pos;
  } /* positive_copy_of_negative_big */



#ifdef ANSI_C

static rtlBiginttype alloc_positive_copy_of_negative_big (const const_rtlBiginttype big1)
#else

static rtlBiginttype alloc_positive_copy_of_negative_big (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;
    rtlBiginttype result;

  /* alloc_positive_copy_of_negative_big */
    if (!ALLOC_BIG(result, big1->size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      pos = 0;
      do {
        carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
        result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        pos++;
      } while (pos < big1->size);
      result->size = pos;
      return(result);
    } /* if */
  } /* alloc_positive_copy_of_negative_big */



/**
 *  Multiplies big1 by 10 and adds carry to the result. This
 *  function works for unsigned big integers. It is assumed
 *  that big1 contains enough memory.
 */
#ifdef ANSI_C

static void uBigMultBy10AndAdd (const rtlBiginttype big1, doublebigdigittype carry)
#else

static void uBigMultBy10AndAdd (big1, carry)
rtlBiginttype big1;
doublebigdigittype carry;
#endif

  {
    memsizetype pos;

  /* uBigMultBy10AndAdd */
    pos = 0;
    do {
      carry += (doublebigdigittype) big1->bigdigits[pos] * 10;
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (pos < big1->size);
    if (carry != 0) {
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      big1->size++;
    } /* if */
  } /* uBigMultBy10AndAdd */



/**
 *  Divides the unsigned big integer big1 by POWER_OF_10_IN_BIGDIGIT
 *  and returns the remainder.
 */
#ifdef ANSI_C

static bigdigittype uBigDivideByPowerOf10 (const rtlBiginttype big1)
#else

static bigdigittype uBigDivideByPowerOf10 (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;

  /* uBigDivideByPowerOf10 */
    pos = big1->size;
    do {
      pos--;
      carry <<= 8 * sizeof(bigdigittype);
      carry += big1->bigdigits[pos];
      big1->bigdigits[pos] = (bigdigittype)
          ((carry / POWER_OF_10_IN_BIGDIGIT) & BIGDIGIT_MASK);
      carry %= POWER_OF_10_IN_BIGDIGIT;
    } while (pos > 0);
    return(carry & BIGDIGIT_MASK);
  } /* uBigDivideByPowerOf10 */



/**
 *  Shifts the big integer big1 to the left by lshift bits.
 *  Bits which are shifted out at the left of big1 are lost.
 *  At the right of big1 zero bits are shifted in. The function
 *  is called for 0 < rshift < 8 * sizeof(bigdigittype).
 */
#ifdef ANSI_C

static void uBigLShift (const rtlBiginttype big1, const unsigned int lshift)
#else

static void uBigLShift (big1, lshift)
rtlBiginttype big1;
unsigned int lshift;
#endif

  {
    doublebigdigittype carry = 0;
    memsizetype pos;

  /* uBigLShift */
    pos = 0;
    do {
      carry |= ((doublebigdigittype) big1->bigdigits[pos]) << lshift;
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (pos < big1->size);
  } /* uBigLShift */



#ifdef OUT_OF_ORDER
/**
 *  Shifts the big integer big1 to the left by lshift bits.
 *  Bits which are shifted out at the left of big1 are lost.
 *  At the right of big1 zero bits are shifted in. The function
 *  is called for 0 < rshift < 8 * sizeof(bigdigittype).
 */
#ifdef ANSI_C

static void uBigLShift (const rtlBiginttype big1, const unsigned int lshift)
#else

static void uBigLShift (big1, lshift)
rtlBiginttype big1;
unsigned int lshift;
#endif

  {
    unsigned int rshift = 8 * sizeof(bigdigittype) - lshift;
    bigdigittype low_digit;
    bigdigittype high_digit;
    memsizetype pos;

  /* uBigLShift */
    low_digit = big1->bigdigits[0];
    big1->bigdigits[0] = (bigdigittype) ((low_digit << lshift) & BIGDIGIT_MASK);
    for (pos = 1; pos < big1->size; pos++) {
      high_digit = big1->bigdigits[pos];
      big1->bigdigits[pos] = (bigdigittype)
          (((low_digit >> rshift) | (high_digit << lshift)) & BIGDIGIT_MASK);
      low_digit = high_digit;
    } /* for */
  } /* uBigLShift */
#endif



#ifdef OUT_OF_ORDER
/**
 *  Shifts the big integer big1 to the right by rshift bits.
 *  Bits which are shifted out at the right of big1 are lost.
 *  At the left of big1 zero bits are shifted in. The function
 *  is called for 0 < rshift < 8 * sizeof(bigdigittype).
 */
#ifdef ANSI_C

static void uBigRShift (const rtlBiginttype big1, const unsigned int rshift)
#else

static void uBigRShift (big1, rshift)
rtlBiginttype big1;
unsigned int rshift;
#endif

  {
    unsigned int lshift = 8 * sizeof(bigdigittype) - rshift;
    doublebigdigittype carry = 0;
    memsizetype pos;

  /* uBigRShift */
    pos = big1->size;
    do {
      carry <<= 8 * sizeof(bigdigittype);
      carry |= ((doublebigdigittype) big1->bigdigits[pos]) << lshift;
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      pos--;
    } while (pos > 0);
  } /* uBigRShift */
#endif



/**
 *  Shifts the big integer big1 to the right by rshift bits.
 *  Bits which are shifted out at the right of big1 are lost.
 *  At the left of big1 zero bits are shifted in. The function
 *  is called for 0 < rshift < 8 * sizeof(bigdigittype).
 */
#ifdef ANSI_C

static void uBigRShift (const rtlBiginttype big1, const unsigned int rshift)
#else

static void uBigRShift (big1, rshift)
rtlBiginttype big1;
unsigned int rshift;
#endif

  {
    unsigned int lshift = 8 * sizeof(bigdigittype) - rshift;
    bigdigittype low_digit;
    bigdigittype high_digit;
    memsizetype pos;

  /* uBigRShift */
    high_digit = 0;
    for (pos = big1->size - 1; pos != 0; pos--) {
      low_digit = big1->bigdigits[pos];
      big1->bigdigits[pos] = (bigdigittype)
          (((low_digit >> rshift) | (high_digit << lshift)) & BIGDIGIT_MASK);
      high_digit = low_digit;
    } /* for */
    low_digit = big1->bigdigits[0];
    big1->bigdigits[0] = (bigdigittype)
        (((low_digit >> rshift) | (high_digit << lshift)) & BIGDIGIT_MASK);
  } /* uBigRShift */



/**
 *  Increments an unsigned big integer by 1. This function does
 *  overflow silently when big1 contains not enough digits.
 */
#ifdef ANSI_C

static void uBigIncr (const rtlBiginttype big1)
#else

static void uBigIncr (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;

  /* uBigIncr */
    pos = 0;
    do {
      carry += big1->bigdigits[pos];
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (carry != 0 && pos < big1->size);
  } /* uBigIncr */



/**
 *  Decrements an unsigned big integer by 1. This function does
 *  overflow silently when big1 contains not enough digits.
 */
#ifdef ANSI_C

static void uBigDecr (const rtlBiginttype big1)
#else

static void uBigDecr (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;

  /* uBigDecr */
    pos = 0;
    do {
      carry += (doublebigdigittype) big1->bigdigits[pos] + BIGDIGIT_MASK;
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (carry == 0 && pos < big1->size);
  } /* uBigDecr */



/**
 *  Computes an integer division of big1 by one digit for
 *  nonnegative big integers. The digit must not be zero.
 */
#ifdef ANSI_C

static void uBigDiv1 (const const_rtlBiginttype big1, const bigdigittype digit,
    const rtlBiginttype result)
#else

static void uBigDiv1 (big1, digit, result)
rtlBiginttype big1;
bigdigittype digit;
rtlBiginttype result;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;

  /* uBigDiv1 */
    pos = big1->size;
    do {
      pos--;
      carry <<= 8 * sizeof(bigdigittype);
      carry += big1->bigdigits[pos];
      result->bigdigits[pos] = (bigdigittype) ((carry / digit) & BIGDIGIT_MASK);
      carry %= digit;
    } while (pos > 0);
  } /* uBigDiv1 */



/**
 *  Computes an integer division of big1 by one digit for
 *  signed big integers. The memory for the result is
 *  requested and the normalized result is returned. This
 *  function handles also the special case of a division by
 *  zero.
 */
#ifdef ANSI_C

static rtlBiginttype bigDiv1 (const_rtlBiginttype big1, bigdigittype digit)
#else

static rtlBiginttype bigDiv1 (big1, digit)
rtlBiginttype big1;
bigdigittype digit;
#endif

  {
    booltype negative = FALSE;
    rtlBiginttype big1_help = NULL;
    rtlBiginttype result;

  /* bigDiv1 */
    if (digit == 0) {
      raise_error(NUMERIC_ERROR);
      return(NULL);
    } else {
      if (!ALLOC_BIG(result, big1->size + 1)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        result->size = big1->size + 1;
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          negative = TRUE;
          big1_help = alloc_positive_copy_of_negative_big(big1);
          big1 = big1_help;
          if (big1_help == NULL) {
            FREE_BIG(result, result->size);
            return(NULL);
          } /* if */
        } /* if */
        result->bigdigits[result->size - 1] = 0;
        if (IS_NEGATIVE(digit)) {
          negative = !negative;
          digit = -digit;
        } /* if */
        uBigDiv1(big1, digit, result);
        if (negative) {
          negate_positive_big(result);
        } /* if */
        result = normalize(result);
        if (big1_help != NULL) {
          FREE_BIG(big1_help, big1_help->size);
        } /* if */
        return(result);
      } /* if */
    } /* if */
  } /* bigDiv1 */



/**
 *  Computes an integer division of big1 by big2 for signed big
 *  integers when big1 has less digits than big2. The memory for
 *  the result is requested and the normalized result is returned.
 *  Normally big1->size < big2->size implies abs(big1) < abs(big2).
 *  When abs(big1) < abs(big2) holds the result is 0. The cases
 *  when big1->size < big2->size and abs(big1) = abs(big2) are if
 *  big1->size + 1 == big2->size and big1 = 0x8000 (0x80000000...)
 *  and big2 = 0x00008000 (0x000080000000...). In this cases the
 *  result is -1. In all other cases the result is 0.
 */
#ifdef ANSI_C

static rtlBiginttype bigDivSizeLess (const const_rtlBiginttype big1,
    const const_rtlBiginttype big2)
#else

static rtlBiginttype bigDivSizeLess (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    memsizetype pos;
    rtlBiginttype result;

  /* bigDivSizeLess */
    if (!ALLOC_BIG(result, 1)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = 1;
      if (big1->size + 1 == big2->size &&
          big1->bigdigits[big1->size - 1] == BIGDIGIT_SIGN &&
          big2->bigdigits[big2->size - 1] == 0 &&
          big2->bigdigits[big2->size - 2] == BIGDIGIT_SIGN) {
        result->bigdigits[0] = BIGDIGIT_MASK;
        for (pos = 0; pos < big1->size - 1; pos++) {
          if (big1->bigdigits[pos] != 0 || big2->bigdigits[pos] != 0) {
            result->bigdigits[0] = 0;
          } /* if */
        } /* for */
      } else {
        result->bigdigits[0] = 0;
      } /* if */
      return(result);
    } /* if */
  } /* bigDivSizeLess */



/**
 *  Multiplies big2 with multiplier and subtracts the result from
 *  big1 at the digit position pos1 of big1. Big1, big2 and
 *  multiplier are nonnegative big integer values.
 *  The algorithm tries to save computations. Therefore
 *  there are checks for mult_carry != 0 and sbtr_carry == 0.
 */
#ifdef ANSI_C

static bigdigittype uBigMultSub (const rtlBiginttype big1, const const_rtlBiginttype big2,
    const bigdigittype multiplier, const memsizetype pos1)
#else

static bigdigittype uBigMultSub (big1, big2, multiplier, pos1)
rtlBiginttype big1;
rtlBiginttype big2;
bigdigittype multiplier;
memsizetype pos1;
#endif

  {
    memsizetype pos;
    doublebigdigittype mult_carry = 0;
    doublebigdigittype sbtr_carry = 1;

  /* uBigMultSub */
    pos = 0;
    do {
      mult_carry += (doublebigdigittype) big2->bigdigits[pos] * multiplier;
      sbtr_carry += big1->bigdigits[pos1 + pos] + (~mult_carry & BIGDIGIT_MASK);
      big1->bigdigits[pos1 + pos] = (bigdigittype) (sbtr_carry & BIGDIGIT_MASK);
      mult_carry >>= 8 * sizeof(bigdigittype);
      sbtr_carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (pos < big2->size);
    for (pos += pos1; mult_carry != 0 && pos < big1->size; pos++) {
      sbtr_carry += big1->bigdigits[pos] + (~mult_carry & BIGDIGIT_MASK);
      big1->bigdigits[pos] = (bigdigittype) (sbtr_carry & BIGDIGIT_MASK);
      mult_carry >>= 8 * sizeof(bigdigittype);
      sbtr_carry >>= 8 * sizeof(bigdigittype);
    } /* for */
    for (; sbtr_carry == 0 && pos < big1->size; pos++) {
      sbtr_carry = (doublebigdigittype) big1->bigdigits[pos] + BIGDIGIT_MASK;
      big1->bigdigits[pos] = (bigdigittype) (sbtr_carry & BIGDIGIT_MASK);
      sbtr_carry >>= 8 * sizeof(bigdigittype);
    } /* for */
    return(sbtr_carry & BIGDIGIT_MASK);
  } /* uBigMultSub */



/**
 *  Adds big2 to big1 at the digit position pos1. Big1 and big2
 *  are nonnegative big integer values. The size of big1 must be
 *  greater or equal the size of big2. The final carry is ignored.
 */
#ifdef ANSI_C

static void uBigAddTo (const rtlBiginttype big1, const const_rtlBiginttype big2,
    const memsizetype pos1)
#else

static void uBigAddTo (big1, big2, pos1)
rtlBiginttype big1;
rtlBiginttype big2;
memsizetype pos1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;

  /* uBigAddTo */
    pos = 0;
    do {
      carry += (doublebigdigittype) big1->bigdigits[pos1 + pos] + big2->bigdigits[pos];
      big1->bigdigits[pos1 + pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (pos < big2->size);
    for (pos += pos1; pos < big1->size; pos++) {
      carry += big1->bigdigits[pos];
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
    } /* for */
  } /* uBigAddTo */



/**
 *  Computes an integer division of big1 by big2 for nonnegative big
 *  integers. The remainder is delivered in big1. There are several
 *  preconditions. Big2 must have at least 2 digits and big1 must have
 *  at least one digit more than big2. If big1 and big2 have the same
 *  length in digits nothing is done. The most significant bit of big2
 *  must be set. The most significant digit of big1 must be less than
 *  the most significant digit of big2. The computations to meet this
 *  predonditions are done outside this function. The special cases
 *  with a one digit big2 or a big1 with less digits than big2 are
 *  handled in other functions. This algorithm based on the algorithm
 *  from D.E. Knuth described in "The art of computer programming"
 *  volume 2 (Seminumerical algorithms).
 */
#ifdef ANSI_C

static void uBigDiv (const rtlBiginttype big1, const const_rtlBiginttype big2,
    const rtlBiginttype result)
#else

static void uBigDiv (big1, big2, result)
rtlBiginttype big1;
rtlBiginttype big2;
rtlBiginttype result;
#endif

  {
    memsizetype pos1;
    doublebigdigittype twodigits;
    doublebigdigittype remainder;
    bigdigittype quotientdigit;
    bigdigittype sbtr_carry;

  /* uBigDiv */
    for (pos1 = big1->size - 1; pos1 >= big2->size; pos1--) {
      twodigits = (((doublebigdigittype) big1->bigdigits[pos1]) <<
          8 * sizeof(bigdigittype)) | big1->bigdigits[pos1 - 1];
      if (big1->bigdigits[pos1] == big2->bigdigits[big2->size - 1]) {
        quotientdigit = BIGDIGIT_MASK;
      } else {
        quotientdigit = twodigits / big2->bigdigits[big2->size - 1];
      } /* if */
      remainder = twodigits - (doublebigdigittype) quotientdigit *
          big2->bigdigits[big2->size - 1];
      while (remainder <= BIGDIGIT_MASK &&
          (doublebigdigittype) big2->bigdigits[big2->size - 2] * quotientdigit >
          (remainder << 8 * sizeof(bigdigittype) | big1->bigdigits[pos1 - 2])) {
        quotientdigit--;
        remainder = twodigits - (doublebigdigittype) quotientdigit *
            big2->bigdigits[big2->size - 1];
      } /* while */
      sbtr_carry = uBigMultSub(big1, big2, quotientdigit, pos1 - big2->size);
      if (sbtr_carry == 0) {
        uBigAddTo(big1, big2, pos1 - big2->size);
        quotientdigit--;
      } /* if */
      result->bigdigits[pos1 - big2->size] = quotientdigit;
    } /* for */
  } /* uBigDiv */



/**
 *  Computes the remainder of a integer division of big1 by
 *  one digit for nonnegative big integers. The digit must
 *  not be zero.
 */
#ifdef ANSI_C

static bigdigittype uBigRem1 (const const_rtlBiginttype big1, const bigdigittype digit)
#else

static bigdigittype uBigRem1 (big1, digit)
rtlBiginttype big1;
bigdigittype digit;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;

  /* uBigRem1 */
    pos = big1->size;
    do {
      pos--;
      carry <<= 8 * sizeof(bigdigittype);
      carry += big1->bigdigits[pos];
      carry %= digit;
    } while (pos > 0);
    return(carry);
  } /* uBigRem1 */



/**
 *  Computes the remainder of the integer division big1 by one digit
 *  for signed big integers. The memory for the remainder is requested
 *  and the normalized remainder is returned. This function handles also
 *  the special case of a division by zero.
 */
#ifdef ANSI_C

static rtlBiginttype bigRem1 (const_rtlBiginttype big1, bigdigittype digit)
#else

static rtlBiginttype bigRem1 (big1, digit)
rtlBiginttype big1;
bigdigittype digit;
#endif

  {
    booltype negative = FALSE;
    rtlBiginttype big1_help = NULL;
    rtlBiginttype remainder;

  /* bigRem1 */
    if (digit == 0) {
      raise_error(NUMERIC_ERROR);
      return(NULL);
    } else {
      if (!ALLOC_BIG(remainder, 1)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        remainder->size = 1;
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          negative = TRUE;
          big1_help = alloc_positive_copy_of_negative_big(big1);
          big1 = big1_help;
          if (big1_help == NULL) {
            FREE_BIG(remainder, remainder->size);
            return(NULL);
          } /* if */
        } /* if */
        if (IS_NEGATIVE(digit)) {
          digit = -digit;
        } /* if */
        remainder->bigdigits[0] = uBigRem1(big1, digit);
        if (negative) {
          negate_positive_big(remainder);
        } /* if */
        if (big1_help != NULL) {
          FREE_BIG(big1_help, big1_help->size);
        } /* if */
        return(remainder);
      } /* if */
    } /* if */
  } /* bigRem1 */



/**
 *  Computes an integer division of big1 by one digit for
 *  nonnegative big integers. The digit must not be zero.
 *  The remainder of the division is returned.
 */
#ifdef ANSI_C

static bigdigittype uBigMDiv1 (const const_rtlBiginttype big1,
    const bigdigittype digit, const rtlBiginttype result)
#else

static bigdigittype uBigMDiv1 (big1, digit, result)
rtlBiginttype big1;
bigdigittype digit;
rtlBiginttype result;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;

  /* uBigMDiv1 */
    pos = big1->size;
    do {
      pos--;
      carry <<= 8 * sizeof(bigdigittype);
      carry += big1->bigdigits[pos];
      result->bigdigits[pos] = (bigdigittype) ((carry / digit) & BIGDIGIT_MASK);
      carry %= digit;
    } while (pos > 0);
    return(carry);
  } /* uBigMDiv1 */



/**
 *  Computes an integer modulo division of big1 by one digit
 *  for signed big integers. The memory for the result is
 *  requested and the normalized result is returned. This
 *  function handles also the special case of a division by
 *  zero.
 */
#ifdef ANSI_C

static rtlBiginttype bigMDiv1 (const_rtlBiginttype big1, bigdigittype digit)
#else

static rtlBiginttype bigMDiv1 (big1, digit)
rtlBiginttype big1;
bigdigittype digit;
#endif

  {
    booltype negative = FALSE;
    rtlBiginttype big1_help = NULL;
    bigdigittype remainder;
    rtlBiginttype result;

  /* bigMDiv1 */
    if (digit == 0) {
      raise_error(NUMERIC_ERROR);
      return(NULL);
    } else {
      if (!ALLOC_BIG(result, big1->size + 1)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        result->size = big1->size + 1;
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          negative = TRUE;
          big1_help = alloc_positive_copy_of_negative_big(big1);
          big1 = big1_help;
          if (big1_help == NULL) {
            FREE_BIG(result, result->size);
            return(NULL);
          } /* if */
        } /* if */
        result->bigdigits[result->size - 1] = 0;
        if (IS_NEGATIVE(digit)) {
          negative = !negative;
          digit = -digit;
        } /* if */
        remainder = uBigMDiv1(big1, digit, result);
        if (negative) {
          if (remainder != 0) {
            uBigIncr(result);
          } /* if */
          negate_positive_big(result);
        } /* if */
        result = normalize(result);
        if (big1_help != NULL) {
          FREE_BIG(big1_help, big1_help->size);
        } /* if */
        return(result);
      } /* if */
    } /* if */
  } /* bigMDiv1 */



/**
 *  Computes a modulo integer division of big1 by big2 for signed
 *  big integers when big1 has less digits than big2. The memory for
 *  the result is requested and the normalized result is returned.
 *  Normally big1->size < big2->size implies abs(big1) < abs(big2).
 *  When abs(big1) < abs(big2) holds the result is 0 or -1. The cases
 *  when big1->size < big2->size and abs(big1) = abs(big2) are if
 *  big1->size + 1 == big2->size and big1 = 0x8000 (0x80000000...)
 *  and big2 = 0x00008000 (0x000080000000...). In this cases the
 *  result is -1. In the cases when the result is 0 or -1 the
 *  following check is done: When big1 and big2 have different signs
 *  the result is -1 otherwise the result is 0.
 */
#ifdef ANSI_C

static rtlBiginttype bigMDivSizeLess (const const_rtlBiginttype big1,
    const const_rtlBiginttype big2)
#else

static rtlBiginttype bigMDivSizeLess (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    memsizetype pos;
    rtlBiginttype result;

  /* bigMDivSizeLess */
    if (!ALLOC_BIG(result, 1)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = 1;
      if (big1->size + 1 == big2->size &&
          big1->bigdigits[big1->size - 1] == BIGDIGIT_SIGN &&
          big2->bigdigits[big2->size - 1] == 0 &&
          big2->bigdigits[big2->size - 2] == BIGDIGIT_SIGN) {
        result->bigdigits[0] = BIGDIGIT_MASK;
        for (pos = 0; pos < big1->size - 1; pos++) {
          if (big1->bigdigits[pos] != 0 || big2->bigdigits[pos] != 0) {
            result->bigdigits[0] = 0;
          } /* if */
        } /* for */
      } else {
        result->bigdigits[0] = 0;
      } /* if */
      if (result->bigdigits[0] == 0 &&
          IS_NEGATIVE(big1->bigdigits[big1->size - 1]) !=
          IS_NEGATIVE(big2->bigdigits[big2->size - 1])) {
        result->bigdigits[0] = BIGDIGIT_MASK;
      } /* if */
      return(result);
    } /* if */
  } /* bigMDivSizeLess */



/**
 *  Computes the modulo of the integer division big1 by one digit
 *  for signed big integers. The memory for the modulo is requested
 *  and the normalized modulo is returned. This function handles also
 *  the special case of a division by zero.
 */
#ifdef ANSI_C

static rtlBiginttype bigMod1 (const const_rtlBiginttype big1, const bigdigittype digit)
#else

static rtlBiginttype bigMod1 (big1, digit)
rtlBiginttype big1;
bigdigittype digit;
#endif

  {
    rtlBiginttype modulo;

  /* bigMod1 */
    modulo = bigRem1(big1, digit);
    if (IS_NEGATIVE(big1->bigdigits[big1->size - 1]) != IS_NEGATIVE(digit) &&
        modulo != NULL && modulo->bigdigits[0] != 0) {
      modulo->bigdigits[0] += digit;
    } /* if */
    return(modulo);
  } /* bigMod1 */



/**
 *  Computes the remainder of the integer division big1 by big2 for
 *  signed big integers when big1 has less digits than big2. The memory
 *  for the remainder is requested and the normalized remainder is returned.
 *  Normally big1->size < big2->size implies abs(big1) < abs(big2).
 *  When abs(big1) < abs(big2) holds the remainder is big1. The cases
 *  when big1->size < big2->size and abs(big1) = abs(big2) are if
 *  big1->size + 1 == big2->size and big1 = 0x8000 (0x80000000...)
 *  and big2 = 0x00008000 (0x000080000000...). In this cases the
 *  remainder is 0. In all other cases the remainder is big1.
 */
#ifdef ANSI_C

static rtlBiginttype bigRemSizeLess (const const_rtlBiginttype big1,
    const const_rtlBiginttype big2)
#else

static rtlBiginttype bigRemSizeLess (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    memsizetype pos;
    booltype remainderIs0;
    rtlBiginttype remainder;

  /* bigRemSizeLess */
    if (big1->size + 1 == big2->size &&
        big1->bigdigits[big1->size - 1] == BIGDIGIT_SIGN &&
        big2->bigdigits[big2->size - 1] == 0 &&
        big2->bigdigits[big2->size - 2] == BIGDIGIT_SIGN) {
      remainderIs0 = TRUE;
      for (pos = 0; pos < big1->size - 1; pos++) {
        if (big1->bigdigits[pos] != 0 || big2->bigdigits[pos] != 0) {
          remainderIs0 = FALSE;
        } /* if */
      } /* for */
    } else {
      remainderIs0 = FALSE;
    } /* if */
    if (remainderIs0) {
      if (!ALLOC_BIG(remainder, 1)) {
        raise_error(MEMORY_ERROR);
      } else {
        remainder->size = 1;
        remainder->bigdigits[0] = 0;
      } /* if */
    } else {
      if (!ALLOC_BIG(remainder, big1->size)) {
        raise_error(MEMORY_ERROR);
      } else {
        remainder->size = big1->size;
        memcpy(remainder->bigdigits, big1->bigdigits,
            (SIZE_TYPE) big1->size * sizeof(bigdigittype));
      } /* if */
    } /* if */
    return(remainder);
  } /* bigRemSizeLess */



/**
 *  Adds big2 to big1 at the digit position pos1. Big1 and big2
 *  are signed big integer values. The size of big1 must be
 *  greater or equal the size of big2.
 */
#ifdef ANSI_C

static void bigAddTo (const rtlBiginttype big1, const const_rtlBiginttype big2)
#else

static void bigAddTo (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;
    doublebigdigittype big2_sign;

  /* bigAddTo */
    pos = 0;
    do {
      carry += (doublebigdigittype) big1->bigdigits[pos] + big2->bigdigits[pos];
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (pos < big2->size);
    big2_sign = IS_NEGATIVE(big2->bigdigits[pos - 1]) ? BIGDIGIT_MASK : 0;
    for (; pos < big1->size; pos++) {
      carry += big1->bigdigits[pos] + big2_sign;
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
    } /* for */
  } /* bigAddTo */



/**
 *  Computes the modulo of the integer division big1 by big2 for
 *  signed big integers when big1 has less digits than big2. The memory
 *  for the modulo is requested and the normalized modulo is returned.
 *  Normally big1->size < big2->size implies abs(big1) < abs(big2).
 *  When abs(big1) < abs(big2) holds the division gives 0. The cases
 *  when big1->size < big2->size and abs(big1) = abs(big2) are if
 *  big1->size + 1 == big2->size and big1 = 0x8000 (0x80000000...)
 *  and big2 = 0x00008000 (0x000080000000...). In this cases the
 *  modulo is 0. In all other cases the modulo is big1 or big1 +
 *  big2 when big1 and big2 have different signs.
 */
#ifdef ANSI_C

static rtlBiginttype bigModSizeLess (const const_rtlBiginttype big1,
    const const_rtlBiginttype big2)
#else

static rtlBiginttype bigModSizeLess (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    memsizetype pos;
    booltype moduloIs0;
    rtlBiginttype modulo;

  /* bigModSizeLess */
    if (big1->size + 1 == big2->size &&
        big1->bigdigits[big1->size - 1] == BIGDIGIT_SIGN &&
        big2->bigdigits[big2->size - 1] == 0 &&
        big2->bigdigits[big2->size - 2] == BIGDIGIT_SIGN) {
      moduloIs0 = TRUE;
      for (pos = 0; pos < big1->size - 1; pos++) {
        if (big1->bigdigits[pos] != 0 || big2->bigdigits[pos] != 0) {
          moduloIs0 = FALSE;
        } /* if */
      } /* for */
    } else {
      moduloIs0 = FALSE;
    } /* if */
    if (moduloIs0) {
      if (!ALLOC_BIG(modulo, 1)) {
        raise_error(MEMORY_ERROR);
      } else {
        modulo->size = 1;
        modulo->bigdigits[0] = 0;
      } /* if */
    } else {
      if (IS_NEGATIVE(big1->bigdigits[big1->size - 1]) !=
          IS_NEGATIVE(big2->bigdigits[big2->size - 1])) {
        if (!ALLOC_BIG(modulo, big2->size)) {
          raise_error(MEMORY_ERROR);
        } else {
          modulo->size = big2->size;
          memcpy(modulo->bigdigits, big2->bigdigits,
              (SIZE_TYPE) big2->size * sizeof(bigdigittype));
          bigAddTo(modulo, big1);
          modulo = normalize(modulo);
        } /* if */
      } else {
        if (!ALLOC_BIG(modulo, big1->size)) {
          raise_error(MEMORY_ERROR);
        } else {
          modulo->size = big1->size;
          memcpy(modulo->bigdigits, big1->bigdigits,
              (SIZE_TYPE) big1->size * sizeof(bigdigittype));
        } /* if */
      } /* if */
    } /* if */
    return(modulo);
  } /* bigModSizeLess */



/**
 *  Computes the remainder of an integer division of big1 by big2
 *  for nonnegative big integers. The remainder is delivered in
 *  big1. There are several preconditions for this function. Big2
 *  must have at least 2 digits and big1 must have at least one
 *  digit more than big2. If big1 and big2 have the same length in
 *  digits nothing is done. The most significant bit of big2 must be
 *  set. The most significant digit of big1 must be less than the
 *  most significant digit of big2. The computations to meet this
 *  predonditions are done outside this function. The special cases
 *  with a one digit big2 or a big1 with less digits than big2 are
 *  handled in other functions. This algorithm based on the algorithm
 *  from D.E. Knuth described in "The art of computer programming"
 *  volume 2 (Seminumerical algorithms).
 */
#ifdef ANSI_C

static void uBigRem (const rtlBiginttype big1, const const_rtlBiginttype big2)
#else

static void uBigRem (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    memsizetype pos1;
    doublebigdigittype twodigits;
    doublebigdigittype remainder;
    bigdigittype quotientdigit;
    bigdigittype sbtr_carry;

  /* uBigRem */
    for (pos1 = big1->size - 1; pos1 >= big2->size; pos1--) {
      twodigits = (((doublebigdigittype) big1->bigdigits[pos1]) <<
          8 * sizeof(bigdigittype)) | big1->bigdigits[pos1 - 1];
      if (big1->bigdigits[pos1] == big2->bigdigits[big2->size - 1]) {
        quotientdigit = BIGDIGIT_MASK;
      } else {
        quotientdigit = twodigits / big2->bigdigits[big2->size - 1];
      } /* if */
      remainder = twodigits - (doublebigdigittype) quotientdigit *
          big2->bigdigits[big2->size - 1];
      while (remainder <= BIGDIGIT_MASK &&
          (doublebigdigittype) big2->bigdigits[big2->size - 2] * quotientdigit >
          (remainder << 8 * sizeof(bigdigittype) | big1->bigdigits[pos1 - 2])) {
        quotientdigit--;
        remainder = twodigits - (doublebigdigittype) quotientdigit *
            big2->bigdigits[big2->size - 1];
      } /* while */
      sbtr_carry = uBigMultSub(big1, big2, quotientdigit, pos1 - big2->size);
      if (sbtr_carry == 0) {
        uBigAddTo(big1, big2, pos1 - big2->size);
        quotientdigit--;
      } /* if */
    } /* for */
  } /* uBigRem */



/**
 *  Returns the sqare of the nonnegative big integer big1. The result
 *  is written into big_help (which is a scratch variable and is
 *  assumed to contain enough memory). Before returning the result
 *  the variable big1 is assigned to big_help. This way it is possible
 *  to square a given base with base = uBigSqare(base, &big_help).
 *  Note that the old base is in the scratch variable big_help
 *  afterwards. This squaring algorithm takes into account that
 *  digit1 * digit2 + digit2 * digit1 == (digit1 * digit2) << 1.
 *  This reduces the number of multiplications approx. by factor 2.
 *  Unfortunately one bit more than sizeof(doublebigdigittype) is
 *  needed to store the shifted product. Therefore extra effort is
 *  necessary to avoid an overflow.
 */
#ifdef ANSI_C

static rtlBiginttype uBigSqare (const rtlBiginttype big1, rtlBiginttype *big_help)
#else

static rtlBiginttype uBigSqare (big1, big_help)
rtlBiginttype big1;
rtlBiginttype *big_help;
#endif

  {
    memsizetype pos1;
    memsizetype pos2;
    doublebigdigittype carry = 0;
    doublebigdigittype product;
    bigdigittype digit;
    rtlBiginttype result;

  /* uBigSqare */
    result = *big_help;
    digit = big1->bigdigits[0];
    carry = (doublebigdigittype) digit * digit;
    result->bigdigits[0] = (bigdigittype) (carry & BIGDIGIT_MASK);
    carry >>= 8 * sizeof(bigdigittype);
    for (pos2 = 1; pos2 < big1->size; pos2++) {
      product = (doublebigdigittype) digit * big1->bigdigits[pos2];
      carry += (product << 1) & BIGDIGIT_MASK;
      result->bigdigits[pos2] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      carry += product >> (8 * sizeof(bigdigittype) - 1);
    } /* for */
    result->bigdigits[pos2] = (bigdigittype) (carry & BIGDIGIT_MASK);
    for (pos1 = 1; pos1 < big1->size; pos1++) {
      digit = big1->bigdigits[pos1];
      carry = (doublebigdigittype) result->bigdigits[pos1 + pos1] +
          (doublebigdigittype) digit * digit;
      result->bigdigits[pos1 + pos1] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      for (pos2 = pos1 + 1; pos2 < big1->size; pos2++) {
        product = (doublebigdigittype) digit * big1->bigdigits[pos2];
        carry += (doublebigdigittype) result->bigdigits[pos1 + pos2] +
            ((product << 1) & BIGDIGIT_MASK);
        result->bigdigits[pos1 + pos2] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        carry += product >> (8 * sizeof(bigdigittype) - 1);
      } /* for */
      result->bigdigits[pos1 + pos2] = (bigdigittype) (carry & BIGDIGIT_MASK);
    } /* for */
    pos1 = big1->size + big1->size;
    pos1--;
    digit = result->bigdigits[pos1];
    if (digit == 0) {
      do {
        pos1--;
        digit = result->bigdigits[pos1];
      } while (pos1 > 0 && digit == 0);
      if (IS_NEGATIVE(digit)) {
        pos1++;
      } /* if */
    } /* if */
    pos1++;
    result->size = pos1;
    *big_help = big1;
    return(result);
  } /* uBigSqare */



#ifdef ANSI_C

static void uBigDigitAdd (const bigdigittype *const big1, const memsizetype size1,
    const bigdigittype *const big2, const memsizetype size2, bigdigittype *const result)
#else

static void uBigDigitAdd (big1, size1, big2, size2, result)
bigdigittype *big1;
memsizetype size1;
bigdigittype *big2;
memsizetype size2;
bigdigittype *result;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;

  /* uBigDigitAdd */
    pos = 0;
    do {
      carry += (doublebigdigittype) big1[pos] + big2[pos];
      result[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (pos < size2);
    for (; pos < size1; pos++) {
      carry += big1[pos];
      result[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
    } /* for */
    result[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
  } /* uBigDigitAdd */



#ifdef ANSI_C

static void uBigDigitSbtrFrom (bigdigittype *const big1, const memsizetype size1,
    const bigdigittype *const big2, const memsizetype size2)
#else

static void uBigDigitSbtrFrom (big1, size1, big2, size2)
bigdigittype *big1;
memsizetype size1;
bigdigittype *big2;
memsizetype size2;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;

  /* uBigDigitSbtrFrom */
    pos = 0;
    do {
      carry += (doublebigdigittype) big1[pos] +
          (~big2[pos] & BIGDIGIT_MASK);
      big1[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (pos < size2);
    for (; carry == 0 && pos < size1; pos++) {
      carry = (doublebigdigittype) big1[pos] + BIGDIGIT_MASK;
      big1[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
    } /* for */
  } /* uBigDigitSbtrFrom */



#ifdef ANSI_C

static void uBigDigitAddTo (bigdigittype *const big1,  const memsizetype size1,
    const bigdigittype *const big2, const memsizetype size2)
#else

static void uBigDigitAddTo (bigdigittype *big1,  memsizetype size1,
    bigdigittype *big2, memsizetype size2)
bigdigittype *big1;
memsizetype size1;
bigdigittype *big2;
memsizetype size2;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;

  /* uBigDigitAddTo */
    pos = 0;
    do {
      carry += (doublebigdigittype) big1[pos] + big2[pos];
      big1[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (pos < size2);
    for (; carry != 0 && pos < size1; pos++) {
      carry += big1[pos];
      big1[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
    } /* for */
  } /* uBigDigitAddTo */



#ifdef ANSI_C

static void uBigDigitMult (const bigdigittype *const big1,
    const bigdigittype *const big2, const memsizetype size,
    bigdigittype *const result)
#else

static void uBigDigitMult (big1, big2, size, result)
bigdigittype *big1;
bigdigittype *big2;
memsizetype size;
bigdigittype *result;
#endif

  {
    memsizetype pos1;
    memsizetype pos2;
    doublebigdigittype carry;
    doublebigdigittype carry2 = 0;
    doublebigdigittype prod;

  /* uBigDigitMult */
    carry = (doublebigdigittype) big1[0] * big2[0];
    result[0] = (bigdigittype) (carry & BIGDIGIT_MASK);
    carry >>= 8 * sizeof(bigdigittype);
    for (pos1 = 1; pos1 < size; pos1++) {
      pos2 = 0;
      do {
        prod = (doublebigdigittype) big1[pos2] * big2[pos1 - pos2];
        carry2 += carry > (doublebigdigittype) ~prod ? 1 : 0;
        carry += prod;
        pos2++;
      } while (pos2 <= pos1);
      result[pos1] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      carry |= (carry2 & BIGDIGIT_MASK) << 8 * sizeof(bigdigittype);
      carry2 >>= 8 * sizeof(bigdigittype);
    } /* for */
    for (; pos1 < size + size - 1; pos1++) {
      pos2 = pos1 - size + 1;
      do {
        prod = (doublebigdigittype) big1[pos2] * big2[pos1 - pos2];
        carry2 += carry > (doublebigdigittype) ~prod ? 1 : 0;
        carry += prod;
        pos2++;
      } while (pos2 < size);
      result[pos1] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      carry |= (carry2 & BIGDIGIT_MASK) << 8 * sizeof(bigdigittype);
      carry2 >>= 8 * sizeof(bigdigittype);
    } /* for */
    result[pos1] = (bigdigittype) (carry & BIGDIGIT_MASK);
  } /* uBigDigitMult */



#ifdef ANSI_C

static void uBigKaratsubaMult (const bigdigittype *const big1,
    const bigdigittype *const big2, const memsizetype size,
    bigdigittype *const result, bigdigittype *const temp)
#else

static void uBigKaratsubaMult (big1, big2, size, result, temp)
bigdigittype *big1;
bigdigittype *big2;
memsizetype size;
bigdigittype *result;
bigdigittype *temp;
#endif

  {
    memsizetype sizeLo;
    memsizetype sizeHi;

  /* uBigKaratsubaMult */
    if (size < KARATSUBA_THRESHOLD) {
      uBigDigitMult(big1, big2, size, result);
    } else {
      sizeHi = size >> 1;
      sizeLo = size - sizeHi;
      uBigDigitAdd(big1, sizeLo, &big1[sizeLo], sizeHi, result);
      uBigDigitAdd(big2, sizeLo, &big2[sizeLo], sizeHi, &result[size]);
      uBigKaratsubaMult(result, &result[size], sizeLo + 1, temp, &temp[(sizeLo + 1) << 1]);
      uBigKaratsubaMult(big1, big2, sizeLo, result, &temp[(sizeLo + 1) << 1]);
      uBigKaratsubaMult(&big1[sizeLo], &big2[sizeLo], sizeHi, &result[sizeLo << 1], &temp[(sizeLo + 1) << 1]);
      uBigDigitSbtrFrom(temp, (sizeLo + 1) << 1, result, sizeLo << 1);
      uBigDigitSbtrFrom(temp, (sizeLo + 1) << 1, &result[sizeLo << 1], sizeHi << 1);
      uBigDigitAddTo(&result[sizeLo], sizeLo + (sizeHi << 1), temp, (sizeLo + 1) << 1);
    } /* if */
  } /* uBigKaratsubaMult */



#ifdef OUT_OF_ORDER
#ifdef ANSI_C

static void uBigMultPositiveWithDigit (const const_rtlBiginttype big1, bigdigittype bigDigit,
    const rtlBiginttype result)
#else

static void uBigMultPositiveWithDigit (big1, bigDigit, result)
rtlBiginttype big1;
bigdigittype bigDigit;
rtlBiginttype result;
#endif

  {
    memsizetype pos;
    doublebigdigittype mult_carry;

  /* uBigMultPositiveWithDigit */
    mult_carry = (doublebigdigittype) big1->bigdigits[0] * bigDigit;
    result->bigdigits[0] = (bigdigittype) (mult_carry & BIGDIGIT_MASK);
    mult_carry >>= 8 * sizeof(bigdigittype);
    for (pos = 1; pos < big1->size; pos++) {
      mult_carry += (doublebigdigittype) big1->bigdigits[pos] * bigDigit;
      result->bigdigits[pos] = (bigdigittype) (mult_carry & BIGDIGIT_MASK);
      mult_carry >>= 8 * sizeof(bigdigittype);
    } /* for */
    result->bigdigits[pos] = (bigdigittype) (mult_carry & BIGDIGIT_MASK);
  } /* uBigMultPositiveWithDigit */



#ifdef ANSI_C

static void uBigMultPositiveWithNegatedDigit (const const_rtlBiginttype big1, bigdigittype bigDigit,
    const rtlBiginttype result)
#else

static void uBigMultPositiveWithNegatedDigit (big1, bigDigit, result)
rtlBiginttype big1;
bigdigittype bigDigit;
rtlBiginttype result;
#endif

  {
    memsizetype pos;
    doublebigdigittype mult_carry;
    doublebigdigittype result_carry = 1;

  /* uBigMultPositiveWithNegatedDigit */
    mult_carry = (doublebigdigittype) big1->bigdigits[0] * bigDigit;
    result_carry += ~(mult_carry & BIGDIGIT_MASK);
    result->bigdigits[0] = (bigdigittype) (result_carry & BIGDIGIT_MASK);
    mult_carry >>= 8 * sizeof(bigdigittype);
    result_carry >>= 8 * sizeof(bigdigittype);
    for (pos = 1; pos < big1->size; pos++) {
      mult_carry += (doublebigdigittype) big1->bigdigits[pos] * bigDigit;
      result_carry += ~(mult_carry & BIGDIGIT_MASK);
      result->bigdigits[pos] = (bigdigittype) (result_carry & BIGDIGIT_MASK);
      mult_carry >>= 8 * sizeof(bigdigittype);
      result_carry >>= 8 * sizeof(bigdigittype);
    } /* for */
    result_carry += ~(mult_carry & BIGDIGIT_MASK);
    result->bigdigits[pos] = (bigdigittype) (result_carry & BIGDIGIT_MASK);
  } /* uBigMultPositiveWithNegatedDigit */



#ifdef ANSI_C

static void uBigMultNegativeWithDigit (const const_rtlBiginttype big1, bigdigittype bigDigit,
    const rtlBiginttype result)
#else

static void uBigMultNegativeWithDigit (big1, bigDigit, result)
rtlBiginttype big1;
bigdigittype bigDigit;
rtlBiginttype result;
#endif

  {
    memsizetype pos;
    doublebigdigittype negate_carry = 1;
    doublebigdigittype mult_carry;
    doublebigdigittype result_carry = 1;

  /* uBigMultNegativeWithDigit */
    negate_carry += ~big1->bigdigits[0] & BIGDIGIT_MASK;
    mult_carry = (negate_carry & BIGDIGIT_MASK) * bigDigit;
    result_carry += ~(mult_carry & BIGDIGIT_MASK);
    result->bigdigits[0] = (bigdigittype) (result_carry & BIGDIGIT_MASK);
    negate_carry >>= 8 * sizeof(bigdigittype);
    mult_carry >>= 8 * sizeof(bigdigittype);
    result_carry >>= 8 * sizeof(bigdigittype);
    for (pos = 1; pos < big1->size; pos++) {
      negate_carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
      mult_carry += (negate_carry & BIGDIGIT_MASK) * bigDigit;
      result_carry += ~(mult_carry & BIGDIGIT_MASK);
      result->bigdigits[pos] = (bigdigittype) (result_carry & BIGDIGIT_MASK);
      negate_carry >>= 8 * sizeof(bigdigittype);
      mult_carry >>= 8 * sizeof(bigdigittype);
      result_carry >>= 8 * sizeof(bigdigittype);
    } /* for */
    result_carry += ~(mult_carry & BIGDIGIT_MASK);
    result->bigdigits[pos] = (bigdigittype) (result_carry & BIGDIGIT_MASK);
  } /* uBigMultNegativeWithDigit */



#ifdef ANSI_C

static void uBigMultNegativeWithNegatedDigit (const const_rtlBiginttype big1, bigdigittype bigDigit,
    const rtlBiginttype result)
#else

static void uBigMultNegativeWithNegatedDigit (big1, bigDigit, result)
rtlBiginttype big1;
bigdigittype bigDigit;
rtlBiginttype result;
#endif

  {
    memsizetype pos;
    doublebigdigittype negate_carry = 1;
    doublebigdigittype mult_carry;

  /* uBigMultNegativeWithNegatedDigit */
    negate_carry += ~big1->bigdigits[0] & BIGDIGIT_MASK;
    mult_carry = (negate_carry & BIGDIGIT_MASK) * bigDigit;
    result->bigdigits[0] = (bigdigittype) (mult_carry & BIGDIGIT_MASK);
    negate_carry >>= 8 * sizeof(bigdigittype);
    mult_carry >>= 8 * sizeof(bigdigittype);
    for (pos = 1; pos < big1->size; pos++) {
      negate_carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
      mult_carry += (negate_carry & BIGDIGIT_MASK) * bigDigit;
      result->bigdigits[pos] = (bigdigittype) (mult_carry & BIGDIGIT_MASK);
      negate_carry >>= 8 * sizeof(bigdigittype);
      mult_carry >>= 8 * sizeof(bigdigittype);
    } /* for */
    result->bigdigits[pos] = (bigdigittype) (mult_carry & BIGDIGIT_MASK);
  } /* uBigMultNegativeWithNegatedDigit */
#endif



#ifdef ANSI_C

static void uBigMult (const_rtlBiginttype big1, const_rtlBiginttype big2,
    const rtlBiginttype result)
#else

static void uBigMult (big1, big2, result)
rtlBiginttype big1;
rtlBiginttype big2;
rtlBiginttype result;
#endif

  {
    const_rtlBiginttype help_big;
    memsizetype pos1;
    memsizetype pos2;
    doublebigdigittype carry;
    doublebigdigittype carry2 = 0;
    doublebigdigittype prod;

  /* uBigMult */
    if (big2->size > big1->size) {
      help_big = big1;
      big1 = big2;
      big2 = help_big;
    } /* if */
    carry = (doublebigdigittype) big1->bigdigits[0] * big2->bigdigits[0];
    result->bigdigits[0] = (bigdigittype) (carry & BIGDIGIT_MASK);
    carry >>= 8 * sizeof(bigdigittype);
    if (big2->size == 1) {
      for (pos1 = 1; pos1 < big1->size; pos1++) {
        carry += (doublebigdigittype) big1->bigdigits[pos1] * big2->bigdigits[0];
        result->bigdigits[pos1] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
      } /* for */
    } else {
      for (pos1 = 1; pos1 < big2->size; pos1++) {
        pos2 = 0;
        do {
          prod = (doublebigdigittype) big1->bigdigits[pos2] * big2->bigdigits[pos1 - pos2];
          carry2 += carry > (doublebigdigittype) ~prod ? 1 : 0;
          carry += prod;
          pos2++;
        } while (pos2 <= pos1);
        result->bigdigits[pos1] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        carry |= (carry2 & BIGDIGIT_MASK) << 8 * sizeof(bigdigittype);
        carry2 >>= 8 * sizeof(bigdigittype);
      } /* for */
      for (; pos1 < big1->size; pos1++) {
        pos2 = pos1 - big2->size + 1;
        do {
          prod = (doublebigdigittype) big1->bigdigits[pos2] * big2->bigdigits[pos1 - pos2];
          carry2 += carry > (doublebigdigittype) ~prod ? 1 : 0;
          carry += prod;
          pos2++;
        } while (pos2 <= pos1);
        result->bigdigits[pos1] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        carry |= (carry2 & BIGDIGIT_MASK) << 8 * sizeof(bigdigittype);
        carry2 >>= 8 * sizeof(bigdigittype);
      } /* for */
      for (; pos1 < big1->size + big2->size - 1; pos1++) {
        pos2 = pos1 - big2->size + 1;
        do {
          prod = (doublebigdigittype) big1->bigdigits[pos2] * big2->bigdigits[pos1 - pos2];
          carry2 += carry > (doublebigdigittype) ~prod ? 1 : 0;
          carry += prod;
          pos2++;
        } while (pos2 < big1->size);
        result->bigdigits[pos1] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        carry |= (carry2 & BIGDIGIT_MASK) << 8 * sizeof(bigdigittype);
        carry2 >>= 8 * sizeof(bigdigittype);
      } /* for */
    } /* if */
    result->bigdigits[pos1] = (bigdigittype) (carry & BIGDIGIT_MASK);
  } /* uBigMult */



#ifdef OUT_OF_ORDER
#ifdef ANSI_C

static void uBigMult (const const_rtlBiginttype big1, const const_rtlBiginttype big2,
    const rtlBiginttype result)
#else

static void uBigMult (big1, big2, result)
rtlBiginttype big1;
rtlBiginttype big2;
rtlBiginttype result;
#endif

  {
    memsizetype pos1;
    memsizetype pos2;
    doublebigdigittype carry;
    doublebigdigittype carry2 = 0;
    doublebigdigittype prod;

  /* uBigMult */
    carry = (doublebigdigittype) big1->bigdigits[0] * big2->bigdigits[0];
    result->bigdigits[0] = (bigdigittype) (carry & BIGDIGIT_MASK);
    carry >>= 8 * sizeof(bigdigittype);
    for (pos1 = 1; pos1 < big1->size + big2->size - 1; pos1++) {
      if (pos1 < big2->size) {
        pos2 = 0;
      } else {
        pos2 = pos1 - big2->size + 1;
      } /* if */
      if (pos1 < big1->size) {
        pos3 = pos1 + 1;
      } else {
        pos3 = big1->size;
      } /* if */
      do {
        prod = (doublebigdigittype) big1->bigdigits[pos2] * big2->bigdigits[pos1 - pos2];
        /* To avoid overflows of carry + prod it is necessary     */
        /* to check if carry + prod > DOUBLEBIGDIGIT_MASK which   */
        /* is equivalent to carry > DOUBLEBIGDIGIT_MASK - prod.   */
        /* A subtraction can be replaced by adding the negated    */
        /* value: carry > DOUBLEBIGDIGIT_MASK + ~prod + 1. This   */
        /* can be simplified to carry > ~prod.                    */
        carry2 += carry > ~prod ? 1 : 0;
        carry += prod;
        pos2++;
      } while (pos2 < pos3);
      result->bigdigits[pos1] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      carry |= (carry2 & BIGDIGIT_MASK) << 8 * sizeof(bigdigittype);
      carry2 >>= 8 * sizeof(bigdigittype);
    } /* for */
    result->bigdigits[pos1] = (bigdigittype) (carry & BIGDIGIT_MASK);
  } /* uBigMult */
#endif



#ifdef OUT_OF_ORDER
#ifdef ANSI_C

static void uBigMult (const const_rtlBiginttype big1, const const_rtlBiginttype big2,
    const rtlBiginttype result)
#else

static void uBigMult (big1, big2, result)
rtlBiginttype big1;
rtlBiginttype big2;
rtlBiginttype result;
#endif

  {
    memsizetype pos1;
    memsizetype pos2;
    doublebigdigittype carry = 0;

  /* uBigMult */
    pos2 = 0;
    do {
      carry += (doublebigdigittype) big1->bigdigits[0] * big2->bigdigits[pos2];
      result->bigdigits[pos2] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos2++;
    } while (pos2 < big2->size);
    result->bigdigits[big2->size] = (bigdigittype) (carry & BIGDIGIT_MASK);
    for (pos1 = 1; pos1 < big1->size; pos1++) {
      carry = 0;
      pos2 = 0;
      do {
        carry += (doublebigdigittype) result->bigdigits[pos1 + pos2] +
            (doublebigdigittype) big1->bigdigits[pos1] * big2->bigdigits[pos2];
        result->bigdigits[pos1 + pos2] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        pos2++;
      } while (pos2 < big2->size);
      result->bigdigits[pos1 + big2->size] = (bigdigittype) (carry & BIGDIGIT_MASK);
    } /* for */
  } /* uBigMult */
#endif



#ifdef ANSI_C

static rtlBiginttype uBigMultK (const_rtlBiginttype big1, const_rtlBiginttype big2,
    const booltype negative)
#else

static rtlBiginttype uBigMultK (big1, big2, negative)
rtlBiginttype big1;
rtlBiginttype big2;
booltype negative;
#endif

  {
    const_rtlBiginttype help_big;
    rtlBiginttype big2_help;
    rtlBiginttype temp;
    rtlBiginttype result;

  /* uBigMultK */
    if (big1->size >= KARATSUBA_THRESHOLD && big2->size >= KARATSUBA_THRESHOLD) {
      if (big2->size > big1->size) {
        help_big = big1;
        big1 = big2;
        big2 = help_big;
      } /* if */
      if (big2->size << 1 <= big1->size) {
        if (!ALLOC_BIG(big2_help, big1->size - (big1->size >> 1))) {
          raise_error(MEMORY_ERROR);
          return(NULL);
        } else {
          big2_help->size = big1->size - (big1->size >> 1);
          memcpy(big2_help->bigdigits, big2->bigdigits, 
              (SIZE_TYPE) big2->size * sizeof(bigdigittype));
          memset(&big2_help->bigdigits[big2->size], 0,
              (SIZE_TYPE) (big2_help->size - big2->size) * sizeof(bigdigittype));
          big2 = big2_help;
          if (!ALLOC_BIG(result, (big1->size >> 1) + (big2->size << 1))) {
            raise_error(MEMORY_ERROR);
          } else {
            result->size = (big1->size >> 1) + (big2->size << 1);
            if (!ALLOC_BIG(temp, big1->size << 2)) {
              raise_error(MEMORY_ERROR);
            } else {
              uBigKaratsubaMult(big1->bigdigits, big2->bigdigits,
                  big1->size >> 1, result->bigdigits, temp->bigdigits);
              uBigKaratsubaMult(&big1->bigdigits[big1->size >> 1], big2->bigdigits,
                  big2->size, temp->bigdigits, &temp->bigdigits[big2->size << 1]);
              memset(&result->bigdigits[(big1->size >> 1) << 1], 0,
                  (SIZE_TYPE) (result->size - ((big1->size >> 1) << 1)) * sizeof(bigdigittype));
              uBigDigitAddTo(&result->bigdigits[big1->size >> 1], result->size - (big1->size >> 1),
                  temp->bigdigits, big2->size << 1);
              if (negative) {
                negate_positive_big(result);
              } /* if */
              result = normalize(result);
              FREE_BIG(temp, big1->size << 2);
            } /* if */
          } /* if */
          FREE_BIG(big2_help, big1->size - (big1->size >> 1));
        } /* if */
      } else {
        if (!ALLOC_BIG(big2_help, big1->size)) {
          raise_error(MEMORY_ERROR);
          return(NULL);
        } else {
          big2_help->size = big1->size;
          memcpy(big2_help->bigdigits, big2->bigdigits, 
              (SIZE_TYPE) big2->size * sizeof(bigdigittype));
          memset(&big2_help->bigdigits[big2->size], 0,
              (SIZE_TYPE) (big2_help->size - big2->size) * sizeof(bigdigittype));
          big2 = big2_help;
          if (!ALLOC_BIG(result, big1->size << 1)) {
            raise_error(MEMORY_ERROR);
          } else {
            if (!ALLOC_BIG(temp, big1->size << 2)) {
              raise_error(MEMORY_ERROR);
            } else {
              uBigKaratsubaMult(big1->bigdigits, big2->bigdigits,
                  big1->size, result->bigdigits, temp->bigdigits);
              result->size = big1->size << 1;
              if (negative) {
                negate_positive_big(result);
              } /* if */
              result = normalize(result);
              FREE_BIG(temp, big1->size << 2);
            } /* if */
          } /* if */
          FREE_BIG(big2_help, big1->size);
        } /* if */
      } /* if */
    } else {
      if (!ALLOC_BIG(result, big1->size + big2->size)) {
        raise_error(MEMORY_ERROR);
      } else {
        result->size = big1->size + big2->size;
        uBigMult(big1, big2, result);
        if (negative) {
          negate_positive_big(result);
        } /* if */
        result = normalize(result);
      } /* if */
    } /* if */
    return(result);
  } /* uBigMultK */



/**
 *  Returns the product of big1 by big2 for nonnegative big integers.
 *  The result is written into big_help (which is a scratch variable
 *  and is assumed to contain enough memory). Before returning the
 *  result the variable big1 is assigned to big_help. This way it is
 *  possible to write number = uBigMultIntoHelp(number, base, &big_help).
 *  Note that the old number is in the scratch variable big_help
 *  afterwards.
 */
#ifdef ANSI_C

static rtlBiginttype uBigMultIntoHelp (const rtlBiginttype big1,
    const const_rtlBiginttype big2, rtlBiginttype *const big_help)
#else

static rtlBiginttype uBigMultIntoHelp (big1, big2, big_help)
rtlBiginttype big1;
rtlBiginttype big2;
rtlBiginttype *big_help;
#endif

  {
    memsizetype pos1;
    bigdigittype digit;
    rtlBiginttype result;

  /* uBigMultIntoHelp */
    result = *big_help;
    uBigMult(big1, big2, result);
    pos1 = big1->size + big2->size;
    pos1--;
    digit = result->bigdigits[pos1];
    if (digit == 0) {
      do {
        pos1--;
        digit = result->bigdigits[pos1];
      } while (pos1 > 0 && digit == 0);
      if (IS_NEGATIVE(digit)) {
        pos1++;
      } /* if */
    } /* if */
    pos1++;
    result->size = pos1;
    *big_help = big1;
    return(result);
  } /* uBigMultIntoHelp */



#ifdef ANSI_C

static int uBigIsNot0 (const const_rtlBiginttype big)
#else

static int uBigIsNot0 (big)
rtlBiginttype big;
#endif

  {
    memsizetype pos;

  /* uBigIsNot0 */
    pos = 0;
    do {
      if (big->bigdigits[pos] != 0) {
        return(TRUE);
      } /* if */
      pos++;
    } while (pos < big->size);
    return(FALSE);
  } /* uBigIsNot0 */



/**
 *  Returns the absolute value of a signed big integer.
 */
#ifdef ANSI_C

rtlBiginttype bigAbs (const const_rtlBiginttype big1)
#else

rtlBiginttype bigAbs (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;
    rtlBiginttype result;

  /* bigAbs */
    if (!ALLOC_BIG(result, big1->size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = big1->size;
      if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
        pos = 0;
        do {
          carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
          result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
          pos++;
        } while (pos < big1->size);
        if (IS_NEGATIVE(result->bigdigits[pos - 1])) {
          result->size++;
          result = REALLOC_BIG(result, big1->size, result->size);
          if (result == NULL) {
            raise_error(MEMORY_ERROR);
            return(NULL);
          } else {
            COUNT3_BIG(big1->size, result->size);
            result->bigdigits[big1->size] = 0;
          } /* if */
        } /* if */
      } else {
        memcpy(result->bigdigits, big1->bigdigits,
            (SIZE_TYPE) big1->size * sizeof(bigdigittype));
      } /* if */
    } /* if */
    return(result);
  } /* bigAbs */



/**
 *  Returns the sum of two signed big integers.
 *  The function sorts the two values by size. This way there is a
 *  loop up to the shorter size and a second loop up to the longer
 *  size.
 */
#ifdef ANSI_C

rtlBiginttype bigAdd (const_rtlBiginttype big1, const_rtlBiginttype big2)
#else

rtlBiginttype bigAdd (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    const_rtlBiginttype help_big;
    memsizetype pos;
    doublebigdigittype carry = 0;
    doublebigdigittype big2_sign;
    rtlBiginttype result;

  /* bigAdd */
    if (big2->size > big1->size) {
      help_big = big1;
      big1 = big2;
      big2 = help_big;
    } /* if */
    if (!ALLOC_BIG(result, big1->size + 1)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      pos = 0;
      do {
        carry += (doublebigdigittype) big1->bigdigits[pos] + big2->bigdigits[pos];
        result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        pos++;
      } while (pos < big2->size);
      big2_sign = IS_NEGATIVE(big2->bigdigits[pos - 1]) ? BIGDIGIT_MASK : 0;
      for (; pos < big1->size; pos++) {
        carry += big1->bigdigits[pos] + big2_sign;
        result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
      } /* for */
      if (IS_NEGATIVE(big1->bigdigits[pos - 1])) {
        big2_sign--;
      } /* if */
      result->bigdigits[pos] = (bigdigittype) ((carry + big2_sign) & BIGDIGIT_MASK);
      result->size = pos + 1;
      result = normalize(result);
      return(result);
    } /* if */
  } /* bigAdd */



/**
 *  Returns the sum of two signed big integers.
 *  Big1 is assumed to be a temporary value which is reused.
 */
#ifdef ANSI_C

rtlBiginttype bigAddTemp (rtlBiginttype big1, const_rtlBiginttype big2)
#else

rtlBiginttype bigAddTemp  (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  { /* bigAddTemp */
    bigGrow(&big1, big2);
    return(big1);
  } /* bigAddTemp */



#ifdef OUT_OF_ORDER
#ifdef ANSI_C

rtlBiginttype bigBinom (rtlBiginttype n_number, rtlBiginttype k_number)
#else

rtlBiginttype bigBinom (n_number, k_number)
rtlBiginttype n_number;
rtlBiginttype k_number;
#endif

  {
    rtlBiginttype number;
    rtlBiginttype result;

  /* bigBinom */
    if (2 * k_number > n_number) {
      k_number = n_number - k_number;
    } /* if */
    if (k_number < 0) {
      result = 0;
    } else if (k_number == 0) {
      result = 1;
    } else {
      result = n_number;
      for (number = 2; number <= k_number; number++) {
        result *= (n_number - number + 1);
        result /= number;
      } /* for */
    } /* if */
    return((inttype) result);
  } /* bigBinom */
#endif



#ifdef ANSI_C

inttype bigBitLength (const const_rtlBiginttype big1)
#else

inttype bigBitLength (big1)
rtlBiginttype big1;
#endif

  {
    inttype result;

  /* bigBitLength */
    result = (big1->size - 1) * 8 * sizeof(bigdigittype);
    if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
      result += most_significant_bit(~big1->bigdigits[big1->size - 1]) + 1;
    } else {
      result += most_significant_bit(big1->bigdigits[big1->size - 1]) + 1;
    } /* if */
    return(result);
  } /* bigBitLength */



#ifdef ANSI_C

stritype bigCLit (const const_rtlBiginttype big1)
#else

stritype bigCLit (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    int byteNum;
    char byteBuffer[22];
    bigdigittype digit;
    memsizetype byteDigitCount;
    memsizetype charIndex;
    memsizetype result_size;
    stritype result;

  /* bigCLit */
    byteDigitCount = big1->size * sizeof(bigdigittype);
    digit = big1->bigdigits[big1->size - 1];
    byteNum = sizeof(bigdigittype) - 1;
    if (IS_NEGATIVE(digit)) {
      while (byteNum > 0 && (digit >> byteNum * 8 & 0xFF) == 0xFF) {
        byteDigitCount--;
        byteNum--;
      } /* while */
      if (byteNum < 3 && (digit >> byteNum * 8 & 0xFF) <= 127) {
        byteDigitCount++;
        byteNum++;
      } /* if */
    } else {
      while (byteNum > 0 && (digit >> byteNum * 8 & 0xFF) == 0) {
        byteDigitCount--;
        byteNum--;
      } /* while */
      if (byteNum < 3 && (digit >> byteNum * 8 & 0xFF) >= 128) {
        byteDigitCount++;
        byteNum++;
      } /* if */
    } /* if */
    result_size = byteDigitCount * 5 + 21;
    if (!ALLOC_STRI(result, result_size)) {
      raise_error(MEMORY_ERROR);
    } else {
      result->size = result_size;
      sprintf(byteBuffer, "{0x%02lX,0x%02lX,0x%02lX,0x%02lX,",
          (byteDigitCount >> 24) & 0xFF, (byteDigitCount >> 16) & 0xFF,
          (byteDigitCount >>  8) & 0xFF,  byteDigitCount        & 0xFF);
      cstri_expand(result->mem, byteBuffer, 21);
      charIndex = 21;
      pos = big1->size;
      while (pos > 0) {
        pos--;
        digit = big1->bigdigits[pos];
#if BIGDIGIT_SIZE == 8
        sprintf(byteBuffer, "0x%02hhX,",
            digit &       0xFF);
#elif BIGDIGIT_SIZE == 16
        sprintf(byteBuffer, "0x%02X,0x%02X,",
            digit >>  8 & 0xFF, digit       & 0xFF);
#elif BIGDIGIT_SIZE == 32
        sprintf(byteBuffer, "0x%02lX,0x%02lX,0x%02lX,0x%02lX,",
            digit >> 24 & 0xFF, digit >> 16 & 0xFF,
            digit >>  8 & 0xFF, digit       & 0xFF);
#endif
        if ((pos + 1) * sizeof(bigdigittype) <= byteDigitCount) {
          cstri_expand(&result->mem[charIndex], byteBuffer, 5 * sizeof(bigdigittype));
          charIndex += 5 * sizeof(bigdigittype);
        } else {
          byteNum = byteDigitCount - pos * sizeof(bigdigittype);
          cstri_expand(&result->mem[charIndex],
              &byteBuffer[5 * (sizeof(bigdigittype) - byteNum)], 5 * byteNum);
          charIndex += 5 * byteNum;
        } /* if */
      } /* while */
      charIndex -= 5;
      result->mem[charIndex + 4] = '}';
    } /* if */
    return(result);
  } /* bigCLit */



#ifdef ANSI_C

inttype bigCmp (const const_rtlBiginttype big1, const const_rtlBiginttype big2)
#else

inttype bigCmp (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    bigdigittype big1_negative;
    bigdigittype big2_negative;
    memsizetype pos;

  /* bigCmp */
    big1_negative = IS_NEGATIVE(big1->bigdigits[big1->size - 1]);
    big2_negative = IS_NEGATIVE(big2->bigdigits[big2->size - 1]);
    if (big1_negative != big2_negative) {
      return(big1_negative ? -1 : 1);
    } else if (big1->size != big2->size) {
      return((big1->size < big2->size) != (big1_negative != 0) ? -1 : 1);
    } else {
      pos = big1->size;
      while (pos > 0) {
        pos--;
        if (big1->bigdigits[pos] != big2->bigdigits[pos]) {
          return(big1->bigdigits[pos] < big2->bigdigits[pos] ? -1 : 1);
        } /* if */
      } /* while */
      return(0);
    } /* if */
  } /* bigCmp */



#ifdef ANSI_C

inttype bigCmpSignedDigit (const const_rtlBiginttype big1, inttype number)
#else

inttype bigCmpSignedDigit (big1, number)
rtlBiginttype big1;
inttype number;
#endif

  {
    inttype result;

  /* bigCmpSignedDigit */
    if (number < 0) {
      if (!IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
        result = 1;
      } else if (big1->size != 1) {
        result = -1;
      } else if (big1->bigdigits[0] != (bigdigittype) number) {
        if (big1->bigdigits[0] < (bigdigittype) number) {
          result = -1;
        } else {
          result = 1;
        } /* if */
      } else {
        result = 0;
      } /* if */
    } else {
      if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
        result = -1;
      } else if (big1->size != 1) {
        result = 1;
      } else if (big1->bigdigits[0] != (bigdigittype) number) {
        if (big1->bigdigits[0] < (bigdigittype) number) {
          result = -1;
        } else {
          result = 1;
        } /* if */
      } else {
        result = 0;
      } /* if */
    } /* if */
    return(result);
  } /* bigCmpSignedDigit */



#ifdef ANSI_C

void bigCpy (rtlBiginttype *const big_to, const const_rtlBiginttype big_from)
#else

void bigCpy (big_to, big_from)
rtlBiginttype *big_to;
rtlBiginttype big_from;
#endif

  {
    memsizetype new_size;
    rtlBiginttype big_dest;

  /* bigCpy */
    big_dest = *big_to;
    new_size = big_from->size;
    if (big_dest->size != new_size) {
      if (!ALLOC_BIG(big_dest, new_size)) {
        raise_error(MEMORY_ERROR);
        return;
      } else {
        FREE_BIG(*big_to, (*big_to)->size);
        big_dest->size = new_size;
        *big_to = big_dest;
      } /* if */
    } /* if */
    memcpy(big_dest->bigdigits, big_from->bigdigits,
        (SIZE_TYPE) new_size * sizeof(bigdigittype));
  } /* bigCpy */



#ifdef ANSI_C

rtlBiginttype bigCreate (const const_rtlBiginttype big_from)
#else

rtlBiginttype bigCreate (big_from)
rtlBiginttype big_from;
#endif

  {
    memsizetype new_size;
    rtlBiginttype result;

  /* bigCreate */
    new_size = big_from->size;
    if (!ALLOC_BIG(result, new_size)) {
      raise_error(MEMORY_ERROR);
    } else {
      result->size = new_size;
      memcpy(result->bigdigits, big_from->bigdigits,
          (SIZE_TYPE) new_size * sizeof(bigdigittype));
    } /* if */
    return(result);
  } /* bigCreate */



#ifdef ANSI_C

void bigDecr (rtlBiginttype *const big_variable)
#else

void bigDecr (big_variable)
rtlBiginttype *big_variable;
#endif

  {
    rtlBiginttype big1;
    memsizetype pos;
    doublebigdigittype carry = 0;
    bigdigittype negative;

  /* bigDecr */
    big1 = *big_variable;
    negative = IS_NEGATIVE(big1->bigdigits[big1->size - 1]);
    pos = 0;
    do {
      carry += (doublebigdigittype) big1->bigdigits[pos] + BIGDIGIT_MASK;
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (carry == 0 && pos < big1->size);
    pos = big1->size;
    if (!IS_NEGATIVE(big1->bigdigits[pos - 1])) {
      if (negative) {
        big1 = REALLOC_BIG(big1, pos, pos + 1);
        if (big1 == NULL) {
          raise_error(MEMORY_ERROR);
        } else {
          COUNT3_BIG(pos, pos + 1);
          big1->size++;
          big1->bigdigits[pos] = BIGDIGIT_MASK;
          *big_variable = big1;
        } /* if */
      } else if (big1->bigdigits[pos - 1] == 0 &&
          pos >= 2 && !IS_NEGATIVE(big1->bigdigits[pos - 2])) {
        big1 = REALLOC_BIG(big1, pos, pos - 1);
        if (big1 == NULL) {
          raise_error(MEMORY_ERROR);
        } else {
          COUNT3_BIG(pos, pos - 1);
          big1->size--;
          *big_variable = big1;
        } /* if */
      } /* if */
    } /* if */
  } /* bigDecr */



#ifdef ANSI_C

void bigDestr (const rtlBiginttype old_bigint)
#else

void bigDestr (old_bigint)
rtlBiginttype old_bigint;
#endif

  { /* bigDestr */
    if (old_bigint != NULL) {
      FREE_BIG(old_bigint, old_bigint->size);
    } /* if */
  } /* bigDestr */



/**
 *  Computes an integer division of big1 by big2 for signed big
 *  integers. The memory for the result is requested and the
 *  normalized result is returned. When big2 has just one digit
 *  or when big1 has less digits than big2 the bigDiv1() or
 *  bigDivSizeLess() functions are called. In the general case
 *  the absolute values of big1 and big2 are taken. Then big1 is
 *  extended by one leading zero digit. After that big1 and big2
 *  are shifted to the left such that the most significant bit
 *  of big2 is set. This fulfills the preconditions for calling
 *  uBigDiv() which does the main work of the division.
 */
#ifdef ANSI_C

rtlBiginttype bigDiv (const const_rtlBiginttype big1, const const_rtlBiginttype big2)
#else

rtlBiginttype bigDiv (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    booltype negative = FALSE;
    rtlBiginttype big1_help;
    rtlBiginttype big2_help;
    unsigned int shift;
    rtlBiginttype result;

  /* bigDiv */
    if (big2->size == 1) {
      result = bigDiv1(big1, big2->bigdigits[0]);
      return(result);
    } else if (big1->size < big2->size) {
      result = bigDivSizeLess(big1, big2);
      return(result);
    } else {
      if (!ALLOC_BIG(big1_help, big1->size + 2)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          negative = TRUE;
          positive_copy_of_negative_big(big1_help, big1);
        } else {
          big1_help->size = big1->size;
          memcpy(big1_help->bigdigits, big1->bigdigits,
              (SIZE_TYPE) big1->size * sizeof(bigdigittype));
        } /* if */
        big1_help->bigdigits[big1_help->size] = 0;
        big1_help->size++;
      } /* if */
      if (!ALLOC_BIG(big2_help, big2->size + 1)) {
        FREE_BIG(big1_help,  big1->size + 2);
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        if (IS_NEGATIVE(big2->bigdigits[big2->size - 1])) {
          negative = !negative;
          positive_copy_of_negative_big(big2_help, big2);
        } else {
          big2_help->size = big2->size;
          memcpy(big2_help->bigdigits, big2->bigdigits,
              (SIZE_TYPE) big2->size * sizeof(bigdigittype));
        } /* if */
      } /* if */
      if (!ALLOC_BIG(result, big1_help->size - big2_help->size + 1)) {
        raise_error(MEMORY_ERROR);
      } else {
        result->size = big1_help->size - big2_help->size + 1;
        result->bigdigits[result->size - 1] = 0;
        shift = most_significant_bit(big2_help->bigdigits[big2_help->size - 1]) + 1;
        if (shift == 0) {
          /* The most significant digit of big2_help is 0. Just ignore it */
          big1_help->size--;
          big2_help->size--;
          if (big2_help->size == 1) {
            uBigDiv1(big1_help, big2_help->bigdigits[0], result);
          } else {
            uBigDiv(big1_help, big2_help, result);
          } /* if */
        } else {
          shift = 8 * sizeof(bigdigittype) - shift;
          uBigLShift(big1_help, shift);
          uBigLShift(big2_help, shift);
          uBigDiv(big1_help, big2_help, result);
        } /* if */
        if (negative) {
          negate_positive_big(result);
        } /* if */
        result = normalize(result);
      } /* if */
      FREE_BIG(big1_help, big1->size + 2);
      FREE_BIG(big2_help, big2->size + 1);
      return(result);
    } /* if */
  } /* bigDiv */



#ifdef ANSI_C

booltype bigEq (const const_rtlBiginttype big1, const const_rtlBiginttype big2)
#else

booltype bigEq (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  { /* bigEq */
    if (big1->size == big2->size &&
      memcmp(big1->bigdigits, big2->bigdigits,
          (SIZE_TYPE) big1->size * sizeof(bigdigittype)) == 0) {
      return(TRUE);
    } else {
      return(FALSE);
    } /* if */
  } /* bigEq */



#ifdef ANSI_C

rtlBiginttype bigFromInt32 (int32type number)
#else

rtlBiginttype bigFromInt32 (number)
int32type number;
#endif

  {
    memsizetype pos;
    memsizetype result_size;
    rtlBiginttype result;

  /* bigFromInt32 */
    result_size = sizeof(int32type) / sizeof(bigdigittype);
    if (!ALLOC_BIG(result, result_size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = result_size;
      for (pos = 0; pos < result_size; pos++) {
        result->bigdigits[pos] = (bigdigittype) (number & BIGDIGIT_MASK);
        number >>= 8 * sizeof(bigdigittype);
      } /* for */
      result = normalize(result);
      return(result);
    } /* if */
  } /* bigFromInt32 */



#ifdef INT64TYPE
#ifdef ANSI_C

rtlBiginttype bigFromInt64 (int64type number)
#else

rtlBiginttype bigFromInt64 (number)
int64type number;
#endif

  {
    memsizetype pos;
    memsizetype result_size;
    rtlBiginttype result;

  /* bigFromInt64 */
    result_size = sizeof(int64type) / sizeof(bigdigittype);
    if (!ALLOC_BIG(result, result_size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = result_size;
      for (pos = 0; pos < result_size; pos++) {
        result->bigdigits[pos] = (bigdigittype) (number & BIGDIGIT_MASK);
        number >>= 8 * sizeof(bigdigittype);
      } /* for */
      result = normalize(result);
      return(result);
    } /* if */
  } /* bigFromInt64 */
#endif



#ifdef ANSI_C

rtlBiginttype bigFromUInt32 (uint32type number)
#else

rtlBiginttype bigFromUInt32 (number)
uint32type number;
#endif

  {
    memsizetype pos;
    memsizetype result_size;
    rtlBiginttype result;

  /* bigFromUInt32 */
    result_size = sizeof(uint32type) / sizeof(bigdigittype) + 1;
    if (!ALLOC_BIG(result, result_size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = result_size;
      for (pos = 0; pos < result_size - 1; pos++) {
        result->bigdigits[pos] = (bigdigittype) (number & BIGDIGIT_MASK);
        number >>= 8 * sizeof(bigdigittype);
      } /* for */
      result->bigdigits[result_size - 1] = (bigdigittype) 0;
      result = normalize(result);
      return(result);
    } /* if */
  } /* bigFromUInt32 */



#ifdef INT64TYPE
#ifdef ANSI_C

rtlBiginttype bigFromUInt64 (uint64type number)
#else

rtlBiginttype bigFromUInt64 (number)
uint64type number;
#endif

  {
    memsizetype pos;
    memsizetype result_size;
    rtlBiginttype result;

  /* bigFromUInt64 */
    result_size = sizeof(uint64type) / sizeof(bigdigittype) + 1;
    if (!ALLOC_BIG(result, result_size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = result_size;
      for (pos = 0; pos < result_size - 1; pos++) {
        result->bigdigits[pos] = (bigdigittype) (number & BIGDIGIT_MASK);
        number >>= 8 * sizeof(bigdigittype);
      } /* for */
      result->bigdigits[result_size - 1] = (bigdigittype) 0;
      result = normalize(result);
      return(result);
    } /* if */
  } /* bigFromUInt64 */
#endif



#ifdef ANSI_C

rtlBiginttype bigGcd (const const_rtlBiginttype big1,
    const const_rtlBiginttype big2)
#else

rtlBiginttype bigGcd (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    rtlBiginttype big1_help;
    rtlBiginttype big2_help;
    inttype lowestSetBitA;
    inttype lowestSetBitB;
    inttype shift;
    rtlBiginttype help_big;
    rtlBiginttype result;

  /* bigGcd */
    if (big1->size == 1 && big1->bigdigits[0] == 0) {
      result = bigCreate(big2);
    } else if (big2->size == 1 && big2->bigdigits[0] == 0) {
      result = bigCreate(big1);
    } else {
      if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
        big1_help = bigMinus(big1);
      } else {
        big1_help = bigCreate(big1);
      } /* if */
      if (IS_NEGATIVE(big2->bigdigits[big2->size - 1])) {
        big2_help = bigMinus(big2);
      } else {
        big2_help = bigCreate(big2);
      } /* if */
      if ((big1_help->size > big2_help->size &&
          big1_help->size - big2_help->size > 10) ||
          (big1_help->size < big2_help->size &&
          big2_help->size - big1_help->size > 10)) {
        while (big1_help->size != 1 || big1_help->bigdigits[0] != 0) {
          help_big = bigRem(big2_help, big1_help);
          bigDestr(big2_help);
          big2_help = big1_help;
          big1_help = help_big;
        } /* while */
        result = big2_help;
        bigDestr(big1_help);
        return(result);
      } else {
        lowestSetBitA = bigLowestSetBit(big1_help);
        lowestSetBitB = bigLowestSetBit(big2_help);
        if (lowestSetBitA < lowestSetBitB) {
          shift = lowestSetBitA;
        } else {
          shift = lowestSetBitB;
        } /* if */
        bigRShiftAssign(&big1_help, lowestSetBitA);
        do {
          bigRShiftAssign(&big2_help, bigLowestSetBit(big2_help));
          if (bigCmp(big1_help, big2_help) < 0) {
            bigShrink(&(big2_help), big1_help);
          } else {
            help_big = bigSbtr(big1_help, big2_help);
            bigDestr(big1_help);
            big1_help = big2_help;
            big2_help = help_big;
          } /* if */
        } while (big2_help->size != 1 || big2_help->bigdigits[0] != 0);
        bigLShiftAssign(&big1_help, shift);
        result = big1_help;
        bigDestr(big2_help);
      } /* if */
    } /* if */
    return(result);
  } /* bigGcd */



/**
 *  Adds big2 to *big_variable. The operation is done in
 *  place and *big_variable is only resized when necessary.
 *  When the size of big2 is smaller than *big_variable the
 *  algorithm tries to save computations. Therefore there are
 *  checks for carry == 0 and carry != 0.
 *  In case the resizing fails the content of *big_variable
 *  is freed and *big_variable is set to NULL.
 */
#ifdef ANSI_C

void bigGrow (rtlBiginttype *const big_variable, const const_rtlBiginttype big2)
#else

void bigGrow (big_variable, big2)
rtlBiginttype *big_variable;
rtlBiginttype big2;
#endif

  {
    rtlBiginttype big1;
    memsizetype pos;
    doublebigdigittype carry = 0;
    doublebigdigittype big1_sign;
    doublebigdigittype big2_sign;
    rtlBiginttype resized_big1;

  /* bigGrow */
    big1 = *big_variable;
    if (big1->size >= big2->size) {
      big1_sign = IS_NEGATIVE(big1->bigdigits[big1->size - 1]) ? BIGDIGIT_MASK : 0;
      pos = 0;
      do {
        carry += (doublebigdigittype) big1->bigdigits[pos] + big2->bigdigits[pos];
        big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        pos++;
      } while (pos < big2->size);
      if (IS_NEGATIVE(big2->bigdigits[pos - 1])) {
        for (; carry == 0 && pos < big1->size; pos++) {
          carry = (doublebigdigittype) big1->bigdigits[pos] + BIGDIGIT_MASK;
          big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        carry += BIGDIGIT_MASK;
      } else {
        for (; carry != 0 && pos < big1->size; pos++) {
          carry += big1->bigdigits[pos];
          big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
      } /* if */
      pos = big1->size;
      carry += big1_sign;
      carry &= BIGDIGIT_MASK;
      if ((carry != 0 || IS_NEGATIVE(big1->bigdigits[pos - 1])) &&
          (carry != BIGDIGIT_MASK || !IS_NEGATIVE(big1->bigdigits[pos - 1]))) {
        resized_big1 = REALLOC_BIG(big1, big1->size, big1->size + 1);
        if (resized_big1 == NULL) {
          FREE_BIG(big1, big1->size);
          *big_variable = NULL;
          raise_error(MEMORY_ERROR);
        } else {
          big1 = resized_big1;
          COUNT3_BIG(big1->size, big1->size + 1);
          big1->size++;
          big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          *big_variable = big1;
        } /* if */
      } else {
        *big_variable = normalize(big1);
      } /* if */
    } else {
      resized_big1 = REALLOC_BIG(big1, big1->size, big2->size + 1);
      if (resized_big1 == NULL) {
        FREE_BIG(big1, big1->size);
        *big_variable = NULL;
        raise_error(MEMORY_ERROR);
      } else {
        big1 = resized_big1;
        COUNT3_BIG(big1->size, big2->size + 1);
        big1_sign = IS_NEGATIVE(big1->bigdigits[big1->size - 1]) ? BIGDIGIT_MASK : 0;
        pos = 0;
        do {
          carry += (doublebigdigittype) big1->bigdigits[pos] + big2->bigdigits[pos];
          big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
          pos++;
        } while (pos < big1->size);
        big2_sign = IS_NEGATIVE(big2->bigdigits[big2->size - 1]) ? BIGDIGIT_MASK : 0;
        for (; pos < big2->size; pos++) {
          carry += big1_sign + big2->bigdigits[pos];
          big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        carry += big1_sign + big2_sign;
        big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        big1->size = pos + 1;
        *big_variable = normalize(big1);
      } /* if */
    } /* if */
  } /* bigGrow */



#ifdef ANSI_C

inttype bigHashCode (const const_rtlBiginttype big1)
#else

inttype bigHashCode (big1)
rtlBiginttype big1;
#endif

  {
    inttype result;

  /* bigHashCode */
    result = big1->bigdigits[0] << 5 ^ big1->size << 3 ^ big1->bigdigits[big1->size - 1];
    return(result);
  } /* bigHashCode */



#ifdef ANSI_C

rtlBiginttype bigImport (ustritype buffer)
#else

rtlBiginttype bigImport (buffer)
ustritype buffer;
#endif

  {
    memsizetype byteDigitCount;
    memsizetype byteIndex;
    memsizetype pos;
    int byteNum;
    bigdigittype digit;
    memsizetype result_size;
    rtlBiginttype result;

  /* bigImport */
    byteDigitCount = ((memsizetype) buffer[0]) << 24 |
                     ((memsizetype) buffer[1]) << 16 |
                     ((memsizetype) buffer[2]) <<  8 |
                     ((memsizetype) buffer[3]);
    result_size = (byteDigitCount + sizeof(bigdigittype) - 1) / sizeof(bigdigittype);
    if (!ALLOC_BIG(result, result_size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = result_size;
      byteIndex = byteDigitCount;
      for (pos = 0; pos < result_size; pos++) {
        digit = 0;
        for (byteNum = 0; byteNum < sizeof(bigdigittype); byteNum++) {
          if (byteIndex > 0) {
            digit |= ((bigdigittype) buffer[3 + byteIndex]) << 8 * byteNum;
            byteIndex--;
          } else {
            if (buffer[4] >= 128) {
              digit |= ((bigdigittype) 0xFF) << 8 * byteNum;
            } /* if */
          } /* if */
        } /* for */
        result->bigdigits[pos] = digit;
      } /* for */
    } /* if */
    return(result);
  } /* bigImport */



#ifdef ANSI_C

void bigIncr (rtlBiginttype *const big_variable)
#else

void bigIncr (big_variable)
rtlBiginttype *big_variable;
#endif

  {
    rtlBiginttype big1;
    memsizetype pos;
    doublebigdigittype carry = 1;
    bigdigittype negative;

  /* bigIncr */
    big1 = *big_variable;
    negative = IS_NEGATIVE(big1->bigdigits[big1->size - 1]);
    pos = 0;
    do {
      carry += big1->bigdigits[pos];
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      pos++;
    } while (carry != 0 && pos < big1->size);
    pos = big1->size;
    if (IS_NEGATIVE(big1->bigdigits[pos - 1])) {
      if (!negative) {
        big1 = REALLOC_BIG(big1, pos, pos + 1);
        if (big1 == NULL) {
          raise_error(MEMORY_ERROR);
        } else {
          COUNT3_BIG(pos, pos + 1);
          big1->size++;
          big1->bigdigits[pos] = 0;
          *big_variable = big1;
        } /* if */
      } else if (big1->bigdigits[pos - 1] == BIGDIGIT_MASK &&
          pos >= 2 && IS_NEGATIVE(big1->bigdigits[pos - 2])) {
        big1 = REALLOC_BIG(big1, pos, pos - 1);
        if (big1 == NULL) {
          raise_error(MEMORY_ERROR);
        } else {
          COUNT3_BIG(pos, pos - 1);
          big1->size--;
          *big_variable = big1;
        } /* if */
      } /* if */
    } /* if */
  } /* bigIncr */



/**
 *  Computes base to the power of exponent for signed big integers.
 *  The result variable is set to base or 1 depending on the
 *  rightmost bit of the exponent. After that the base is
 *  squared in a loop and every time the corresponding bit of
 *  the exponent is set the current square is multiplied
 *  with the result variable. This reduces the number of square
 *  operations to ld(exponent).
 */
#ifdef ANSI_C

rtlBiginttype bigIPow (const const_rtlBiginttype base, inttype exponent)
#else

rtlBiginttype bigIPow (base, exponent)
rtlBiginttype base;
inttype exponent;
#endif

  {
    booltype negative = FALSE;
    memsizetype help_size;
    rtlBiginttype square;
    rtlBiginttype big_help;
    rtlBiginttype result;

  /* bigIPow */
    if (exponent < 0) {
      raise_error(NUMERIC_ERROR);
      return(NULL);
    } else {
      help_size = base->size * (exponent + 1);
      if (!ALLOC_BIG(square, help_size)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        if (!ALLOC_BIG(big_help, help_size)) {
          FREE_BIG(square,  help_size);
          raise_error(MEMORY_ERROR);
          return(NULL);
        } else {
          if (!ALLOC_BIG(result, help_size)) {
            FREE_BIG(square,  help_size);
            FREE_BIG(big_help,  help_size);
            raise_error(MEMORY_ERROR);
            return(NULL);
          } else {
            if (IS_NEGATIVE(base->bigdigits[base->size - 1])) {
              negative = TRUE;
              positive_copy_of_negative_big(square, base);
            } else {
              square->size = base->size;
              memcpy(square->bigdigits, base->bigdigits,
                  (SIZE_TYPE) base->size * sizeof(bigdigittype));
            } /* if */
            if (exponent & 1) {
              result->size = square->size;
              memcpy(result->bigdigits, square->bigdigits,
                  (SIZE_TYPE) square->size * sizeof(bigdigittype));
            } else {
              negative = FALSE;
              result->size = 1;
              result->bigdigits[0] = 1;
            } /* if */
            exponent >>= 1;
            while (exponent != 0) {
              square = uBigSqare(square, &big_help);
              if (exponent & 1) {
                result = uBigMultIntoHelp(result, square, &big_help);
              } /* if */
              exponent >>= 1;
            } /* while */
            memset(&result->bigdigits[result->size], 0,
                (SIZE_TYPE) (help_size - result->size) * sizeof(bigdigittype));
            result->size = help_size;
            if (negative) {
              negate_positive_big(result);
            } /* if */
            result = normalize(result);
            FREE_BIG(square, help_size);
            FREE_BIG(big_help, help_size);
            return(result);
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* bigIPow */



#ifdef ANSI_C

rtlBiginttype bigLog2 (const const_rtlBiginttype big1)
#else

rtlBiginttype bigLog2 (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype result_size;
    doublebigdigittype number;
    memsizetype pos;
    inttype bigdigit_log2;
    rtlBiginttype result;

  /* bigLog2 */
    if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
      raise_error(NUMERIC_ERROR);
      result = 0;
    } else {
      result_size = sizeof(inttype) / sizeof(bigdigittype) + 1;
      if (!ALLOC_BIG(result, result_size)) {
        raise_error(MEMORY_ERROR);
      } else {
        result->size = result_size;
        number = big1->size - 1;
        for (pos = 0; pos < result_size - 1; pos++) {
          result->bigdigits[pos] = (bigdigittype) (number & BIGDIGIT_MASK);
          number >>= 8 * sizeof(bigdigittype);
        } /* for */
        result->bigdigits[pos] = 0;
        uBigLShift(result, BIGDIGIT_LOG2_SIZE);
        bigdigit_log2 = most_significant_bit(big1->bigdigits[big1->size - 1]);
        if (bigdigit_log2 == -1) {
          uBigDecr(result);
        } else {
          result->bigdigits[0] |= bigdigit_log2;
        } /* if */
        result = normalize(result);
      } /* if */
    } /* if */
    return(result);
  } /* bigLog2 */



#ifdef ANSI_C

inttype bigLowestSetBit (const const_rtlBiginttype big1)
#else

inttype bigLowestSetBit (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype big1_size;
    memsizetype pos;

  /* bigLowestSetBit */
    big1_size = big1->size;
    pos = 0;
    while (pos < big1_size && big1->bigdigits[pos] == 0) {
      pos++;
    } /* while */
    if (pos < big1_size) {
      return((pos << BIGDIGIT_LOG2_SIZE) + least_significant_bit(big1->bigdigits[pos]));
    } else {
      return(-1);
    } /* if */
  } /* bigLowestSetBit */



#ifdef ANSI_C

rtlBiginttype bigLShift (const const_rtlBiginttype big1, const inttype lshift)
#else

rtlBiginttype bigLShift (big1, lshift)
rtlBiginttype big1;
inttype rshift;
#endif

  {
    unsigned int digit_rshift;
    unsigned int digit_lshift;
    bigdigittype digit_mask;
    bigdigittype low_digit;
    bigdigittype high_digit;
    const bigdigittype *source_digits;
    bigdigittype *dest_digits;
    memsizetype size_reduction;
    memsizetype pos;
    memsizetype result_size;
    rtlBiginttype result;

  /* bigLShift */
    if (lshift < 0) {
      result = NULL;
      raise_error(NUMERIC_ERROR);
    } else {
      if (big1->size == 1 && big1->bigdigits[0] == 0) {
        if (!ALLOC_BIG(result, 1)) {
          raise_error(MEMORY_ERROR);
        } else {
          result->size = 1;
          result->bigdigits[0] = 0;
        } /* if */
      } else if ((lshift & BIGDIGIT_SIZE_MASK) == 0) {
        result_size = big1->size + (lshift >> BIGDIGIT_LOG2_SIZE);
        if (!ALLOC_BIG(result, result_size)) {
          raise_error(MEMORY_ERROR);
        } else {
          result->size = result_size;
          memcpy(&result->bigdigits[lshift >> BIGDIGIT_LOG2_SIZE], big1->bigdigits,
              (SIZE_TYPE) big1->size * sizeof(bigdigittype));
          memset(result->bigdigits, 0,
              (lshift >> BIGDIGIT_LOG2_SIZE) * sizeof(bigdigittype));
        } /* if */
      } else {
        result_size = big1->size + (lshift >> BIGDIGIT_LOG2_SIZE) + 1;
        digit_lshift = lshift & BIGDIGIT_SIZE_MASK;
        digit_rshift = 8 * sizeof(bigdigittype) - digit_lshift;
        size_reduction = 0;
        low_digit = big1->bigdigits[big1->size - 1];
        if (IS_NEGATIVE(low_digit)) {
          digit_mask = (BIGDIGIT_MASK << (digit_rshift - 1)) & BIGDIGIT_MASK;
          if ((low_digit & digit_mask) == digit_mask) {
            result_size--;
            size_reduction = 1;
          } else {
            low_digit = BIGDIGIT_MASK;
          } /* if */
        } else {
          if (low_digit >> (digit_rshift - 1) == 0) {
            result_size--;
            size_reduction = 1;
          } else {
            low_digit = 0;
          } /* if */
        } /* if */
        if (!ALLOC_BIG(result, result_size)) {
          raise_error(MEMORY_ERROR);
        } else {
          result->size = result_size;
          dest_digits = &result->bigdigits[result_size];
          if (size_reduction) {
            source_digits = &big1->bigdigits[big1->size - 1];
          } else {
            source_digits = &big1->bigdigits[big1->size];
          } /* if */
          high_digit = (low_digit << digit_lshift) & BIGDIGIT_MASK;
          for (pos = big1->size - size_reduction; pos != 0; pos--) {
            low_digit = *--source_digits;
            *--dest_digits = high_digit | (low_digit >> digit_rshift);
            high_digit = (low_digit << digit_lshift) & BIGDIGIT_MASK;
          } /* for */
          *--dest_digits = high_digit;
          if (dest_digits > result->bigdigits) {
            memset(result->bigdigits, 0,
                (dest_digits - result->bigdigits) * sizeof(bigdigittype));
          } /* if */
        } /* if */
      } /* if */
    } /* if */
    return(result);
  } /* bigLShift */



#ifdef ANSI_C

void bigLShiftAssign (rtlBiginttype *const big_variable, inttype lshift)
#else

void bigLShiftAssign (big_variable, rshift)
rtlBiginttype *const big_variable;
inttype lshift;
#endif

  {
    rtlBiginttype big1;
    unsigned int digit_rshift;
    unsigned int digit_lshift;
    bigdigittype digit_mask;
    bigdigittype low_digit;
    bigdigittype high_digit;
    const bigdigittype *source_digits;
    bigdigittype *dest_digits;
    memsizetype size_reduction;
    memsizetype pos;
    memsizetype result_size;
    rtlBiginttype result;

  /* bigLShiftAssign */
    if (lshift < 0) {
      raise_error(NUMERIC_ERROR);
    } else if (lshift != 0) {
      big1 = *big_variable;
      if (big1->size == 1 && big1->bigdigits[0] == 0) {
        if (!ALLOC_BIG(result, 1)) {
          raise_error(MEMORY_ERROR);
        } else {
          result->size = 1;
          result->bigdigits[0] = 0;
          *big_variable = result;
          FREE_BIG(big1, big1->size);
        } /* if */
      } else if ((lshift & BIGDIGIT_SIZE_MASK) == 0) {
        result_size = big1->size + (lshift >> BIGDIGIT_LOG2_SIZE);
        if (!ALLOC_BIG(result, result_size)) {
          raise_error(MEMORY_ERROR);
        } else {
          result->size = result_size;
          memcpy(&result->bigdigits[lshift >> BIGDIGIT_LOG2_SIZE], big1->bigdigits,
              (SIZE_TYPE) big1->size * sizeof(bigdigittype));
          memset(result->bigdigits, 0,
              (lshift >> BIGDIGIT_LOG2_SIZE) * sizeof(bigdigittype));
          *big_variable = result;
          FREE_BIG(big1, big1->size);
        } /* if */
      } else {
        result_size = big1->size + (lshift >> BIGDIGIT_LOG2_SIZE) + 1;
        digit_lshift = lshift & BIGDIGIT_SIZE_MASK;
        digit_rshift = 8 * sizeof(bigdigittype) - digit_lshift;
        size_reduction = 0;
        low_digit = big1->bigdigits[big1->size - 1];
        if (IS_NEGATIVE(low_digit)) {
          digit_mask = (BIGDIGIT_MASK << (digit_rshift - 1)) & BIGDIGIT_MASK;
          if ((low_digit & digit_mask) == digit_mask) {
            result_size--;
            size_reduction = 1;
          } else {
            low_digit = BIGDIGIT_MASK;
          } /* if */
        } else {
          if (low_digit >> (digit_rshift - 1) == 0) {
            result_size--;
            size_reduction = 1;
          } else {
            low_digit = 0;
          } /* if */
        } /* if */
        if (result_size != big1->size) {
          if (!ALLOC_BIG(result, result_size)) {
            raise_error(MEMORY_ERROR);
          } /* if */
        } else {
          result = big1;
        } /* if */
        if (result != NULL) {
          result->size = result_size;
          dest_digits = &result->bigdigits[result_size];
          if (size_reduction) {
            source_digits = &big1->bigdigits[big1->size - 1];
          } else {
            source_digits = &big1->bigdigits[big1->size];
          } /* if */
          high_digit = (low_digit << digit_lshift) & BIGDIGIT_MASK;
          for (pos = big1->size - size_reduction; pos != 0; pos--) {
            low_digit = *--source_digits;
            *--dest_digits = high_digit | (low_digit >> digit_rshift);
            high_digit = (low_digit << digit_lshift) & BIGDIGIT_MASK;
          } /* for */
          *--dest_digits = high_digit;
          if (dest_digits > result->bigdigits) {
            memset(result->bigdigits, 0,
                (dest_digits - result->bigdigits) * sizeof(bigdigittype));
          } /* if */
          if (result != big1) {
            *big_variable = result;
            FREE_BIG(big1, big1->size);
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* bigLShiftAssign */



/**
 *  Computes an integer modulo division of big1 by big2 for signed
 *  big integers. The memory for the result is requested and the
 *  normalized result is returned. When big2 has just one digit
 *  or when big1 has less digits than big2 the bigDiv1() or
 *  bigDivSizeLess() functions are called. In the general case
 *  the absolute values of big1 and big2 are taken. Then big1 is
 *  extended by one leading zero digit. After that big1 and big2
 *  are shifted to the left such that the most significant bit
 *  of big2 is set. This fulfills the preconditions for calling
 *  uBigDiv() which does the main work of the division.
 */
#ifdef ANSI_C

rtlBiginttype bigMDiv (const const_rtlBiginttype big1, const const_rtlBiginttype big2)
#else

rtlBiginttype bigMDiv (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    booltype negative = FALSE;
    rtlBiginttype big1_help;
    rtlBiginttype big2_help;
    unsigned int shift;
    bigdigittype mdiv1_remainder = 0;
    rtlBiginttype result;

  /* bigMDiv */
    if (big2->size == 1) {
      result = bigMDiv1(big1, big2->bigdigits[0]);
      return(result);
    } else if (big1->size < big2->size) {
      result = bigMDivSizeLess(big1, big2);
      return(result);
    } else {
      if (!ALLOC_BIG(big1_help, big1->size + 2)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          negative = TRUE;
          positive_copy_of_negative_big(big1_help, big1);
        } else {
          big1_help->size = big1->size;
          memcpy(big1_help->bigdigits, big1->bigdigits,
              (SIZE_TYPE) big1->size * sizeof(bigdigittype));
        } /* if */
        big1_help->bigdigits[big1_help->size] = 0;
        big1_help->size++;
      } /* if */
      if (!ALLOC_BIG(big2_help, big2->size + 1)) {
        FREE_BIG(big1_help,  big1->size + 2);
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        if (IS_NEGATIVE(big2->bigdigits[big2->size - 1])) {
          negative = !negative;
          positive_copy_of_negative_big(big2_help, big2);
        } else {
          big2_help->size = big2->size;
          memcpy(big2_help->bigdigits, big2->bigdigits,
              (SIZE_TYPE) big2->size * sizeof(bigdigittype));
        } /* if */
      } /* if */
      if (!ALLOC_BIG(result, big1_help->size - big2_help->size + 1)) {
        raise_error(MEMORY_ERROR);
      } else {
        result->size = big1_help->size - big2_help->size + 1;
        result->bigdigits[result->size - 1] = 0;
        shift = most_significant_bit(big2_help->bigdigits[big2_help->size - 1]) + 1;
        if (shift == 0) {
          /* The most significant digit of big2_help is 0. Just ignore it */
          big1_help->size--;
          big2_help->size--;
          if (big2_help->size == 1) {
            mdiv1_remainder = uBigMDiv1(big1_help, big2_help->bigdigits[0], result);
          } else {
            uBigDiv(big1_help, big2_help, result);
          } /* if */
        } else {
          shift = 8 * sizeof(bigdigittype) - shift;
          uBigLShift(big1_help, shift);
          uBigLShift(big2_help, shift);
          uBigDiv(big1_help, big2_help, result);
        } /* if */
        if (negative) {
          if ((big2_help->size == 1 && mdiv1_remainder != 0) ||
              (big2_help->size != 1 && uBigIsNot0(big1_help))) {
            uBigIncr(result);
          } /* if */
          negate_positive_big(result);
        } /* if */
        result = normalize(result);
      } /* if */
      FREE_BIG(big1_help, big1->size + 2);
      FREE_BIG(big2_help, big2->size + 1);
      return(result);
    } /* if */
  } /* bigMDiv */



#ifdef ANSI_C

rtlBiginttype bigMinus (const const_rtlBiginttype big1)
#else

rtlBiginttype bigMinus (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;
    rtlBiginttype result;

  /* bigMinus */
    if (!ALLOC_BIG(result, big1->size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = big1->size;
      pos = 0;
      do {
        carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
        result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        pos++;
      } while (pos < big1->size);
      if (IS_NEGATIVE(result->bigdigits[pos - 1])) {
        if (IS_NEGATIVE(big1->bigdigits[pos - 1])) {
          result = REALLOC_BIG(result, pos, pos + 1);
          if (result == NULL) {
            raise_error(MEMORY_ERROR);
            return(NULL);
          } else {
            COUNT3_BIG(pos, pos + 1);
            result->size++;
            result->bigdigits[pos] = 0;
          } /* if */
        } else if (result->bigdigits[pos - 1] == BIGDIGIT_MASK &&
            pos >= 2 && IS_NEGATIVE(result->bigdigits[pos - 2])) {
          result = REALLOC_BIG(result, pos, pos - 1);
          if (result == NULL) {
            raise_error(MEMORY_ERROR);
            return(NULL);
          } else {
            COUNT3_BIG(pos, pos - 1);
            result->size--;
          } /* if */
        } /* if */
      } /* if */
      return(result);
    } /* if */
  } /* bigMinus */



/**
 *  Computes the modulo of an integer division of big1 by big2
 *  for signed big integers. The memory for the result is requested
 *  and the normalized result is returned. When big2 has just one
 *  digit or when big1 has less digits than big2 the bigMod1() or
 *  bigModSizeLess() functions are called. In the general case
 *  the absolute values of big1 and big2 are taken. Then big1 is
 *  extended by one leading zero digit. After that big1 and big2
 *  are shifted to the left such that the most significant bit
 *  of big2 is set. This fulfills the preconditions for calling
 *  uBigRem() which does the main work of the division. Afterwards
 *  the result must be shifted to the right to get the remainder.
 *  If big1 and big2 have the same sign the modulo has the same
 *  value as the remainder. When the remainder is zero the modulo
 *  is also zero. If the signs of big1 and big2 are different the
 *  modulo is computed from the remainder by adding big1.
 */
#ifdef ANSI_C

rtlBiginttype bigMod (const const_rtlBiginttype big1, const const_rtlBiginttype big2)
#else

rtlBiginttype bigMod (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    booltype negative1 = FALSE;
    booltype negative2 = FALSE;
    rtlBiginttype big2_help;
    unsigned int shift;
    rtlBiginttype result;

  /* bigMod */
    if (big2->size == 1) {
      result = bigMod1(big1, big2->bigdigits[0]);
      return(result);
    } else if (big1->size < big2->size) {
      result = bigModSizeLess(big1, big2);
      return(result);
    } else {
      if (!ALLOC_BIG(result, big1->size + 2)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          negative1 = TRUE;
          positive_copy_of_negative_big(result, big1);
        } else {
          result->size = big1->size;
          memcpy(result->bigdigits, big1->bigdigits,
              (SIZE_TYPE) big1->size * sizeof(bigdigittype));
        } /* if */
        result->bigdigits[result->size] = 0;
        result->size++;
      } /* if */
      if (!ALLOC_BIG(big2_help, big2->size + 1)) {
        FREE_BIG(result,  big1->size + 2);
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        if (IS_NEGATIVE(big2->bigdigits[big2->size - 1])) {
          negative2 = TRUE;
          positive_copy_of_negative_big(big2_help, big2);
        } else {
          big2_help->size = big2->size;
          memcpy(big2_help->bigdigits, big2->bigdigits,
              (SIZE_TYPE) big2->size * sizeof(bigdigittype));
        } /* if */
      } /* if */
      shift = most_significant_bit(big2_help->bigdigits[big2_help->size - 1]) + 1;
      if (shift == 0) {
        /* The most significant digit of big2_help is 0. Just ignore it */
        result->size--;
        big2_help->size--;
        if (big2_help->size == 1) {
          result->bigdigits[0] = uBigRem1(result, big2_help->bigdigits[0]);
          memset(&result->bigdigits[1], 0, 
              (SIZE_TYPE) (result->size - 1) * sizeof(bigdigittype));
        } else {
          uBigRem(result, big2_help);
        } /* if */
        result->bigdigits[result->size] = 0;
        big2_help->size++;
      } else {
        shift = 8 * sizeof(bigdigittype) - shift;
        uBigLShift(result, shift);
        uBigLShift(big2_help, shift);
        uBigRem(result, big2_help);
        uBigRShift(result, shift);
      } /* if */
      result->bigdigits[big1->size + 1] = 0;
      result->size = big1->size + 2;
      if (negative1) {
        if (negative2) {
          negate_positive_big(result);
        } else {
          if (uBigIsNot0(result)) {
            negate_positive_big(result);
            bigAddTo(result, big2);
          } /* if */
        } /* if */
      } else {
        if (negative2) {
          if (uBigIsNot0(result)) {
            bigAddTo(result, big2);
          } /* if */
        } /* if */
      } /* if */
      result = normalize(result);
      FREE_BIG(big2_help, big2->size + 1);
      return(result);
    } /* if */
  } /* bigMod */



/**
 *  Returns the product of two signed big integers.
 */
#ifdef ANSI_C

rtlBiginttype bigMult (const_rtlBiginttype big1, const_rtlBiginttype big2)
#else

rtlBiginttype bigMult (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    booltype negative = FALSE;
    rtlBiginttype big1_help = NULL;
    rtlBiginttype big2_help = NULL;
    rtlBiginttype result;

  /* bigMult */
    if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
      negative = TRUE;
      big1_help = alloc_positive_copy_of_negative_big(big1);
      big1 = big1_help;
      if (big1_help == NULL) {
        return(NULL);
      } /* if */
    } /* if */
    if (IS_NEGATIVE(big2->bigdigits[big2->size - 1])) {
      negative = !negative;
      big2_help = alloc_positive_copy_of_negative_big(big2);
      big2 = big2_help;
      if (big2_help == NULL) {
        if (big1_help != NULL) {
          FREE_BIG(big1_help, big1_help->size);
        } /* if */
        return(NULL);
      } /* if */
    } /* if */
    /* printf("bigMult(%u, %u)\n", big1->size, big2->size); */
#if 0
    if (!ALLOC_BIG(result, big1->size + big2->size)) {
      raise_error(MEMORY_ERROR);
    } else {
      uBigMult(big1, big2, result);
      result->size = big1->size + big2->size;
      if (negative) {
        negate_positive_big(result);
      } /* if */
      result = normalize(result);
    } /* if */
#else
    result = uBigMultK(big1, big2, negative);
#endif
    if (big1_help != NULL) {
      FREE_BIG(big1_help, big1_help->size);
    } /* if */
    if (big2_help != NULL) {
      FREE_BIG(big2_help, big2_help->size);
    } /* if */
    return(result);
  } /* bigMult */



#ifdef ANSI_C

void bigMultAssign (rtlBiginttype *const big_variable, const_rtlBiginttype big2)
#else

void bigMultAssign (big_variable, big2)
rtlBiginttype *big_variable;
rtlBiginttype big2;
#endif

  {
    rtlBiginttype big1;
    booltype negative = FALSE;
    rtlBiginttype big1_help = NULL;
    rtlBiginttype big2_help = NULL;
    memsizetype pos1;
    memsizetype pos2;
    doublebigdigittype carry = 0;
    rtlBiginttype result;

  /* bigMultAssign */
    big1 = *big_variable;
    if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
      negative = TRUE;
      big1_help = alloc_positive_copy_of_negative_big(big1);
      big1 = big1_help;
      if (big1_help == NULL) {
        return;
      } /* if */
    } /* if */
    if (IS_NEGATIVE(big2->bigdigits[big2->size - 1])) {
      negative = !negative;
      big2_help = alloc_positive_copy_of_negative_big(big2);
      big2 = big2_help;
      if (big2_help == NULL) {
        if (big1_help != NULL) {
          FREE_BIG(big1_help, big1_help->size);
        } /* if */
        return;
      } /* if */
    } /* if */
    if (!ALLOC_BIG(result, big1->size + big2->size)) {
      raise_error(MEMORY_ERROR);
    } else {
      pos2 = 0;
      do {
        carry += (doublebigdigittype) big1->bigdigits[0] * big2->bigdigits[pos2];
        result->bigdigits[pos2] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        pos2++;
      } while (pos2 < big2->size);
      result->bigdigits[big2->size] = (bigdigittype) (carry & BIGDIGIT_MASK);
      for (pos1 = 1; pos1 < big1->size; pos1++) {
        carry = 0;
        pos2 = 0;
        do {
          carry += (doublebigdigittype) result->bigdigits[pos1 + pos2] +
              (doublebigdigittype) big1->bigdigits[pos1] * big2->bigdigits[pos2];
          result->bigdigits[pos1 + pos2] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
          pos2++;
        } while (pos2 < big2->size);
        result->bigdigits[pos1 + big2->size] = (bigdigittype) (carry & BIGDIGIT_MASK);
      } /* for */
      result->size = big1->size + big2->size;
      if (negative) {
        negate_positive_big(result);
      } /* if */
      result = normalize(result);
      FREE_BIG(*big_variable, (*big_variable)->size);
      *big_variable = result;
    } /* if */
    if (big1_help != NULL) {
      FREE_BIG(big1_help, big1_help->size);
    } /* if */
    if (big2_help != NULL) {
      FREE_BIG(big2_help, big2_help->size);
    } /* if */
  } /* bigMultAssign */



#ifdef OUT_OF_ORDER
#ifdef ANSI_C

rtlBiginttype bigMultSignedDigit (const_rtlBiginttype big1, inttype number)
#else

rtlBiginttype bigMultSignedDigit (big1, number)
rtlBiginttype big1;
inttype number;
#endif

  {
    rtlBiginttype result;

  /* bigMultSignedDigit */
    if (!ALLOC_BIG(result, big1->size + 1)) {
      raise_error(MEMORY_ERROR);
    } else {
      result->size = big1->size + 1;
      if (number < 0) {
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          uBigMultNegativeWithNegatedDigit(big1, (bigdigittype) -number, result);
        } else {
          uBigMultPositiveWithNegatedDigit(big1, (bigdigittype) -number, result);
        } /* if */
      } else {
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          uBigMultNegativeWithDigit(big1, (bigdigittype) number, result);
        } else {
          uBigMultPositiveWithDigit(big1, (bigdigittype) number, result);
        } /* if */
      } /* if */
      result = normalize(result);
    } /* if */
    return(result);
  } /* bigMultSignedDigit */
#endif



#ifdef ANSI_C

booltype bigNe (const const_rtlBiginttype big1, const const_rtlBiginttype big2)
#else

booltype bigNe (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  { /* bigNe */
    if (big1->size != big2->size ||
      memcmp(big1->bigdigits, big2->bigdigits,
          (SIZE_TYPE) big1->size * sizeof(bigdigittype)) != 0) {
      return(TRUE);
    } else {
      return(FALSE);
    } /* if */
  } /* bigNe */



#ifdef ANSI_C

booltype bigOdd (const const_rtlBiginttype big1)
#else

booltype bigOdd (big1)
rtlBiginttype big1;
#endif

  { /* bigOdd */
    return(big1->bigdigits[0] & 1);
  } /* bigOdd */



#ifdef ANSI_C

rtlBiginttype bigParse (const const_stritype stri)
#else

rtlBiginttype bigParse (stri)
stritype stri;
#endif

  {
    memsizetype result_size;
    booltype okay;
    booltype negative;
    memsizetype position;
    doublebigdigittype digitval;
    rtlBiginttype result;

  /* bigParse */
    if (stri->size == 0) {
      result_size = 1;
    } else {
      result_size = (stri->size - 1) / DECIMAL_DIGITS_IN_BIGDIGIT + 1;
    } /* if */
    if (!ALLOC_BIG(result, result_size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = 1;
      result->bigdigits[0] = 0;
      okay = TRUE;
      position = 0;
      if (stri->size >= 1 && stri->mem[0] == ((strelemtype) '-')) {
        negative = TRUE;
        position++;
      } else {
        negative = FALSE;
      } /* if */
      while (position < stri->size &&
          stri->mem[position] >= ((strelemtype) '0') &&
          stri->mem[position] <= ((strelemtype) '9')) {
        digitval = ((doublebigdigittype) (stri->mem[position]) - ((bigdigittype) '0'));
        uBigMultBy10AndAdd(result, digitval);
        position++;
      } /* while */
      if (position == 0 || position < stri->size) {
        okay = FALSE;
      } /* if */
      if (okay) {
        memset(&result->bigdigits[result->size], 0,
            (SIZE_TYPE) (result_size - result->size) * sizeof(bigdigittype));
        result->size = result_size;
        if (negative) {
          negate_positive_big(result);
        } /* if */
        result = normalize(result);
        return(result);
      } else {
        FREE_BIG(result, result_size);
        raise_error(RANGE_ERROR);
        return(NULL);
      } /* if */
    } /* if */
  } /* bigParse */



#ifdef ANSI_C

rtlBiginttype bigPred (const const_rtlBiginttype big1)
#else

rtlBiginttype bigPred (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;
    rtlBiginttype result;

  /* bigPred */
    if (!ALLOC_BIG(result, big1->size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = big1->size;
      pos = 0;
      do {
        carry += (doublebigdigittype) big1->bigdigits[pos] + BIGDIGIT_MASK;
        result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        pos++;
      } while (pos < big1->size);
      if (!IS_NEGATIVE(result->bigdigits[pos - 1])) {
        if (IS_NEGATIVE(big1->bigdigits[pos - 1])) {
          result = REALLOC_BIG(result, pos, pos + 1);
          if (result == NULL) {
            raise_error(MEMORY_ERROR);
            return(NULL);
          } else {
            COUNT3_BIG(pos, pos + 1);
            result->size++;
            result->bigdigits[pos] = BIGDIGIT_MASK;
          } /* if */
        } else if (result->bigdigits[pos - 1] == 0 &&
            pos >= 2 && !IS_NEGATIVE(result->bigdigits[pos - 2])) {
          result = REALLOC_BIG(result, pos, pos - 1);
          if (result == NULL) {
            raise_error(MEMORY_ERROR);
            return(NULL);
          } else {
            COUNT3_BIG(pos, pos - 1);
            result->size--;
          } /* if */
        } /* if */
      } /* if */
      return(result);
    } /* if */
  } /* bigPred */



/**
 *  Computes a random number between lower_limit and upper_limit
 *  for signed big integers. The memory for the result is requested
 *  and the normalized result is returned. The random numbers are
 *  uniform distributed over the whole range.
 */
#ifdef ANSI_C

rtlBiginttype bigRand (const const_rtlBiginttype lower_limit,
    const const_rtlBiginttype upper_limit)
#else

rtlBiginttype bigRand (lower_limit, upper_limit)
rtlBiginttype lower_limit;
rtlBiginttype upper_limit;
#endif

  {
    rtlBiginttype scale_limit;
    bigdigittype mask;
    memsizetype pos;
    doublebigdigittype random_number = 0;
    memsizetype result_size;
    rtlBiginttype result;

  /* bigRand */
    if (bigCmp(lower_limit, upper_limit) > 0) {
      raise_error(RANGE_ERROR);
      return(0);
    } else {
      scale_limit = bigSbtr(upper_limit, lower_limit);
      if (lower_limit->size > scale_limit->size) {
        result_size = lower_limit->size + 1;
      } else {
        result_size = scale_limit->size + 1;
      } /* if */
      if (!ALLOC_BIG(result, result_size)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        memset(&result->bigdigits[scale_limit->size], 0,
            (SIZE_TYPE) (result_size - scale_limit->size) * sizeof(bigdigittype));
        result->size = scale_limit->size;
        mask = ((bigdigittype) BIGDIGIT_MASK) >>
            (8 * sizeof(bigdigittype) -
            most_significant_bit(scale_limit->bigdigits[scale_limit->size - 1]) - 1);
        do {
          pos = 0;
          do {
            if (random_number == 0) {
              random_number = rand_32();
            } /* if */
            result->bigdigits[pos] = (bigdigittype) (random_number & BIGDIGIT_MASK);
            random_number >>= 8 * sizeof(bigdigittype);
            pos++;
          } while (pos < scale_limit->size);
          result->bigdigits[pos - 1] &= mask;
        } while (bigCmp(result, scale_limit) > 0);
        result->size = result_size;
        bigAddTo(result, lower_limit);
        result = normalize(result);
        FREE_BIG(scale_limit, scale_limit->size);
        return(result);
      } /* if */
    } /* if */
  } /* bigRand */



/**
 *  Computes the remainder of an integer division of big1 by big2
 *  for signed big integers. The memory for the result is requested
 *  and the normalized result is returned. When big2 has just one
 *  digit or when big1 has less digits than big2 the bigRem1() or
 *  bigRemSizeLess() functions are called. In the general case
 *  the absolute values of big1 and big2 are taken. Then big1 is
 *  extended by one leading zero digit. After that big1 and big2
 *  are shifted to the left such that the most significant bit
 *  of big2 is set. This fulfills the preconditions for calling
 *  uBigRem() which does the main work of the division. Afterwards
 *  the result must be shifted to the right to get the remainder.
 */
#ifdef ANSI_C

rtlBiginttype bigRem (const const_rtlBiginttype big1, const const_rtlBiginttype big2)
#else

rtlBiginttype bigRem (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    booltype negative = FALSE;
    rtlBiginttype big2_help;
    unsigned int shift;
    rtlBiginttype result;

  /* bigRem */
    if (big2->size == 1) {
      result = bigRem1(big1, big2->bigdigits[0]);
      return(result);
    } else if (big1->size < big2->size) {
      result = bigRemSizeLess(big1, big2);
      return(result);
    } else {
      if (!ALLOC_BIG(result, big1->size + 2)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          negative = TRUE;
          positive_copy_of_negative_big(result, big1);
        } else {
          result->size = big1->size;
          memcpy(result->bigdigits, big1->bigdigits,
              (SIZE_TYPE) big1->size * sizeof(bigdigittype));
        } /* if */
        result->bigdigits[result->size] = 0;
        result->size++;
      } /* if */
      if (!ALLOC_BIG(big2_help, big2->size + 1)) {
        FREE_BIG(result,  big1->size + 2);
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        if (IS_NEGATIVE(big2->bigdigits[big2->size - 1])) {
          positive_copy_of_negative_big(big2_help, big2);
        } else {
          big2_help->size = big2->size;
          memcpy(big2_help->bigdigits, big2->bigdigits,
              (SIZE_TYPE) big2->size * sizeof(bigdigittype));
        } /* if */
      } /* if */
      shift = most_significant_bit(big2_help->bigdigits[big2_help->size - 1]) + 1;
      if (shift == 0) {
        /* The most significant digit of big2_help is 0. Just ignore it */
        result->size--;
        big2_help->size--;
        if (big2_help->size == 1) {
          result->bigdigits[0] = uBigRem1(result, big2_help->bigdigits[0]);
          memset(&result->bigdigits[1], 0, 
              (SIZE_TYPE) (result->size - 1) * sizeof(bigdigittype));
        } else {
          uBigRem(result, big2_help);
        } /* if */
        result->bigdigits[result->size] = 0;
        big2_help->size++;
      } else {
        shift = 8 * sizeof(bigdigittype) - shift;
        uBigLShift(result, shift);
        uBigLShift(big2_help, shift);
        uBigRem(result, big2_help);
        uBigRShift(result, shift);
      } /* if */
      result->bigdigits[big1->size + 1] = 0;
      result->size = big1->size + 2;
      if (negative) {
        negate_positive_big(result);
      } /* if */
      result = normalize(result);
      FREE_BIG(big2_help, big2->size + 1);
      return(result);
    } /* if */
  } /* bigRem */



#ifdef ANSI_C

rtlBiginttype bigRShift (const const_rtlBiginttype big1, const inttype rshift)
#else

rtlBiginttype bigRShift (big1, rshift)
rtlBiginttype big1;
inttype rshift;
#endif

  {
    unsigned int digit_rshift;
    unsigned int digit_lshift;
    bigdigittype digit_mask;
    bigdigittype low_digit;
    bigdigittype high_digit;
    const bigdigittype *source_digits;
    bigdigittype *dest_digits;
    memsizetype pos;
    memsizetype result_size;
    rtlBiginttype result;

  /* bigRShift */
    if (rshift < 0) {
      result = NULL;
      raise_error(NUMERIC_ERROR);
    } else {
      if (big1->size <= rshift >> BIGDIGIT_LOG2_SIZE) {
        if (!ALLOC_BIG(result, 1)) {
          raise_error(MEMORY_ERROR);
        } else {
          result->size = 1;
          if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
            result->bigdigits[0] = BIGDIGIT_MASK;
          } else {
            result->bigdigits[0] = 0;
          } /* if */
        } /* if */
      } else if ((rshift & BIGDIGIT_SIZE_MASK) == 0) {
        result_size = big1->size - (rshift >> BIGDIGIT_LOG2_SIZE);
        if (!ALLOC_BIG(result, result_size)) {
          raise_error(MEMORY_ERROR);
        } else {
          result->size = result_size;
          memcpy(result->bigdigits, &big1->bigdigits[rshift >> BIGDIGIT_LOG2_SIZE],
              (SIZE_TYPE) result_size * sizeof(bigdigittype));
        } /* if */
      } else {
        result_size = big1->size - (rshift >> BIGDIGIT_LOG2_SIZE);
        digit_rshift = rshift & BIGDIGIT_SIZE_MASK;
        digit_lshift = 8 * sizeof(bigdigittype) - digit_rshift;
        if (result_size > 1) {
          high_digit = big1->bigdigits[big1->size - 1];
          if (IS_NEGATIVE(high_digit)) {
            digit_mask = (BIGDIGIT_MASK << (digit_rshift - 1)) & BIGDIGIT_MASK;
            if ((digit_rshift == 1 && high_digit == BIGDIGIT_MASK) ||
                (high_digit & digit_mask) == digit_mask) {
              result_size--;
            } /* if */
          } else {
            if ((digit_rshift == 1 && high_digit == 0) ||
                high_digit >> (digit_rshift - 1) == 0) {
              result_size--;
            } /* if */
          } /* if */
        } /* if */
        if (!ALLOC_BIG(result, result_size)) {
          raise_error(MEMORY_ERROR);
        } else {
          result->size = result_size;
          source_digits = &big1->bigdigits[rshift >> BIGDIGIT_LOG2_SIZE];
          dest_digits = result->bigdigits;
          high_digit = *source_digits++;
          low_digit = high_digit >> digit_rshift;
          for (pos = big1->size - (rshift >> BIGDIGIT_LOG2_SIZE) - 1; pos != 0; pos--) {
            high_digit = *source_digits++;
            *dest_digits++ = low_digit | ((high_digit << digit_lshift) & BIGDIGIT_MASK);
            low_digit = high_digit >> digit_rshift;
          } /* for */
          if (dest_digits - result->bigdigits < result_size) {
            if (IS_NEGATIVE(high_digit)) {
              *dest_digits = low_digit | ((BIGDIGIT_MASK << digit_lshift) & BIGDIGIT_MASK);
            } else {
              *dest_digits = low_digit;
            } /* if */
          } /* if */
        } /* if */
      } /* if */
    } /* if */
    return(result);
  } /* bigRShift */



#ifdef ANSI_C

void bigRShiftAssign (rtlBiginttype *const big_variable, inttype rshift)
#else

void bigRShiftAssign (big_variable, rshift)
rtlBiginttype *const big_variable;
inttype rshift;
#endif

  {
    rtlBiginttype big1;
    memsizetype size_reduction;
    unsigned int digit_rshift;
    unsigned int digit_lshift;
    bigdigittype low_digit;
    bigdigittype high_digit;
    const bigdigittype *source_digits;
    bigdigittype *dest_digits;
    memsizetype pos;

  /* bigRShiftAssign */
    if (rshift < 0) {
      raise_error(NUMERIC_ERROR);
    } else {
      big1 = *big_variable;
      size_reduction = rshift >> BIGDIGIT_LOG2_SIZE;
      if (big1->size <= size_reduction) {
        if (!ALLOC_BIG(*big_variable, 1)) {
          raise_error(MEMORY_ERROR);
        } else {
          (*big_variable)->size = 1;
          if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
            (*big_variable)->bigdigits[0] = BIGDIGIT_MASK;
          } else {
            (*big_variable)->bigdigits[0] = 0;
          } /* if */
          FREE_BIG(big1, big1->size);
        } /* if */
      } else if ((rshift & BIGDIGIT_SIZE_MASK) == 0) {
        if (rshift != 0) {
          memmove(big1->bigdigits, &big1->bigdigits[size_reduction],
              (SIZE_TYPE) (big1->size - size_reduction) * sizeof(bigdigittype));
          big1 = REALLOC_BIG(big1, big1->size, big1->size - size_reduction);
          if (big1 == NULL) {
            raise_error(MEMORY_ERROR);
          } else {
            COUNT3_BIG(big1->size, big1->size - size_reduction);
            big1->size -= size_reduction;
            *big_variable = big1;
          } /* if */
        } /* if */
      } else {
        digit_rshift = rshift & BIGDIGIT_SIZE_MASK;
        digit_lshift = 8 * sizeof(bigdigittype) - digit_rshift;
        source_digits = &big1->bigdigits[size_reduction];
        dest_digits = big1->bigdigits;
        high_digit = *source_digits++;
        low_digit = high_digit >> digit_rshift;
        for (pos = big1->size - size_reduction - 1; pos != 0; pos--) {
          high_digit = *source_digits++;
          *dest_digits++ = low_digit | ((high_digit << digit_lshift) & BIGDIGIT_MASK);
          low_digit = high_digit >> digit_rshift;
        } /* for */
        if (IS_NEGATIVE(high_digit)) {
          *dest_digits = low_digit | ((BIGDIGIT_MASK << digit_lshift) & BIGDIGIT_MASK);
          if (*dest_digits == BIGDIGIT_MASK) {
            if (size_reduction == 0) {
              *big_variable = normalize(big1);
            } else {
              pos = big1->size - size_reduction;
              if (pos >= 2 && IS_NEGATIVE(big1->bigdigits[pos - 2])) {
                pos--;
              } /* if */
              big1 = REALLOC_BIG(big1, big1->size, pos);
              if (big1 == NULL) {
                raise_error(MEMORY_ERROR);
              } else {
                COUNT3_BIG(big1->size, pos);
                big1->size = pos;
                *big_variable = big1;
              } /* if */
              size_reduction = 0;
            } /* if */
          } /* if */
        } else {
          *dest_digits = low_digit;
          if (low_digit == 0) {
            if (size_reduction == 0) {
              *big_variable = normalize(big1);
            } else {
              pos = big1->size - size_reduction;
              if (pos >= 2 && !IS_NEGATIVE(big1->bigdigits[pos - 2])) {
                pos--;
              } /* if */
              big1 = REALLOC_BIG(big1, big1->size, pos);
              if (big1 == NULL) {
                raise_error(MEMORY_ERROR);
              } else {
                COUNT3_BIG(big1->size, pos);
                big1->size = pos;
                *big_variable = big1;
              } /* if */
              size_reduction = 0;
            } /* if */
          } /* if */
        } /* if */
        if (size_reduction != 0) {
          big1 = REALLOC_BIG(big1, big1->size, big1->size - size_reduction);
          if (big1 == NULL) {
            *big_variable = NULL;
            raise_error(MEMORY_ERROR);
          } else {
            COUNT3_BIG(big1->size, big1->size - size_reduction);
            big1->size -= size_reduction;
            *big_variable = big1;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* bigRShiftAssign */



/**
 *  Returns the difference of two signed big integers.
 */
#ifdef ANSI_C

rtlBiginttype bigSbtr (const const_rtlBiginttype big1, const const_rtlBiginttype big2)
#else

rtlBiginttype bigSbtr (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;
    doublebigdigittype big1_sign;
    doublebigdigittype big2_sign;
    rtlBiginttype result;

  /* bigSbtr */
    if (big1->size >= big2->size) {
      if (!ALLOC_BIG(result, big1->size + 1)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        pos = 0;
        do {
          carry += (doublebigdigittype) big1->bigdigits[pos] +
              (~big2->bigdigits[pos] & BIGDIGIT_MASK);
          result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
          pos++;
        } while (pos < big2->size);
        big2_sign = IS_NEGATIVE(big2->bigdigits[pos - 1]) ? 0 : BIGDIGIT_MASK;
        for (; pos < big1->size; pos++) {
          carry += big1->bigdigits[pos] + big2_sign;
          result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        if (IS_NEGATIVE(big1->bigdigits[pos - 1])) {
          big2_sign--;
        } /* if */
        result->bigdigits[pos] = (bigdigittype) ((carry + big2_sign) & BIGDIGIT_MASK);
        result->size = pos + 1;
        result = normalize(result);
        return(result);
      } /* if */
    } else {
      if (!ALLOC_BIG(result, big2->size + 1)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        pos = 0;
        do {
          carry += (doublebigdigittype) big1->bigdigits[pos] +
              (~big2->bigdigits[pos] & BIGDIGIT_MASK);
          result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
          pos++;
        } while (pos < big1->size);
        big1_sign = IS_NEGATIVE(big1->bigdigits[pos - 1]) ? BIGDIGIT_MASK : 0;
        for (; pos < big2->size; pos++) {
          carry += big1_sign + (~big2->bigdigits[pos] & BIGDIGIT_MASK);
          result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        big2_sign = IS_NEGATIVE(big2->bigdigits[pos - 1]) ? 0 : BIGDIGIT_MASK;
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          big2_sign--;
        } /* if */
        result->bigdigits[pos] = (bigdigittype) ((carry + big2_sign) & BIGDIGIT_MASK);
        result->size = pos + 1;
        result = normalize(result);
        return(result);
      } /* if */
    } /* if */
  } /* bigSbtr */



/**
 *  Returns the difference of two signed big integers.
 *  Big1 is assumed to be a temporary value which is reused.
 */
#ifdef ANSI_C

rtlBiginttype bigSbtrTemp (rtlBiginttype big1, const_rtlBiginttype big2)
#else

rtlBiginttype bigSbtrTemp  (big1, big2)
rtlBiginttype big1;
rtlBiginttype big2;
#endif

  { /* bigSbtrTemp */
    bigShrink(&big1, big2);
    return(big1);
  } /* bigSbtrTemp */



/**
 *  Subtracts big2 from *big_variable. The operation is done in
 *  place and *big_variable is only resized when necessary.
 *  When the size of big2 is smaller than *big_variable the
 *  algorithm tries to save computations. Therefore there are
 *  checks for carry != 0 and carry == 0.
 *  In case the resizing fails the content of *big_variable
 *  is freed and *big_variable is set to NULL.
 */
#ifdef ANSI_C

void bigShrink (rtlBiginttype *const big_variable, const const_rtlBiginttype big2)
#else

void bigShrink (big_variable, big2)
rtlBiginttype *big_variable;
rtlBiginttype big2;
#endif

  {
    rtlBiginttype big1;
    memsizetype pos;
    doublebigdigittype carry = 1;
    doublebigdigittype big1_sign;
    doublebigdigittype big2_sign;
    rtlBiginttype resized_big1;

  /* bigShrink */
    big1 = *big_variable;
    if (big1->size >= big2->size) {
      big1_sign = IS_NEGATIVE(big1->bigdigits[big1->size - 1]) ? BIGDIGIT_MASK : 0;
      pos = 0;
      do {
        carry += (doublebigdigittype) big1->bigdigits[pos] +
            (~big2->bigdigits[pos] & BIGDIGIT_MASK);
        big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        pos++;
      } while (pos < big2->size);
      if (IS_NEGATIVE(big2->bigdigits[pos - 1])) {
        for (; carry != 0 && pos < big1->size; pos++) {
          carry += big1->bigdigits[pos];
          big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
      } else {
        for (; carry == 0 && pos < big1->size; pos++) {
          carry = (doublebigdigittype) big1->bigdigits[pos] + BIGDIGIT_MASK;
          big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        carry += BIGDIGIT_MASK;
      } /* if */
      pos = big1->size;
      carry += big1_sign;
      carry &= BIGDIGIT_MASK;
      if ((carry != 0 || IS_NEGATIVE(big1->bigdigits[pos - 1])) &&
          (carry != BIGDIGIT_MASK || !IS_NEGATIVE(big1->bigdigits[pos - 1]))) {
        resized_big1 = REALLOC_BIG(big1, big1->size, big1->size + 1);
        if (resized_big1 == NULL) {
          FREE_BIG(big1, big1->size);
          *big_variable = NULL;
          raise_error(MEMORY_ERROR);
        } else {
          big1 = resized_big1;
          COUNT3_BIG(big1->size, big1->size + 1);
          big1->size++;
          big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          *big_variable = big1;
        } /* if */
      } else {
        *big_variable = normalize(big1);
      } /* if */
    } else {
      resized_big1 = REALLOC_BIG(big1, big1->size, big2->size + 1);
      if (resized_big1 == NULL) {
        FREE_BIG(big1, big1->size);
        *big_variable = NULL;
        raise_error(MEMORY_ERROR);
      } else {
        big1 = resized_big1;
        COUNT3_BIG(big1->size, big2->size + 1);
        big1_sign = IS_NEGATIVE(big1->bigdigits[big1->size - 1]) ? BIGDIGIT_MASK : 0;
        pos = 0;
        do {
          carry += (doublebigdigittype) big1->bigdigits[pos] +
              (~big2->bigdigits[pos] & BIGDIGIT_MASK);
          big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
          pos++;
        } while (pos < big1->size);
        big2_sign = IS_NEGATIVE(big2->bigdigits[big2->size - 1]) ? 0 : BIGDIGIT_MASK;
        for (; pos < big2->size; pos++) {
          carry += big1_sign + (~big2->bigdigits[pos] & BIGDIGIT_MASK);
          big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        carry += big1_sign + big2_sign;
        big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        big1->size = pos + 1;
        *big_variable = normalize(big1);
      } /* if */
    } /* if */
  } /* bigShrink */



#ifdef ANSI_C

stritype bigStr (const const_rtlBiginttype big1)
#else

stritype bigStr (big1)
rtlBiginttype big1;
#endif

  {
    rtlBiginttype help_big;
    memsizetype pos;
    bigdigittype digit;
    int digit_pos;
    strelemtype digit_ch;
    memsizetype result_size;
    stritype resized_result;
    stritype result;

  /* bigStr */
    result_size = 256;
    if (!ALLOC_STRI(result, result_size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      if (!ALLOC_BIG(help_big, big1->size + 1)) {
        FREE_STRI(result, result_size);
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        pos = 0;
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          positive_copy_of_negative_big(help_big, big1);
          result->mem[pos] = '-';
          pos++;
        } else {
          help_big->size = big1->size;
          memcpy(help_big->bigdigits, big1->bigdigits,
              (SIZE_TYPE) big1->size * sizeof(bigdigittype));
        } /* if */
        do {
          if (pos + DECIMAL_DIGITS_IN_BIGDIGIT > result_size) {
            REALLOC_STRI(resized_result, result, result_size, result_size + 256);
            if (resized_result == NULL) {
              FREE_STRI(result, result_size);
              FREE_BIG(help_big, big1->size + 1);
              raise_error(MEMORY_ERROR);
              return(NULL);
            } else {
              result = resized_result;
              COUNT3_STRI(result_size, result_size + 256);
              result_size += 256;
            } /* if */
          } /* if */
          digit = uBigDivideByPowerOf10(help_big);
          if (help_big->bigdigits[help_big->size - 1] == 0) {
            help_big->size--;
          } /* if */
          if (help_big->size > 1 || help_big->bigdigits[0] != 0) {
            for (digit_pos = DECIMAL_DIGITS_IN_BIGDIGIT;
                digit_pos != 0; digit_pos--) {
              result->mem[pos] = '0' + digit % 10;
              digit /= 10;
              pos++;
            } /* for */
          } else {
            do {
              result->mem[pos] = '0' + digit % 10;
              digit /= 10;
              pos++;
            } while (digit != 0);
          } /* if */
        } while (help_big->size > 1 || help_big->bigdigits[0] != 0);
        FREE_BIG(help_big, big1->size + 1);
        result->size = pos;
        REALLOC_STRI(resized_result, result, result_size, pos);
        if (resized_result == NULL) {
          FREE_STRI(result, result_size);
          raise_error(MEMORY_ERROR);
          return(NULL);
        } else {
          result = resized_result;
          COUNT3_STRI(result_size, pos);
          if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
            for (pos = 1; pos <= result->size >> 1; pos++) {
              digit_ch = result->mem[pos];
              result->mem[pos] = result->mem[result->size - pos];
              result->mem[result->size - pos] = digit_ch;
            } /* for */
          } else {
            for (pos = 0; pos < result->size >> 1; pos++) {
              digit_ch = result->mem[pos];
              result->mem[pos] = result->mem[result->size - pos - 1];
              result->mem[result->size - pos - 1] = digit_ch;
            } /* for */
          } /* if */
          return(result);
        } /* if */
      } /* if */
    } /* if */
  } /* bigStr */



#ifdef ANSI_C

rtlBiginttype bigSucc (const const_rtlBiginttype big1)
#else

rtlBiginttype bigSucc (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;
    rtlBiginttype result;

  /* bigSucc */
    if (!ALLOC_BIG(result, big1->size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      result->size = big1->size;
      pos = 0;
      do {
        carry += big1->bigdigits[pos];
        result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
        pos++;
      } while (pos < big1->size);
      if (IS_NEGATIVE(result->bigdigits[pos - 1])) {
        if (!IS_NEGATIVE(big1->bigdigits[pos - 1])) {
          result = REALLOC_BIG(result, pos, pos + 1);
          if (result == NULL) {
            raise_error(MEMORY_ERROR);
            return(NULL);
          } else {
            COUNT3_BIG(pos, pos + 1);
            result->size++;
            result->bigdigits[pos] = 0;
          } /* if */
        } else if (result->bigdigits[pos - 1] == BIGDIGIT_MASK &&
            pos >= 2 && IS_NEGATIVE(result->bigdigits[pos - 2])) {
          result = REALLOC_BIG(result, pos, pos - 1);
          if (result == NULL) {
            raise_error(MEMORY_ERROR);
            return(NULL);
          } else {
            COUNT3_BIG(pos, pos - 1);
            result->size--;
          } /* if */
        } /* if */
      } /* if */
      return(result);
    } /* if */
  } /* bigSucc */



#ifdef ANSI_C

bstritype bigToBStri (const_rtlBiginttype big1)
#else

bstritype bigToBStri (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    int byteNum;
    bigdigittype digit;
    memsizetype charIndex;
    memsizetype result_size;
    bstritype result;

  /* bigToBStri */
    result_size = big1->size * sizeof(bigdigittype);
    digit = big1->bigdigits[big1->size - 1];
    byteNum = sizeof(bigdigittype) - 1;
    if (IS_NEGATIVE(digit)) {
      while (byteNum > 0 && (digit >> byteNum * 8 & 0xFF) == 0xFF) {
        result_size--;
        byteNum--;
      } /* while */
      if (byteNum < 3 && (digit >> byteNum * 8 & 0xFF) <= 127) {
        result_size++;
        byteNum++;
      } /* if */
    } else {
      while (byteNum > 0 && (digit >> byteNum * 8 & 0xFF) == 0) {
        result_size--;
        byteNum--;
      } /* while */
      if (byteNum < 3 && (digit >> byteNum * 8 & 0xFF) >= 128) {
        result_size++;
        byteNum++;
      } /* if */
    } /* if */
    if (!ALLOC_BSTRI(result, result_size)) {
      raise_error(MEMORY_ERROR);
    } else {
      result->size = result_size;
      charIndex = 0;
      pos = big1->size;
      while (pos > 0) {
        pos--;
        digit = big1->bigdigits[pos];
        for (byteNum = sizeof(bigdigittype) - 1; byteNum >= 0; byteNum--) {
          if (pos * sizeof(bigdigittype) + byteNum < result_size) {
            result->mem[charIndex] = digit >> byteNum * 8 & 0xFF;
            charIndex++;
          } /* if */
        } /* for */
      } /* for */
    } /* if */
    return(result);
  } /* bigToBStri */



#ifdef ANSI_C

int32type bigToInt32 (const const_rtlBiginttype big1)
#else

int32type bigToInt32 (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    int32type result;

  /* bigToInt32 */
    if (big1->size > sizeof(int32type) / sizeof(bigdigittype)) {
      raise_error(RANGE_ERROR);
      return(0);
    } else {
      pos = big1->size - 1;
      result = big1->bigdigits[pos];
      while (pos > 0) {
        pos--;
        result <<= 8 * sizeof(bigdigittype);
        result |= (int32type) big1->bigdigits[pos];
      } /* while */
      return(result);
    } /* if */
  } /* bigToInt32 */



#ifdef INT64TYPE
#ifdef ANSI_C

int64type bigToInt64 (const const_rtlBiginttype big1)
#else

int64type bigToInt64 (big1)
rtlBiginttype big1;
#endif

  {
    memsizetype pos;
    int64type result;

  /* bigToInt64 */
    if (big1->size > sizeof(int64type) / sizeof(bigdigittype)) {
      raise_error(RANGE_ERROR);
      return(0);
    } else {
      pos = big1->size - 1;
      result = big1->bigdigits[pos];
      while (pos > 0) {
        pos--;
        result <<= 8 * sizeof(bigdigittype);
        result |= (int64type) big1->bigdigits[pos];
      } /* while */
      return(result);
    } /* if */
  } /* bigToInt64 */
#endif
