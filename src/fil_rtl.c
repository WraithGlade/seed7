/********************************************************************/
/*                                                                  */
/*  fil_rtl.c     Primitive actions for the C library file type.    */
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
/*  File: seed7/src/fil_rtl.c                                       */
/*  Changes: 1992, 1993, 1994, 2009, 2013  Thomas Mertes            */
/*  Content: Primitive actions for the C library file type.         */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "sys/types.h"
#include "sys/stat.h"

#ifdef UNISTD_H_PRESENT
#include "unistd.h"
#endif

/* #include "errno.h" */

#include "common.h"
#include "os_decls.h"
#include "heaputl.h"
#include "striutl.h"
#include "big_drv.h"
#include "rtl_err.h"

#undef EXTERN
#define EXTERN
#include "fil_rtl.h"


#ifdef C_PLUS_PLUS
#define C "C"
#else
#define C
#endif

#ifdef DEFINE_FSEEKI64_PROTOTYPE
extern C int __cdecl _fseeki64(FILE *, __int64, int);
#endif

#ifdef DEFINE_FTELLI64_PROTOTYPE
extern C __int64 __cdecl _ftelli64(FILE *);
#endif

#ifdef DEFINE_WPOPEN_PROTOTYPE
extern C FILE *_wpopen (const wchar_t *, const wchar_t *);
#endif


#define BUFFER_SIZE             4096
#define GETS_DEFAULT_SIZE    1048576
#define GETS_STRI_SIZE_DELTA    4096
#define READ_STRI_INIT_SIZE      256
#define READ_STRI_SIZE_DELTA    2048



/**
 *  Translate a Seed7 file open mode to a C file open mode.
 *  The following modes are accepted
 *   Seed7 mode | C mode | Comment
 *   "r"        | "rb"   | Open file for reading.
 *   "w"        | "rw"   | Truncate to zero length or create file for writing.
 *   "a"        | "ra"   | Append; open or create file for writing at end-of-file.
 *   "r+"       | "rb+"  | Open file for update (reading and writing).
 *   "w+"       | "rw+"  | Truncate to zero length or create file for update.
 *   "a+"       | "ra+"  | Append; open or create file for update, writing at end-of-file.
 *   "rt"       | "r"    | Open file for reading.
 *   "wt"       | "w"    | Truncate to zero length or create file for writing.
 *   "at"       | "a"    | Append; open or create file for writing at end-of-file.
 *   "rt+"      | "r+"   | Open file for update (reading and writing).
 *   "wt+"      | "w+"   | Truncate to zero length or create file for update.
 *   "at+"      | "q+"   | Append; open or create file for update, writing at end-of-file.
 *  Other Seed7 modes correspond to the C mode "".
 *  The Seed7 modes with t are text modes and the modes
 *  without t are binary modes.
 */
static void get_mode (os_chartype os_mode[4], const const_stritype file_mode)

  { /* get_mode */
    os_mode[0] = '\0';
    if (file_mode->size >= 1 &&
        (file_mode->mem[0] == 'r' ||
         file_mode->mem[0] == 'w' ||
         file_mode->mem[0] == 'a')) {
      if (file_mode->size == 1) {
        /* Binary mode
           r ... Open file for reading.
           w ... Truncate to zero length or create file for writing.
           a ... Append; open or create file for writing at end-of-file.
        */
        os_mode[0] = (os_chartype) file_mode->mem[0];
        os_mode[1] = 'b';
        os_mode[2] = '\0';
      } else if (file_mode->size == 2) {
        if (file_mode->mem[1] == '+') {
          /* Binary mode
             r+ ... Open file for update (reading and writing).
             w+ ... Truncate to zero length or create file for update.
             a+ ... Append; open or create file for update, writing at end-of-file.
          */
          os_mode[0] = (os_chartype) file_mode->mem[0];
          os_mode[1] = 'b';
          os_mode[2] = '+';
          os_mode[3] = '\0';
        } else if (file_mode->mem[1] == 't') {
          /* Text mode
             rt ... Open file for reading.
             wt ... Truncate to zero length or create file for writing.
             at ... Append; open or create file for writing at end-of-file.
          */
          os_mode[0] = (os_chartype) file_mode->mem[0];
          os_mode[1] = '\0';
        } /* if */
      } else if (file_mode->size == 3) {
        if (file_mode->mem[1] == 't' &&
            file_mode->mem[2] == '+') {
          /* Text mode
             rt+ ... Open file for update (reading and writing).
             wt+ ... Truncate to zero length or create file for update.
             at+ ... Append; open or create file for update, writing at end-of-file.
          */
          os_mode[0] = (os_chartype) file_mode->mem[0];
          os_mode[1] = '+';
          os_mode[2] = '\0';
        } /* if */
      } /* if */
    } /* if */
  } /* get_mode */



/**
 *  Determine the length of a file by using a seek function.
 *  The file length is measured in bytes.
 *  The file position is moved to the end of the file and the
 *  end position is used as file length. Afterwards the file
 *  position is moved back to the previous position.
 *  This function returns an os_off_t result. The size of
 *  os_off_t might be different from the size of inttype.
 *  @return the length of the file or
 *          -1 when the length could not be obtained.
 */
