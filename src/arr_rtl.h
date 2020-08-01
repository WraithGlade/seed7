/********************************************************************/
/*                                                                  */
/*  arr_rtl.h     Primitive actions for the array type.             */
/*  Copyright (C) 1989 - 2006  Thomas Mertes                        */
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
/*  File: seed7/src/arr_rtl.h                                       */
/*  Changes: 1991, 1992, 1993, 1994, 2005, 2006  Thomas Mertes      */
/*  Content: Primitive actions for the array type.                  */
/*                                                                  */
/********************************************************************/

#ifdef USE_WMAIN
rtlArraytype getArgv (const int argc, const wstritype *const argv, stritype *arg_0,
    stritype *programName, stritype *exePath);
#else
rtlArraytype getArgv (const int argc, const cstritype *const argv, stritype *arg_0,
    stritype *programName, stritype *exePath);
#endif
stritype examineSearchPath (const const_stritype fileName);
void arrAppend (rtlArraytype *const arr_variable, const rtlArraytype arr_from);
rtlArraytype arrArrlit2 (inttype start_position, rtlArraytype arr1);
rtlArraytype arrBaselit (const rtlGenerictype element);
rtlArraytype arrBaselit2 (inttype start_position, const rtlGenerictype element);
rtlArraytype arrCat (rtlArraytype arr1, const rtlArraytype arr2);
rtlArraytype arrExtend (rtlArraytype arr1, const rtlGenerictype element);
void arrFree (rtlArraytype oldArray);
rtlArraytype arrGen (const rtlGenerictype element1, const rtlGenerictype element2);
rtlArraytype arrHead (const const_rtlArraytype arr1, inttype stop);
rtlArraytype arrHeadTemp (rtlArraytype *arr_temp, inttype stop);
rtlGenerictype arrIdxTemp (rtlArraytype *arr_temp, inttype pos);
rtlArraytype arrMalloc (inttype min_position, inttype max_position);
void arrPush (rtlArraytype *const arr_variable, const rtlGenerictype element);
rtlArraytype arrRange (const const_rtlArraytype arr1, inttype start, inttype stop);
rtlArraytype arrRangeTemp (rtlArraytype *arr_temp, inttype start, inttype stop);
rtlArraytype arrRealloc (rtlArraytype arr, memsizetype oldSize, memsizetype newSize);
rtlGenerictype arrRemove (rtlArraytype *arr_to, inttype position);
rtlArraytype arrSort (rtlArraytype arr1, inttype cmp_func (rtlGenerictype, rtlGenerictype));
rtlArraytype arrSubarr (const const_rtlArraytype arr1, inttype start, inttype len);
rtlArraytype arrSubarrTemp (rtlArraytype *arr_temp, inttype start, inttype len);
rtlArraytype arrTail (const const_rtlArraytype arr1, inttype start);
rtlArraytype arrTailTemp (rtlArraytype *arr_temp, inttype start);
