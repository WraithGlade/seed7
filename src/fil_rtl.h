/********************************************************************/
/*                                                                  */
/*  fil_rtl.h     Primitive actions for the C library file type.    */
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
/*  File: seed7/src/fil_rtl.h                                       */
/*  Changes: 1992, 1993, 1994, 2009  Thomas Mertes                  */
/*  Content: Primitive actions for the C library file type.         */
/*                                                                  */
/********************************************************************/

/* The posix macros S_ISLNK, S_ISSOCK, S_ISCHR, S_ISBLK, S_ISFIFO,  */
/* S_ISDIR, S_ISREG, S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP,   */
/* S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH, S_IRWXU, S_IRWXG and S_IRWXO */
/* are used to check the 'st_mode' field of 'struct stat'. The      */
/* 'struct stat' and its variants is used by the stat() functions   */
/* (e.g. stat(), fstat(), _stat(), _stat64(), _stati64(), _wstat(), */
/* _wstat64(), _wstati64() ). The macros are defined in the case    */
/* when they are missing in "sys/stat.h".                           */

#ifndef S_ISLNK
#ifdef S_IFLNK
#define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)
#else
#define S_ISLNK(mode) FALSE
#endif
#endif

#ifndef S_ISSOCK
#ifdef S_IFSOCK
#define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)
#else
#define S_ISSOCK(mode) FALSE
#endif
#endif

#ifndef S_ISCHR
#ifdef S_IFCHR
#define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)
#else
#define S_ISCHR(mode) FALSE
#endif
#endif

#ifndef S_ISBLK
#ifdef S_IFBLK
#define S_ISBLK(mode) (((mode) & S_IFMT) == S_IFBLK)
#else
#define S_ISBLK(mode) FALSE
#endif
#endif

#ifndef S_ISFIFO
#ifdef S_IFFIFO
#define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFFIFO)
#else
#define S_ISFIFO(mode) FALSE
#endif
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

#ifndef S_IRUSR
#ifdef S_IREAD
#define S_IRUSR S_IREAD
#else
#define S_IRUSR 0000400
#endif
#endif

#ifndef S_IWUSR
#ifdef S_IREAD
#define S_IWUSR S_IWRITE
#else
#define S_IWUSR 0000200
#endif
#endif

#ifndef S_IXUSR
#ifdef S_IREAD
#define S_IXUSR S_IEXEC
#else
#define S_IXUSR 0000100
#endif
#endif

#ifndef S_IRGRP
#define S_IRGRP 0000040
#endif

#ifndef S_IWGRP
#define S_IWGRP 0000020
#endif

#ifndef S_IXGRP
#define S_IXGRP 0000010
#endif

#ifndef S_IROTH
#define S_IROTH 0000004
#endif

#ifndef S_IWOTH
#define S_IWOTH 0000002
#endif

#ifndef S_IXOTH
#define S_IXOTH 0000001
#endif

#ifndef S_IRWXU
#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#endif

#ifndef S_IRWXG
#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
#endif

#ifndef S_IRWXO
#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#endif


int offsetSeek (filetype aFile, const os_off_t anOffset, const int origin);
memsizetype remainingBytesInFile (filetype aFile);
inttype getFileLengthUsingSeek (filetype aFile);
biginttype getBigFileLengthUsingSeek (filetype aFile);
biginttype filBigLng (filetype aFile);
void filBigSeek (filetype aFile, const const_biginttype big_position);
biginttype filBigTell (filetype aFile);
void filClose (filetype aFile);
stritype filGets (filetype inFile, inttype length);
booltype filHasNext (filetype inFile);
stritype filLineRead (filetype inFile, chartype *terminationChar);
stritype filLit (filetype aFile);
inttype filLng (filetype aFile);
filetype filOpen (const const_stritype path, const const_stritype mode);
void filPclose (filetype aFile);
filetype filPopen (const const_stritype command,
    const const_stritype parameters, const const_stritype mode);
void filPrint (const const_stritype stri);
void filSeek (filetype aFile, inttype file_position);
void filSetbuf (filetype aFile, inttype mode, inttype size);
inttype filTell (filetype aFile);
stritype filWordRead (filetype inFile, chartype *terminationChar);
void filWrite (filetype outFile, const const_stritype stri);