static os_off_t seekFileLength (filetype aFile)

  {
    os_off_t file_length;

  /* seekFileLength */
#ifdef FTELL_WRONG_FOR_PIPE
    {
      int file_no;
      os_fstat_struct stat_buf;

      file_no = fileno(aFile);
      if (file_no == -1 || os_fstat(file_no, &stat_buf) != 0 ||
          !S_ISREG(stat_buf.st_mode)) {
        return (os_off_t) -1;
      } /* if */
    }
#endif
#if defined(os_fseek) && defined(os_ftell)
    {
      os_off_t current_file_position;

      current_file_position = os_ftell(aFile);
      if (current_file_position == (os_off_t) -1) {
        file_length = -1;
      } else if (os_fseek(aFile, (os_off_t) 0, SEEK_END) != 0) {
        file_length = -1;
      } else {
        file_length = os_ftell(aFile);
        if (os_fseek(aFile, current_file_position, SEEK_SET) != 0) {
          file_length = -1;
        } /* if */
      } /* if */
    }
#elif defined os_fsetpos && defined os_fgetpos
    {
      fpos_t current_file_pos;
      fpos_t file_pos;

      if (os_fgetpos(aFile, &current_file_pos) != 0) {
        file_length = -1;
      } else if (fseek(aFile, 0, SEEK_END) != 0) {
        file_length = -1;
      } else {
        if (os_fgetpos(aFile, &file_pos) != 0) {
          file_length = -1;
        } else {
          file_length = file_pos;
        } /* if */
        if (os_fsetpos(aFile, &current_file_pos) != 0) {
          file_length = -1;
        } /* if */
      } /* if */
    }
#else
    {
      int file_no;
      os_off_t current_file_position;

      file_no = fileno(aFile);
      if (file_no == -1) {
        file_length = -1;
      } else {
        fflush(aFile);
        current_file_position = os_lseek(file_no, (os_off_t) 0, SEEK_CUR);
        if (current_file_position == (os_off_t) -1) {
          file_length = -1;
        } else {
          file_length = os_lseek(file_no, (os_off_t) 0, SEEK_END);
          if (file_length != (os_off_t) -1) {
            if (os_lseek(file_no, current_file_position, SEEK_SET) == (os_off_t) -1) {
              file_length = -1;
            } /* if */
          } /* if */
        } /* if */
      } /* if */
    }
#endif
    return file_length;
  } /* seekFileLength */



/**
 *  Determine the current file position.
 *  The file position is measured in bytes.
 *  This function uses 0 as the position of the first byte in the file.
 *  The function returns an os_off_t result. The size of
 *  os_off_t might be different from the size of inttype.
 *  @return the current file position or
 *          -1 when the file position could not be obtained.
 */
static os_off_t offsetTell (filetype aFile)

  {
    os_off_t current_file_position;

  /* offsetTell */
#ifdef FTELL_WRONG_FOR_PIPE
    {
      int file_no;
      os_fstat_struct stat_buf;

      file_no = fileno(aFile);
      if (file_no == -1 || os_fstat(file_no, &stat_buf) != 0 ||
          !S_ISREG(stat_buf.st_mode)) {
        return (os_off_t) -1;
      } /* if */
    }
#endif
#ifdef os_ftell
    current_file_position = os_ftell(aFile);
#elif defined os_fgetpos
    {
      fpos_t file_pos;

      if (os_fgetpos(aFile, &file_pos) != 0) {
        current_file_position = -1;
      } else {
        current_file_position = file_pos;
      } /* if */
    }
#else
    {
      int file_no;

      file_no = fileno(aFile);
      if (file_no == -1) {
        current_file_position = -1;
      } else {
        fflush(aFile);
        current_file_position = os_lseek(file_no, (os_off_t) 0, SEEK_CUR);
      } /* if */
    }
#endif
    return current_file_position;
  } /* offsetTell */



/**
 *  Set the current file position.
 *  The file position is measured in bytes.
 *  This function uses 0 as the position of the first byte in the file.
 *  The parameter 'anOffset' uses the type os_off_t. The size of
 *  os_off_t might be different from the size of inttype.
 *  @return 0 upon successful completion or
 *          -1 when the file position could not be set.
 */
int offsetSeek (filetype aFile, const os_off_t anOffset, const int origin)

  {
    int result;

  /* offsetSeek */
    /* printf("offsetSeek(%d, " INT64TYPE_FORMAT ", %d)\n",
       fileno(aFile), (INT64TYPE) anOffset, origin); */
#ifdef OS_FSEEK_OFFSET_BITS
#if OS_FSEEK_OFFSET_BITS == 32
    if (anOffset > (os_off_t) INT32TYPE_MAX ||
        anOffset < (os_off_t) INT32TYPE_MIN) {
      return -1;
    } /* if */
#endif
#endif
#ifdef FTELL_WRONG_FOR_PIPE
    {
      int file_no;
      os_fstat_struct stat_buf;

      file_no = fileno(aFile);
      if (file_no == -1 || os_fstat(file_no, &stat_buf) != 0 ||
          !S_ISREG(stat_buf.st_mode)) {
        return -1;
      } /* if */
    }
#endif
#ifdef os_fseek
    result = os_fseek(aFile, anOffset, origin);
#elif defined os_fsetpos && defined os_fgetpos
    {
      fpos_t file_pos;

      switch (origin) {
        case SEEK_SET:
          file_pos = anOffset;
          break;
        case SEEK_CUR:
          if (os_fgetpos(aFile, &file_pos) != 0) {
            return -1;
          } else {
            file_pos += anOffset;
          } /* if */
          break;
        case SEEK_END:
          if (fseek(aFile, 0, SEEK_END) != 0) {
            return -1;
          } else if (os_fgetpos(aFile, &file_pos) != 0) {
            return -1;
          } else {
            file_pos += anOffset;
          } /* if */
          break;
      } /* switch */
      if (os_fsetpos(aFile, &file_pos) != 0) {
        result = -1;
      } else {
        result = 0;
      } /* if */
    }
#else
    {
      int file_no;

      file_no = fileno(aFile);
      if (file_no == -1) {
        result = -1;
      } else {
        fflush(aFile);
        if (os_lseek(file_no, anOffset, origin) == (os_off_t) -1) {
          result = -1;
        } else {
          result = 0;
        } /* if */
      } /* if */
    }
#endif
    return result;
  } /* offsetSeek */



/**
 *  Compute how many bytes can be read from the current position.
 *  Returns 0 when the current position or the file size cannot be
 *  determined or when the current position is beyond the filesize.
 *  Returns MAX_MEMSIZETYPE when the result does not fit into
 *  memsizetype.
 */
