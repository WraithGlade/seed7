/********************************************************************/
/*                                                                  */
/*  s7   Seed7 interpreter                                          */
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
/*  Module: General                                                 */
/*  File: seed7/src/identutl.h                                      */
/*  Changes: 1991, 1992, 1993, 1994  Thomas Mertes                  */
/*  Content: Procedures to maintain objects of type identtype.      */
/*                                                                  */
/********************************************************************/

/* #define IDENT_TABLE(STRI, LEN) prog.ident.table[((STRI[0] & 63) << 4) | (LEN & 15)] */
/* #define IDENT_TABLE(STRI, LEN) prog.ident.table[((STRI[0] << 4) | LEN) & (ID_TABLE_SIZE - 1)] */
/* #define IDENT_TABLE(STRI, LEN) prog.ident.table[((STRI[0] << 4) ^ (STRI[1] << 2) ^ LEN) & (ID_TABLE_SIZE - 1)] */
#define IDENT_TABLE(STRI, LEN) prog.ident.table[((STRI[0] << 4) ^ (STRI[LEN - 1] << 2) ^ (int) LEN) & (ID_TABLE_SIZE - 1)]


identtype new_ident (const_ustritype name, sysizetype length);
identtype get_ident (const_ustritype name);
void close_idents (const_progtype currentProg);
void init_idents (progtype currentProg, errinfotype *err_info);
