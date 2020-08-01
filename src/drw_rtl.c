/********************************************************************/
/*                                                                  */
/*  drw_rtl.c     Generic graphic drawing functions.                */
/*  Copyright (C) 1989 - 2007  Thomas Mertes                        */
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
/*  File: seed7/src/drw_rtl.c                                       */
/*  Changes: 2007  Thomas Mertes                                    */
/*  Content: Generic graphic drawing functions.                     */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdio.h"

#include "common.h"
#include "drw_drv.h"

#undef EXTERN
#define EXTERN
#include "drw_rtl.h"


#define NO_PTR

#ifdef ANSI_C

void drwCpy (wintype *win_to, wintype win_from)
#else

void drwCpy (win_to, win_from)
wintype *win_to;
wintype win_from;
#endif

  { /* drwCpy */
#ifdef TRACE_DRW
    printf("drwCpy(%lu, %ld)\n", win_to, win_from);
#endif
    if (*win_to != NULL) {
      (*win_to)->usage_count--;
      if ((*win_to)->usage_count == 0) {
        drwFree(*win_to);
      } /* if */
    } /* if */
    *win_to = win_from;
    if (win_from != NULL) {
      win_from->usage_count++;
    } /* if */
  } /* drwCpy */



#ifdef ANSI_C

wintype drwCreate (wintype win_from)
#else

wintype drwCreate (win_from)
wintype win_from;
#endif

  { /* drwCreate */
    if (win_from != NULL) {
      win_from->usage_count++;
    } /* if */
    return(win_from);
  } /* drwCreate */



#ifdef ANSI_C

void drwDestr (wintype old_win)
#else

void drwDestr (old_win)
wintype old_win;
#endif

  { /* drwDestr */
    if (old_win != NULL) {
      old_win->usage_count--;
      if (old_win->usage_count == 0) {
        drwFree(old_win);
      } /* if */
    } /* if */
  } /* drwDestr */