memsizetype remainingBytesInFile (filetype aFile)

  {
    int file_no;
    os_fstat_struct stat_buf;
    os_off_t file_length;
    os_off_t current_file_position;
    memsizetype result;

  /* remainingBytesInFile */
    current_file_position = offsetTell(aFile);
    if (current_file_position == (os_off_t) -1) {
      result = 0;
    } else {
      file_no = fileno(aFile);
      if (file_no != -1 && os_fstat(file_no, &stat_buf) == 0 &&
          S_ISREG(stat_buf.st_mode)) {
        file_length = stat_buf.st_size;
      } else {
        file_length = seekFileLength(aFile);
      } /* if */
      if (file_length == (os_off_t) -1) {
        result = 0;
      } else if (file_length < current_file_position) {
        result = 0;
      } else if ((unsigned_os_off_t) (file_length - current_file_position) >= MAX_MEMSIZETYPE) {
        result = MAX_MEMSIZETYPE;
      } else {
        result = (memsizetype) (file_length - current_file_position);
      } /* if */
    } /* if */
    return result;
  } /* remainingBytesInFile */



inttype getFileLengthUsingSeek (filetype aFile)

  {
    os_off_t file_length;
    inttype result;

  /* getFileLengthUsingSeek */
    file_length = seekFileLength(aFile);
    if (unlikely(file_length < (os_off_t) 0)) {
      /* printf("errno=%d\n", errno);
      printf("EBADF=%d  EINVAL=%d  ESPIPE=%d\n",
          EBADF, EINVAL, ESPIPE); */
      raise_error(FILE_ERROR);
      result = 0;
    } else if (unlikely(file_length > INTTYPE_MAX)) {
      raise_error(RANGE_ERROR);
      result = 0;
    } else {
      result = (inttype) file_length;
    } /* if */
    return result;
  } /* getFileLengthUsingSeek */



biginttype getBigFileLengthUsingSeek (filetype aFile)

  {
    os_off_t file_length;
    biginttype result;

  /* getBigFileLengthUsingSeek */
    file_length = seekFileLength(aFile);
    if (unlikely(file_length < (os_off_t) 0)) {
      /* printf("errno=%d\n", errno);
      printf("EBADF=%d  EINVAL=%d  ESPIPE=%d\n",
          EBADF, EINVAL, ESPIPE); */
      raise_error(FILE_ERROR);
      result = NULL;
    } else {
#if OS_OFF_T_SIZE == 32
      result = bigFromUInt32((uint32type) file_length);
#elif OS_OFF_T_SIZE == 64
      result = bigFromUInt64((uint64type) file_length);
#else
#error "sizeof(os_off_t) is neither 4 nor 8."
#endif
    } /* if */
    return result;
  } /* getBigFileLengthUsingSeek */



static memsizetype read_string (filetype inFile, stritype stri, errinfotype *err_info)

  {
    uchartype buffer[BUFFER_SIZE];
    memsizetype bytes_in_buffer = 1;
    memsizetype stri_pos;
    register uchartype *from;
    register strelemtype *to;
    register memsizetype number;

  /* read_string */
    /* printf("stri->size=%lu\n", stri->size); */
    stri_pos = 0;
    while (stri->size - stri_pos >= BUFFER_SIZE && bytes_in_buffer > 0 &&
        *err_info == OKAY_NO_ERROR) {
      /* printf("read_size=%ld\n", BUFFER_SIZE); */
      bytes_in_buffer = (memsizetype) fread(buffer, 1, BUFFER_SIZE, inFile);
      if (bytes_in_buffer == 0 && stri_pos == 0 && ferror(inFile)) {
        *err_info = FILE_ERROR;
      } else {
        /* printf("#A# bytes_in_buffer=%d stri_pos=%d\n",
            bytes_in_buffer, stri_pos); */
        from = buffer;
        to = &stri->mem[stri_pos];
        number = bytes_in_buffer;
        for (; number > 0; from++, to++, number--) {
          *to = *from;
        } /* for */
        stri_pos += bytes_in_buffer;
      } /* if */
    } /* while */
    if (stri->size > stri_pos && bytes_in_buffer > 0 &&
        *err_info == OKAY_NO_ERROR) {
      /* printf("read_size=%ld\n", stri->size - stri_pos); */
      bytes_in_buffer = (memsizetype) fread(buffer, 1, stri->size - stri_pos, inFile);
      if (bytes_in_buffer == 0 && stri_pos == 0 && ferror(inFile)) {
        *err_info = FILE_ERROR;
      } else {
        /* printf("#B# bytes_in_buffer=%d stri_pos=%d\n",
            bytes_in_buffer, stri_pos); */
        from = buffer;
        to = &stri->mem[stri_pos];
        number = bytes_in_buffer;
        for (; number > 0; from++, to++, number--) {
          *to = *from;
        } /* for */
        stri_pos += bytes_in_buffer;
      } /* if */
    } /* if */
    /* printf("stri_pos=%lu\n", stri_pos); */
    return stri_pos;
  } /* read_string */



