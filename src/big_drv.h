/********************************************************************/
/*                                                                  */
/*  big_drv.h     Driver functions for the bigInteger type.         */
/*  Copyright (C) 1989 - 2010  Thomas Mertes                        */
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
/*  File: seed7/src/big_drv.h                                       */
/*  Changes: 2005, 2006, 2008, 2009, 2010  Thomas Mertes            */
/*  Content: Driver functions for the bigInteger type.              */
/*                                                                  */
/********************************************************************/

#ifndef INT64TYPE
#define bigFromUInt64(x) 0
#define bigToInt64(x) 0
#endif

#if   INTTYPE_SIZE == 64
#define bigIConv bigFromInt64
#define bigOrd   bigToInt64
#elif INTTYPE_SIZE == 32
#define bigIConv bigFromInt32
#define bigOrd   bigToInt32
#endif


#ifdef ANSI_C

cstritype bigHexCStri (const const_biginttype big1);
biginttype bigAbs (const const_biginttype big1);
biginttype bigAdd (const_biginttype summand1, const_biginttype summand2);
biginttype bigAddTemp (biginttype big1, const const_biginttype big2);
inttype bigBitLength (const const_biginttype big1);
stritype bigCLit (const const_biginttype big1);
inttype bigCmp (const const_biginttype big1, const const_biginttype big2);
inttype bigCmpSignedDigit (const const_biginttype big1, inttype number);
void bigCpy (biginttype *const big_to, const const_biginttype big_from);
biginttype bigCreate (const const_biginttype big_from);
void bigDecr (biginttype *const big_variable);
void bigDestr (const const_biginttype old_bigint);
biginttype bigDiv (const const_biginttype dividend, const const_biginttype divisor);
booltype bigEq (const const_biginttype big1, const const_biginttype big2);
booltype bigEqSignedDigit (const const_biginttype big1, inttype number);
biginttype bigFromInt32 (int32type number);
#ifdef INT64TYPE
  biginttype bigFromInt64 (int64type number);
#endif
biginttype bigFromUInt32 (uint32type number);
#ifdef INT64TYPE
  biginttype bigFromUInt64 (uint64type number);
#endif
biginttype bigGcd (const const_biginttype big1, const const_biginttype big2);
void bigGrow (biginttype *const big_variable, const const_biginttype big2);
inttype bigHashCode (const const_biginttype big1);
biginttype bigImport (const const_ustritype buffer);
void bigIncr (biginttype *const big_variable);
biginttype bigIPow (const const_biginttype base, inttype exponent);
biginttype bigLog2 (const const_biginttype big1);
inttype bigLowestSetBit (const const_biginttype big1);
biginttype bigLShift (const const_biginttype big1, const inttype lshift);
void bigLShiftAssign (biginttype *const big_variable, inttype lshift);
biginttype bigLShiftOne (const inttype lshift);
biginttype bigMDiv (const const_biginttype dividend, const const_biginttype divisor);
biginttype bigMinus (const const_biginttype big1);
biginttype bigMod (const const_biginttype dividend, const const_biginttype divisor);
biginttype bigMult (const_biginttype factor1, const_biginttype factor2);
void bigMultAssign (biginttype *const big_variable, const_biginttype big2);
biginttype bigMultSignedDigit (const_biginttype big1, inttype number);
booltype bigOdd (const const_biginttype big1);
biginttype bigParse (const const_stritype stri);
biginttype bigParseBased (const const_stritype stri, inttype base);
biginttype bigPred (const const_biginttype big1);
biginttype bigPredTemp (biginttype big1);
biginttype bigSquare (const_biginttype big1);
biginttype bigRand (const const_biginttype lower_limit, const const_biginttype upper_limit);
biginttype bigRem (const const_biginttype dividend, const const_biginttype divisor);
biginttype bigRShift (const const_biginttype big1, const inttype rshift);
void bigRShiftAssign (biginttype *const big_variable, inttype rshift);
biginttype bigSbtr (const const_biginttype minuend, const const_biginttype subtrahend);
biginttype bigSbtrTemp (biginttype big1, const_biginttype big2);
void bigShrink (biginttype *const big_variable, const const_biginttype big2);
stritype bigStr (const const_biginttype big1);
biginttype bigSucc (const const_biginttype big1);
biginttype bigSuccTemp (biginttype big1);
int32type bigToInt32 (const const_biginttype big1);
#ifdef INT64TYPE
  int64type bigToInt64 (const const_biginttype big1);
#endif
biginttype bigZero (void);

#else

cstritype bigHexCStri ();
biginttype bigAbs ();
biginttype bigAdd ();
biginttype bigAddTemp ();
inttype bigBitLength ();
stritype bigCLit ();
inttype bigCmp ();
inttype bigCmpSignedDigit ();
void bigCpy ();
biginttype bigCreate ();
void bigDecr ();
void bigDestr ();
biginttype bigDiv ();
booltype bigEq ();
booltype bigEqSignedDigit ();
biginttype bigFromInt32 ();
#ifdef INT64TYPE
  biginttype bigFromInt64 ();
#endif
biginttype bigFromUInt32 ();
#ifdef INT64TYPE
  biginttype bigFromUInt64 ();
#endif
biginttype bigGcd ();
void bigGrow ();
inttype bigHashCode ();
biginttype bigImport ();
void bigIncr ();
biginttype bigIPow ();
biginttype bigLog2 ();
inttype bigLowestSetBit ();
biginttype bigLShift ();
void bigLShiftAssign ();
biginttype bigLShiftOne ();
biginttype bigMDiv ();
biginttype bigMinus ();
biginttype bigMod ();
biginttype bigMult ();
void bigMultAssign ();
biginttype bigMultSignedDigit ();
booltype bigOdd ();
biginttype bigParse ();
biginttype bigParseBased ();
biginttype bigPred ();
biginttype bigPredTemp ();
biginttype bigSquare ();
biginttype bigRand ();
biginttype bigRem ();
biginttype bigRShift ();
void bigRShiftAssign ();
biginttype bigSbtr ();
biginttype bigSbtrTemp ();
void bigShrink ();
stritype bigStr ();
biginttype bigSucc ();
biginttype bigSuccTemp ();
int32type bigToInt32 ();
#ifdef INT64TYPE
  int64type bigToInt64 ();
#endif
biginttype bigZero ();

#endif