static stritype read_and_alloc_stri (filetype inFile, memsizetype chars_missing,
    memsizetype *num_of_chars_read, errinfotype *err_info)

  {
    uchartype buffer[BUFFER_SIZE];
    memsizetype bytes_in_buffer = 1;
    memsizetype result_pos;
    register uchartype *from;
    register strelemtype *to;
    register memsizetype number;
    memsizetype new_size;
    stritype resized_result;
    stritype result;

  /* read_and_alloc_stri */
    /* printf("read_and_alloc_stri(%d, %d, *, *)\n", fileno(inFile), chars_missing); */
    if (!ALLOC_STRI_SIZE_OK(result, GETS_STRI_SIZE_DELTA)) {
      *err_info = MEMORY_ERROR;
      result = NULL;
    } else {
      result->size = GETS_STRI_SIZE_DELTA;
      /* printf("chars_missing=%lu\n", chars_missing); */
      result_pos = 0;
      while (chars_missing - result_pos >= BUFFER_SIZE && bytes_in_buffer > 0 &&
          *err_info == OKAY_NO_ERROR) {
        /* printf("read_size=%ld\n", BUFFER_SIZE); */
        bytes_in_buffer = (memsizetype) fread(buffer, 1, BUFFER_SIZE, inFile);
        if (bytes_in_buffer == 0 && result_pos == 0 && ferror(inFile)) {
          *err_info = FILE_ERROR;
        } else {
          /* printf("#A# bytes_in_buffer=%d result_pos=%d\n",
              bytes_in_buffer, result_pos); */
          if (result_pos + bytes_in_buffer > result->size) {
            new_size = result->size + GETS_STRI_SIZE_DELTA;
            REALLOC_STRI_CHECK_SIZE(resized_result, result, result->size, new_size);
            if (resized_result == NULL) {
              *err_info = MEMORY_ERROR;
              return result;
            } else {
              result = resized_result;
              COUNT3_STRI(result->size, new_size);
              result->size = new_size;
            } /* if */
          } /* if */
          from = buffer;
          to = &result->mem[result_pos];
          number = bytes_in_buffer;
          for (; number > 0; from++, to++, number--) {
            *to = *from;
          } /* for */
          result_pos += bytes_in_buffer;
        } /* if */
      } /* while */
      if (chars_missing > result_pos && bytes_in_buffer > 0 &&
          *err_info == OKAY_NO_ERROR) {
        /* printf("read_size=%ld\n", chars_missing - result_pos); */
        bytes_in_buffer = (memsizetype) fread(buffer, 1, chars_missing - result_pos, inFile);
        if (bytes_in_buffer == 0 && result_pos == 0 && ferror(inFile)) {
          *err_info = FILE_ERROR;
        } else {
          /* printf("#B# bytes_in_buffer=%d result_pos=%d\n",
              bytes_in_buffer, result_pos); */
          if (result_pos + bytes_in_buffer > result->size) {
            new_size = result->size + GETS_STRI_SIZE_DELTA;
            REALLOC_STRI_CHECK_SIZE(resized_result, result, result->size, new_size);
            if (resized_result == NULL) {
              *err_info = MEMORY_ERROR;
              return result;
            } else {
              result = resized_result;
              COUNT3_STRI(result->size, new_size);
              result->size = new_size;
            } /* if */
          } /* if */
          from = buffer;
          to = &result->mem[result_pos];
          number = bytes_in_buffer;
          for (; number > 0; from++, to++, number--) {
            *to = *from;
          } /* for */
          result_pos += bytes_in_buffer;
        } /* if */
      } /* if */
      /* printf("result_pos=%lu\n", result_pos); */
      *num_of_chars_read = result_pos;
    } /* if */
    return result;
  } /* read_and_alloc_stri */



/**
 *  Determine the size of a file and return it as bigInteger.
 *  The file length is measured in bytes.
 *  @return the size of the given file.
 *  @exception FILE_ERROR A system function returns an error or the
 *             file length reported by the system is negative.
 *  @exception MEMORY_ERROR Not enough memory to represent the result.
 */
biginttype filBigLng (filetype aFile)

  {
    int file_no;
    os_fstat_struct stat_buf;
    biginttype result;

  /* filBigLng */
    file_no = fileno(aFile);
    if (file_no != -1 && os_fstat(file_no, &stat_buf) == 0 &&
        S_ISREG(stat_buf.st_mode)) {
      if (unlikely(stat_buf.st_size < 0)) {
        raise_error(FILE_ERROR);
        result = NULL;
      } else {
#if OS_OFF_T_SIZE == 32
        result = bigFromUInt32((uint32type) stat_buf.st_size);
#elif OS_OFF_T_SIZE == 64
        result = bigFromUInt64((uint64type) stat_buf.st_size);
#else
#error "sizeof(os_off_t) is neither 4 nor 8."
#endif
      } /* if */
    } else {
      result = getBigFileLengthUsingSeek(aFile);
    } /* if */
    return result;
  } /* filBigLng */



/**
 *  Set the current file position.
 *  The file position is measured in bytes from the start of the file.
 *  The first byte in the file has the position 1.
 *  @exception RANGE_ERROR The file position is negative or zero or
 *             the file position is not representable in the system
 *             file position type.
 *  @exception FILE_ERROR A system function returns an error.
 */
void filBigSeek (filetype aFile, const const_biginttype position)

  {
    os_off_t file_position;

  /* filBigSeek */
#if OS_OFF_T_SIZE == 32
    file_position = (os_off_t) bigToInt32(position);
#elif OS_OFF_T_SIZE == 64
    file_position = (os_off_t) bigToInt64(position);
#else
#error "sizeof(os_off_t) is neither 4 nor 8."
#endif
    if (unlikely(file_position <= 0)) {
      raise_error(RANGE_ERROR);
    } else if (unlikely(offsetSeek(aFile, file_position - 1, SEEK_SET) != 0)) {
      raise_error(FILE_ERROR);
    } /* if */
  } /* filBigSeek */



/**
 *  Obtain the current file position.
 *  The file position is measured in bytes from the start of the file.
 *  The first byte in the file has the position 1.
 *  @return the current file position.
 *  @exception FILE_ERROR A system function returns an error or the
 *             file position reported by the system is negative.
 *  @exception MEMORY_ERROR Not enough memory to represent the result.
 */
biginttype filBigTell (filetype aFile)

  {
    os_off_t current_file_position;
    biginttype result;

  /* filBigTell */
    current_file_position = offsetTell(aFile);
    if (unlikely(current_file_position < (os_off_t) 0)) {
      raise_error(FILE_ERROR);
      result = NULL;
    } else {
#if OS_OFF_T_SIZE == 32
      result = bigFromUInt32((uint32type) current_file_position + 1);
#elif OS_OFF_T_SIZE == 64
      result = bigFromUInt64((uint64type) current_file_position + 1);
#else
#error "sizeof(os_off_t) is neither 4 nor 8."
#endif
    } /* if */
    return result;
  } /* filBigTell */



/**
 *  Close a clib_file.
 *  @exception FILE_ERROR A system function returns an error.
 */
void filClose (filetype aFile)

  { /* filClose */
    if (unlikely(fclose(aFile) != 0)) {
      /* printf("errno=%d\n", errno);
         printf("EACCES=%d  EBUSY=%d  EEXIST=%d  ENOTEMPTY=%d  ENOENT=%d  EISDIR=%d  EROFS=%d  EBADF=%d\n",
                EACCES, EBUSY, EEXIST, ENOTEMPTY, ENOENT, EISDIR, EROFS, EBADF); */
      raise_error(FILE_ERROR);
    } /* if */
  } /* filClose */



#ifdef OUT_OF_ORDER
inttype filFileType (filetype aFile)

  {
    int file_no;
    os_fstat_struct stat_buf;
    inttype result;

  /* filFileType */
    file_no = fileno(aFile);
    if (file_no != -1 && os_fstat(file_no, &stat_buf) == 0) {
      if (S_ISREG(stat_buf.st_mode)) {
        result = 2;
      } else if (S_ISDIR(stat_buf.st_mode)) {
        result = 3;
      } else if (S_ISCHR(stat_buf.st_mode)) {
        result = 4;
      } else if (S_ISBLK(stat_buf.st_mode)) {
        result = 5;
      } else if (S_ISFIFO(stat_buf.st_mode)) {
        result = 6;
      } else if (S_ISLNK(stat_buf.st_mode)) {
        result = 7;
      } else if (S_ISSOCK(stat_buf.st_mode)) {
        result = 8;
      } else {
        result = 1;
      } /* if */
    } else {
      result = 0;
      raise_error(FILE_ERROR);
    } /* if */
    return result;
  } /* filFileType */
#endif



/**
 *  Read a string with 'length' characters from 'inFile'.
 *  In order to work reasonable good for the common case (reading
 *  just some characters), memory for 'length' characters is requested
 *  with malloc(). After the data is read the result string is
 *  shrinked to the actual size (with realloc()). When 'length' is
 *  larger than GETS_DEFAULT_SIZE or the memory cannot be requested
 *  a different strategy is used. In this case the function tries to
 *  find out the number of available characters (this is possible
 *  for a regular file but not for a pipe). If this fails a third
 *  strategy is used. In this case a smaller block is requested. This
 *  block is filled with data, resized and filled in a loop.
 *  @return the string read.
 *  @exception RANGE_ERROR The length is negative.
 *  @exception MEMORY_ERROR Not enough memory to represent the result.
 *  @exception FILE_ERROR A system function returns an error.
 */
stritype filGets (filetype inFile, inttype length)

  {
    memsizetype chars_requested;
    memsizetype bytes_there;
    memsizetype allocated_size;
    errinfotype err_info = OKAY_NO_ERROR;
    memsizetype num_of_chars_read;
    stritype resized_result;
    stritype result;

  /* filGets */
    /* printf("filGets(%d, %d)\n", fileno(inFile), length); */
    if (unlikely(length < 0)) {
      raise_error(RANGE_ERROR);
      result = NULL;
    } else {
      if ((uinttype) length > MAX_MEMSIZETYPE) {
        chars_requested = MAX_MEMSIZETYPE;
      } else {
        chars_requested = (memsizetype) length;
      } /* if */
      if (chars_requested > GETS_DEFAULT_SIZE) {
        /* Avoid requesting too much */
        result = NULL;
      } else {
        allocated_size = chars_requested;
        (void) ALLOC_STRI_SIZE_OK(result, allocated_size);
      } /* if */
      if (result == NULL) {
        bytes_there = remainingBytesInFile(inFile);
        /* printf("bytes_there=%lu\n", bytes_there); */
        if (bytes_there != 0) {
          /* Now we know that bytes_there bytes are available in inFile */
          if (chars_requested <= bytes_there) {
            allocated_size = chars_requested;
          } else {
            allocated_size = bytes_there;
          } /* if */
          /* printf("allocated_size=%lu\n", allocated_size); */
          if (unlikely(!ALLOC_STRI_CHECK_SIZE(result, allocated_size))) {
            /* printf("MAX_STRI_LEN=%lu, SIZ_STRI(MAX_STRI_LEN)=%lu\n",
                MAX_STRI_LEN, SIZ_STRI(MAX_STRI_LEN)); */
            raise_error(MEMORY_ERROR);
            return NULL;
          } /* if */
        } /* if */
      } /* if */
      if (result != NULL) {
        /* We have allocated a buffer for the requested number of char
           or for the number of bytes which are available in the file */
        result->size = allocated_size;
        if (allocated_size <= BUFFER_SIZE) {
          /* printf("read_size=%ld\n", allocated_size); */
          num_of_chars_read = (memsizetype) fread(result->mem, 1,
              (size_t) allocated_size, inFile);
          /* printf("num_of_chars_read=%lu\n", num_of_chars_read); */
          if (num_of_chars_read == 0 && ferror(inFile)) {
            /* printf("errno=%d\n", errno);
            printf("EACCES=%d  EBUSY=%d  EEXIST=%d  ENOTEMPTY=%d  ENOENT=%d  EISDIR=%d  EROFS=%d  EBADF=%d\n",
                EACCES, EBUSY, EEXIST, ENOTEMPTY, ENOENT, EISDIR, EROFS, EBADF);
            printf("inFile=%lx\n", (long int) inFile); */
            err_info = FILE_ERROR;
          } else {
            uchartype *from = &((uchartype *) result->mem)[num_of_chars_read - 1];
            strelemtype *to = &result->mem[num_of_chars_read - 1];
            memsizetype number = num_of_chars_read;

            for (; number > 0; from--, to--, number--) {
              *to = *from;
            } /* for */
          } /* if */
        } else {
          num_of_chars_read = read_string(inFile, result, &err_info);
        } /* if */
      } else {
        /* We do not know how many bytes are avaliable therefore
           result is resized with GETS_STRI_SIZE_DELTA until we
           have read enough or we reach EOF */
        result = read_and_alloc_stri(inFile, chars_requested, &num_of_chars_read, &err_info);
      } /* if */
      if (unlikely(err_info != OKAY_NO_ERROR)) {
        if (result != NULL) {
          FREE_STRI(result, result->size);
        } /* if */
        raise_error(err_info);
        result = NULL;
      } else if (num_of_chars_read < result->size) {
        REALLOC_STRI_SIZE_OK(resized_result, result, result->size, num_of_chars_read);
        if (unlikely(resized_result == NULL)) {
          FREE_STRI(result, result->size);
          raise_error(MEMORY_ERROR);
          result = NULL;
        } else {
          result = resized_result;
          COUNT3_STRI(result->size, num_of_chars_read);
          result->size = num_of_chars_read;
        } /* if */
      } /* if */
    } /* if */
    /* printf("filGets(%d, %d) ==> ", fileno(inFile), length);
       prot_stri(result);
       printf("\n"); */
    return result;
  } /* filGets */



/**
 *  Determine if at least one character can be read successfully.
 *  This function allows a file to be handled like an iterator.
 *  @return FALSE if 'getc' would return EOF, TRUE otherwise.
 */
booltype filHasNext (filetype inFile)

  {
    int next_char;
    booltype result;

  /* filHasNext */
    if (feof(inFile)) {
      result = FALSE;
    } else {
      next_char = getc(inFile);
      if (next_char != EOF) {
        if (unlikely(ungetc(next_char, inFile) != next_char)) {
          raise_error(FILE_ERROR);
          result = FALSE;
        } else {
          result = TRUE;
        } /* if */
      } else {
        clearerr(inFile);
        result = FALSE;
      } /* if */
    } /* if */
    return result;
  } /* filHasNext */



/**
 *  Read a line from a clib_file.
 *  The function accepts lines ending with "\n", "\r\n" or EOF.
 *  The line ending characters are not copied into the string.
 *  That means that the "\r" of a "\r\n" sequence is silently removed.
 *  When the function is left terminationChar contains '\n' or EOF.
 *  @return the line read.
 *  @exception MEMORY_ERROR Not enough memory to represent the result.
 *  @exception FILE_ERROR A system function returns an error.
 */
stritype filLineRead (filetype inFile, chartype *terminationChar)

  {
    register int ch;
    register memsizetype position;
    strelemtype *memory;
    memsizetype memlength;
    memsizetype newmemlength;
    stritype resized_result;
    stritype result;

  /* filLineRead */
    memlength = READ_STRI_INIT_SIZE;
    if (unlikely(!ALLOC_STRI_SIZE_OK(result, memlength))) {
      raise_error(MEMORY_ERROR);
    } else {
      memory = result->mem;
      position = 0;
      while ((ch = getc(inFile)) != (int) '\n' && ch != EOF) {
        if (position >= memlength) {
          newmemlength = memlength + READ_STRI_SIZE_DELTA;
          REALLOC_STRI_CHECK_SIZE(resized_result, result, memlength, newmemlength);
          if (unlikely(resized_result == NULL)) {
            FREE_STRI(result, memlength);
            raise_error(MEMORY_ERROR);
            return NULL;
          } /* if */
          result = resized_result;
          COUNT3_STRI(memlength, newmemlength);
          memory = result->mem;
          memlength = newmemlength;
        } /* if */
        memory[position++] = (strelemtype) ch;
      } /* while */
      if (ch == (int) '\n' && position != 0 && memory[position - 1] == '\r') {
        position--;
      } /* if */
      if (unlikely(ch == EOF && position == 0 && ferror(inFile))) {
        FREE_STRI(result, memlength);
        raise_error(FILE_ERROR);
        result = NULL;
      } else {
        REALLOC_STRI_SIZE_OK(resized_result, result, memlength, position);
        if (unlikely(resized_result == NULL)) {
          FREE_STRI(result, memlength);
          raise_error(MEMORY_ERROR);
          result = NULL;
        } else {
          result = resized_result;
          COUNT3_STRI(memlength, position);
          result->size = position;
          *terminationChar = (chartype) ch;
        } /* if */
      } /* if */
    } /* if */
    return result;
  } /* filLineRead */



stritype filLit (filetype aFile)

  {
    const_cstritype file_name;
    stritype result;

  /* filLit */
    if (aFile == NULL) {
      file_name = "NULL";
    } else if (aFile == stdin) {
      file_name = "stdin";
    } else if (aFile == stdout) {
      file_name = "stdout";
    } else if (aFile == stderr) {
      file_name = "stderr";
    } else {
      file_name = "file";
    } /* if */
    result = cstri_to_stri(file_name);
    if (unlikely(result == NULL)) {
      raise_error(MEMORY_ERROR);
    } /* if */
    return result;
  } /* filLit */



/**
 *  Obtain the length of a file.
 *  The file length is measured in bytes.
 *  @return the size of the given file.
 *  @exception RANGE_ERROR The file length does not fit into
 *             an integer value.
 *  @exception FILE_ERROR A system function returns an error or the
 *             file length reported by the system is negative.
 */
inttype filLng (filetype aFile)

  {
    int file_no;
    os_fstat_struct stat_buf;
    inttype result;

  /* filLng */
    file_no = fileno(aFile);
    if (file_no != -1 && os_fstat(file_no, &stat_buf) == 0 &&
        S_ISREG(stat_buf.st_mode)) {
      if (unlikely(stat_buf.st_size < 0)) {
        raise_error(FILE_ERROR);
        result = 0;
      } else if (unlikely(stat_buf.st_size > INTTYPE_MAX)) {
        raise_error(RANGE_ERROR);
        result = 0;
      } else {
        result = (inttype) stat_buf.st_size;
      } /* if */
    } else {
      result = getFileLengthUsingSeek(aFile);
    } /* if */
    return result;
  } /* filLng */



/**
 *  Opens the file with the specified 'path' and 'mode'.
 *  Unicode characters in 'path' are converted to the representation
 *  used by the fopen() function of the operating system. In 'path'
 *  a slash / must be used as path delimiter. Using a backslash \
 *  in 'path' may raise the exception 'RANGE_ERROR'. The following
 *  values are allowed as mode: "r", "w", "a", "r+", "w+", "a+",
 *  "rt", "wt", "at", "rt+", "wt+" and "at+". The modes with t are
 *  text modes and the modes without t are binary modes. Note that
 *  this modes differ from the ones used by the C function fopen().
 *  When 'mode' is not one of the allowed values the exception
 *  'RANGE_ERROR' is raised. An attempt to open a directory returns
 *  NULL.
 *  @return return a filetype value when fopen() succeeds, or NULL
 *          when fopen() fails or when the file is a directory.
 *  @exception MEMORY_ERROR Not enough memory to convert the path
 *             to the system path type.
 *  @exception RANGE_ERROR The 'mode' is not one of the allowed
 *             values or 'path' does not use the standard path
 *             representation or 'path' cannot be converted
 *             to the system path type.
 */
filetype filOpen (const const_stritype path, const const_stritype mode)

  {
    os_stritype os_path;
    os_chartype os_mode[4];
    int path_info = PATH_IS_NORMAL;
    errinfotype err_info = OKAY_NO_ERROR;
#ifdef FOPEN_OPENS_DIRECTORIES
    int file_no;
    os_fstat_struct stat_buf;
#endif
    filetype result;

  /* filOpen */
    /* printf("BEGIN filOpen(%lX, %lX)\n", path, mode); */
    get_mode(os_mode, mode);
    if (unlikely(os_mode[0] == '\0')) {
      raise_error(RANGE_ERROR);
      result = NULL;
    } else {
      os_path = cp_to_os_path(path, &path_info, &err_info);
      if (unlikely(err_info != OKAY_NO_ERROR)) {
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
        if (path_info == PATH_IS_NORMAL) {
          raise_error(err_info);
        } /* if */
#else
        raise_error(err_info);
#endif
        result = NULL;
      } else {
        result = os_fopen(os_path, os_mode);
        os_stri_free(os_path);
#ifdef FOPEN_OPENS_DIRECTORIES
        if (result != NULL) {
          file_no = fileno(result);
          if (file_no != -1 && os_fstat(file_no, &stat_buf) == 0 &&
              S_ISDIR(stat_buf.st_mode)) {
            /* An attempt to open a directory with filOpen()     */
            /* returns NULL even when fopen() succeeds. On many  */
            /* modern operating systems functions like fgetc()   */
            /* and readf() fail to read from a directory anyway. */
            /* So it is better to fail early, when the file is   */
            /* opened, instead of later at an unexpected place.  */
            /* Even when reading a directory as file succeeds    */
            /* there is another issue: Reading a directory as    */
            /* file is not portable, since it delivers an        */
            /* operating system specific representation of the   */
            /* directory. So reading a directory as file should  */
            /* be avoided altogether. The functions dirOpen(),   */
            /* dirRead() and dirClose() provide a portable way   */
            /* to open, read and close a directory.              */
            fclose(result);
            result = NULL;
          } /* if */
        } /* if */
#endif
      } /* if */
    } /* if */
    /* printf("END filOpen(%lX, %lX) => %lX\n", path, mode, result); */
    return result;
  } /* filOpen */



void filPclose (filetype aFile)

  { /* filPclose */
    if (unlikely(os_pclose(aFile) == -1)) {
      raise_error(FILE_ERROR);
    } /* if */
  } /* filPclose */



/**
 *  Open a pipe to a shell 'command', with 'parameters'.
 *  The command reads, respectively writes with Latin-1 encoding.
 *  A pipe can only be opened with the modes "r" and "w".
 *  @return the pipe file opened, or NULL if it could not be opened.
 *  @exception RANGE_ERROR An illegal mode was used.
 */
filetype filPopen (const const_stritype command,
    const const_stritype parameters, const const_stritype mode)

  {
    os_stritype os_command;
    os_chartype os_mode[4];
    errinfotype err_info = OKAY_NO_ERROR;
    filetype result;

  /* filPopen */
    os_command = cp_to_command(command, parameters, &err_info);
    if (unlikely(os_command == NULL)) {
      raise_error(err_info);
      result = NULL;
    } else {
      os_mode[0] = '\0';
      if (mode->size == 1 &&
          (mode->mem[0] == 'r' ||
           mode->mem[0] == 'w')) {
        os_mode[0] = (os_chartype) mode->mem[0];
        os_mode[1] = '\0';
      } /* if */
      /* The mode "rb" is not allowed under Unix/Linux */
      /* therefore get_mode(os_mode, mode); cannot be called */
      if (unlikely(os_mode[0] == '\0')) {
        raise_error(RANGE_ERROR);
        result = NULL;
      } else {
        /* printf("os_command: \"%s\"\n", os_command); */
        result = os_popen(os_command, os_mode);
      } /* if */
      os_stri_free(os_command);
    } /* if */
    return result;
  } /* filPopen */



void filPrint (const const_stritype stri)

  {
    cstritype str1;

  /* filPrint */
    str1 = cp_to_cstri8(stri);
    if (unlikely(str1 == NULL)) {
      raise_error(MEMORY_ERROR);
    } else {
      puts(str1);
      fflush(stdout);
      free_cstri(str1, stri);
    } /* if */
  } /* filPrint */



/**
 *  Set the current file position.
 *  The file position is measured in bytes from the start of the file.
 *  The first byte in the file has the position 1.
 *  @exception RANGE_ERROR The file position is negative or zero or
 *             the file position is not representable in the system
 *             file position type.
 *  @exception FILE_ERROR A system function returns an error.
 */
void filSeek (filetype aFile, inttype position)

  {
    os_off_t file_position;

  /* filSeek */
    /* printf("filSeek(%ld, %ld)\n", aFile, position); */
    if (unlikely(position <= 0)) {
      raise_error(RANGE_ERROR);
#if OS_OFF_T_SIZE < INTTYPE_SIZE
#if OS_OFF_T_SIZE == 32
    } else if (unlikely(position > INT32TYPE_MAX)) {
      raise_error(RANGE_ERROR);
#elif OS_OFF_T_SIZE == 64
    } else if (unlikely(position > INT64TYPE_MAX)) {
      raise_error(RANGE_ERROR);
#else
#error "sizeof(os_off_t) is neither 4 nor 8."
#endif
#endif
    } else {
      file_position = (os_off_t) position;
      if (unlikely(offsetSeek(aFile, file_position - 1, SEEK_SET) != 0)) {
        raise_error(FILE_ERROR);
      } /* if */
    } /* if */
  } /* filSeek */



void filSetbuf (filetype aFile, inttype mode, inttype size)

  { /* filSetbuf */
    if (unlikely(mode < 0 || mode > 2 || size < 0 || (uinttype) size > MAX_MEMSIZETYPE)) {
      raise_error(RANGE_ERROR);
    } else if (unlikely(setvbuf(aFile, NULL, (int) mode, (memsizetype) size) != 0)) {
      raise_error(FILE_ERROR);
    } /* if */
  } /* filSetbuf */



/**
 *  Obtain the current file position.
 *  The file position is measured in bytes from the start of the file.
 *  The first byte in the file has the position 1.
 *  @return the current file position.
 *  @exception RANGE_ERROR The file position does not fit into
 *             an integer value.
 *  @exception FILE_ERROR A system function returns an error or the
 *             file position reported by the system is negative.
 */
inttype filTell (filetype aFile)

  {
    os_off_t current_file_position;
    inttype result;

  /* filTell */
    current_file_position = offsetTell(aFile);
    if (unlikely(current_file_position < (os_off_t) 0)) {
      raise_error(FILE_ERROR);
      result = 0;
    } else if (unlikely(current_file_position >= INTTYPE_MAX)) {
      raise_error(RANGE_ERROR);
      result = 0;
    } else {
      result = (inttype) (current_file_position + 1);
    } /* if */
    return result;
  } /* filTell */



/**
 *  Read a word from a clib_file.
 *  Before reading the word it skips spaces and tabs. The function
 *  accepts words ending with " ", "\t", "\n", "\r\n" or EOF.
 *  The word ending characters are not copied into the string.
 *  That means that the "\r" of a "\r\n" sequence is silently removed.
 *  When the function is left terminationChar contains ' ', '\t', '\n' or
 *  EOF.
 *  @return the word read.
 *  @exception MEMORY_ERROR Not enough memory to represent the result.
 *  @exception FILE_ERROR A system function returns an error.
 */
stritype filWordRead (filetype inFile, chartype *terminationChar)

  {
    register int ch;
    register memsizetype position;
    strelemtype *memory;
    memsizetype memlength;
    memsizetype newmemlength;
    stritype resized_result;
    stritype result;

  /* filWordRead */
    memlength = READ_STRI_INIT_SIZE;
    if (unlikely(!ALLOC_STRI_SIZE_OK(result, memlength))) {
      raise_error(MEMORY_ERROR);
    } else {
      memory = result->mem;
      position = 0;
      do {
        ch = getc(inFile);
      } while (ch == (int) ' ' || ch == (int) '\t');
      while (ch != (int) ' ' && ch != (int) '\t' &&
          ch != (int) '\n' && ch != EOF) {
        if (position >= memlength) {
          newmemlength = memlength + READ_STRI_SIZE_DELTA;
          REALLOC_STRI_CHECK_SIZE(resized_result, result, memlength, newmemlength);
          if (unlikely(resized_result == NULL)) {
            FREE_STRI(result, memlength);
            raise_error(MEMORY_ERROR);
            return NULL;
          } /* if */
          result = resized_result;
          COUNT3_STRI(memlength, newmemlength);
          memory = result->mem;
          memlength = newmemlength;
        } /* if */
        memory[position++] = (strelemtype) ch;
        ch = getc(inFile);
      } /* while */
      if (ch == (int) '\n' && position != 0 && memory[position - 1] == '\r') {
        position--;
      } /* if */
      if (unlikely(ch == EOF && position == 0 && ferror(inFile))) {
        FREE_STRI(result, memlength);
        raise_error(FILE_ERROR);
        result = NULL;
      } else {
        REALLOC_STRI_SIZE_OK(resized_result, result, memlength, position);
        if (unlikely(resized_result == NULL)) {
          FREE_STRI(result, memlength);
          raise_error(MEMORY_ERROR);
          result = NULL;
        } else {
          result = resized_result;
          COUNT3_STRI(memlength, position);
          result->size = position;
          *terminationChar = (chartype) ch;
        } /* if */
      } /* if */
    } /* if */
    return result;
  } /* filWordRead */



/**
 *  Write a string to a clib_file.
 *  @exception FILE_ERROR A system function returns an error.
 *  @exception RANGE_ERROR The string contains a character that does
 *             not fit into a byte.
 */
void filWrite (filetype outFile, const const_stritype stri)

  {
    register strelemtype *str;
    memsizetype len;
    register uchartype *ustri;
    register uint16type buf_len;
    uchartype buffer[BUFFER_SIZE];

  /* filWrite */
    /* printf("filWrite(%d, ", fileno(outFile));
       prot_stri(stri);
       printf(")\n"); */
#ifdef FWRITE_WRONG_FOR_READ_ONLY_FILES
    if (unlikely(stri->size > 0 && (outFile->flags & _F_WRIT) == 0)) {
      raise_error(FILE_ERROR);
      return;
    } /* if */
#endif
    for (str = stri->mem, len = stri->size;
        len >= BUFFER_SIZE; len -= BUFFER_SIZE) {
      for (ustri = buffer, buf_len = BUFFER_SIZE; buf_len > 0; buf_len--) {
        if (unlikely(*str >= 256)) {
          raise_error(RANGE_ERROR);
          return;
        } /* if */
        *ustri++ = (uchartype) *str++;
      } /* for */
      if (unlikely(BUFFER_SIZE != fwrite(buffer, sizeof(uchartype), BUFFER_SIZE, outFile))) {
        raise_error(FILE_ERROR);
        return;
      } /* if */
    } /* for */
    if (len > 0) {
      for (ustri = buffer, buf_len = (uint16type) len; buf_len > 0; buf_len--) {
        if (unlikely(*str >= 256)) {
          raise_error(RANGE_ERROR);
          return;
        } /* if */
        *ustri++ = (uchartype) *str++;
      } /* for */
      if (unlikely(len != fwrite(buffer, sizeof(uchartype), len, outFile))) {
        raise_error(FILE_ERROR);
        return;
      } /* if */
    } /* if */
  } /* filWrite */
