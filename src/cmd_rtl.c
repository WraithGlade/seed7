/********************************************************************/
/*                                                                  */
/*  cmd_rtl.c     Primitive actions for various commands.           */
/*  Copyright (C) 1989 - 2011  Thomas Mertes                        */
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
/*  File: seed7/src/cmd_rtl.c                                       */
/*  Changes: 1994, 2006, 2009  Thomas Mertes                        */
/*  Content: Primitive actions for various commands.                */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "limits.h"
#include "time.h"
#include "sys/types.h"
#include "sys/stat.h"
#ifdef INCLUDE_SYS_UTIME
#include "sys/utime.h"
#else
#include "utime.h"
#endif
#ifdef OS_STRI_WCHAR
#include "wchar.h"
#ifdef OS_WIDE_DIR_INCLUDE_DIR_H
#include "dir.h"
#endif
#ifdef OS_CHMOD_INCLUDE_IO_H
#include "io.h"
#endif
#endif
#include "errno.h"

#ifdef UNISTD_H_PRESENT
#include "unistd.h"
#endif

#include "dir_drv.h"

#include "common.h"
#include "data_rtl.h"
#include "os_decls.h"
#include "heaputl.h"
#include "striutl.h"
#include "str_rtl.h"
#include "fil_rtl.h"
#include "dir_rtl.h"
#include "set_rtl.h"
#include "tim_rtl.h"
#include "tim_drv.h"
#include "big_drv.h"
#include "cmd_drv.h"
#include "rtl_err.h"

#ifdef USE_MMAP
#include "sys/mman.h"
#endif

#undef EXTERN
#define EXTERN
#include "cmd_rtl.h"

#undef TRACE_CMD_RTL


#ifndef CPLUSPLUS_COMPILER
#define CPLUSPLUS_COMPILER C_COMPILER
#endif
#ifndef EXECUTABLE_FILE_EXTENSION
#define EXECUTABLE_FILE_EXTENSION ""
#endif
#ifndef CC_FLAGS
#define CC_FLAGS ""
#endif
#ifndef REDIRECT_C_ERRORS
#define REDIRECT_C_ERRORS ""
#endif
#ifndef LINKER_OPT_DEBUG_INFO
#define LINKER_OPT_DEBUG_INFO ""
#endif
#ifndef LINKER_OPT_NO_DEBUG_INFO
#define LINKER_OPT_NO_DEBUG_INFO ""
#endif
#ifndef LINKER_OPT_OUTPUT_FILE
#define LINKER_OPT_OUTPUT_FILE ""
#endif
#ifndef LINKER_FLAGS
#define LINKER_FLAGS ""
#endif

#ifndef INT64TYPE
#define INT64TYPE_STRI ""
#define UINT64TYPE_STRI ""
#define INT32TYPE_LITERAL_SUFFIX ""
#define INT64TYPE_LITERAL_SUFFIX ""
#endif

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

#define SIZE_NORMAL_BUFFER   32768
#define SIZE_RESERVE_BUFFER   2048

#define PRESERVE_NOTHING    0x00
#define PRESERVE_MODE       0x01
#define PRESERVE_OWNERSHIP  0x02
#define PRESERVE_TIMESTAMPS 0x04
#define PRESERVE_SYMLINKS   0x08
#define PRESERVE_ALL        0xFF

#define MAX_SYMLINK_PATH_LENGTH 0x1FFFFFFF

#define USR_BITS_NORMAL (S_IRUSR == 0400 && S_IWUSR == 0200 && S_IXUSR == 0100)
#define GRP_BITS_NORMAL (S_IRGRP == 0040 && S_IWGRP == 0020 && S_IXGRP == 0010)
#define OTH_BITS_NORMAL (S_IROTH == 0004 && S_IWOTH == 0002 && S_IXOTH == 0001)
#define MODE_BITS_NORMAL (USR_BITS_NORMAL && GRP_BITS_NORMAL && OTH_BITS_NORMAL)

#define INITAL_ARRAY_SIZE  256
#define ARRAY_SIZE_DELTA   256

#ifdef DEFINE_OS_ENVIRON
extern os_stritype *os_environ;
#endif

extern stritype programPath; /* defined in hi.c or in the executable of a program */



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
    return (int) strCompare(
        ((const_rtlObjecttype *) strg1)->value.strivalue,
        ((const_rtlObjecttype *) strg2)->value.strivalue);
  } /* cmp_mem */



#ifdef ANSI_C
static void remove_any_file (os_stritype, errinfotype *);
static void copy_any_file (os_stritype, os_stritype, int, errinfotype *);
#else
static void remove_any_file ();
static void copy_any_file ();
#endif



#ifdef ANSI_C

static void remove_dir (os_stritype dir_name, errinfotype *err_info)
#else

static void remove_dir (dir_name, err_info)
os_stritype dir_name;
errinfotype *err_info;
#endif

  {
    os_DIR *directory;
    os_dirent_struct *current_entry;
    size_t dir_name_size;
    size_t dir_path_capacity = 0;
    os_stritype dir_path = NULL;
    size_t new_size;
    os_stritype resized_path;
    booltype init_path = TRUE;

  /* remove_dir */
#ifdef TRACE_CMD_RTL
    printf("BEGIN remove_dir(\"%s\")\n", dir_name);
#endif
/*  printf("opendir(%s);\n", dir_name);
    fflush(stdout); */
    if ((directory = os_opendir(dir_name)) == NULL) {
      *err_info = FILE_ERROR;
    } else {
      do {
        current_entry = os_readdir(directory);
/*      printf("$%ld$\n", (long) current_entry);
        fflush(stdout); */
      } while (current_entry != NULL &&
          (memcmp(current_entry->d_name, dot,    sizeof(os_chartype) * 2) == 0 ||
           memcmp(current_entry->d_name, dotdot, sizeof(os_chartype) * 3) == 0));
      dir_name_size = os_stri_strlen(dir_name);
      while (*err_info == OKAY_NO_ERROR && current_entry != NULL) {
/*      printf("!%s!\n", current_entry->d_name);
        fflush(stdout); */
        new_size = dir_name_size + 1 + os_stri_strlen(current_entry->d_name);
        if (new_size > dir_path_capacity) {
          resized_path = os_stri_realloc(dir_path, new_size);
          if (resized_path != NULL) {
            dir_path = resized_path;
            dir_path_capacity = new_size;
          } else if (dir_path != NULL) {
            os_stri_free(dir_path);
            dir_path = NULL;
          } /* if */
        } /* if */
        if (dir_path != NULL) {
          if (init_path) {
            os_stri_strcpy(dir_path, dir_name);
            os_stri_strcpy(&dir_path[dir_name_size], slash);
            init_path = FALSE;
          } /* if */
          os_stri_strcpy(&dir_path[dir_name_size + 1], current_entry->d_name);
          remove_any_file(dir_path, err_info);
        } else {
          *err_info = MEMORY_ERROR;
        } /* if */
        do {
          current_entry = os_readdir(directory);
/*        printf("$%ld$\n", (long) current_entry);
          fflush(stdout); */
        } while (current_entry != NULL &&
            (memcmp(current_entry->d_name, dot,    sizeof(os_chartype) * 2) == 0 ||
             memcmp(current_entry->d_name, dotdot, sizeof(os_chartype) * 3) == 0));
      } /* while */
      if (dir_path != NULL) {
        os_stri_free(dir_path);
      } /* if */
      os_closedir(directory);
      if (*err_info == OKAY_NO_ERROR) {
        /* printf("before remove directory <%s>\n", dir_name); */
        if (os_rmdir(dir_name) != 0) {
          *err_info = FILE_ERROR;
          /* printf("errno=%d\n", errno);
          printf("EACCES=%d  EBUSY=%d  EEXIST=%d  ENOTEMPTY=%d  ENOENT=%d  ENOTDIR=%d  EROFS=%d\n",
              EACCES, EBUSY, EEXIST, ENOTEMPTY, ENOENT, ENOTDIR, EROFS);
          printf("EFAULT=%d  EISDIR=%d  ENAMETOOLONG=%d  ENODEV=%d  EINVAL=%d\n",
              EFAULT, EISDIR, ENAMETOOLONG, ENODEV, EINVAL); */
          /* printf("dir_name=\"%s\"\n", dir_name); */
        } /* if */
      } /* if */
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("END remove_dir(\"%s\", %d)\n", dir_name, *err_info);
#endif
  } /* remove_dir */



#ifdef ANSI_C

static void remove_any_file (os_stritype file_name, errinfotype *err_info)
#else

static void remove_any_file (file_name, err_info)
os_stritype file_name;
errinfotype *err_info;
#endif

  {
    os_stat_struct file_stat;

  /* remove_any_file */
#ifdef TRACE_CMD_RTL
    printf("BEGIN remove_any_file(\"%s\")\n", file_name);
#endif
    if (os_lstat(file_name, &file_stat) != 0) {
      /* File does not exist */
      *err_info = FILE_ERROR;
    } else {
      if (S_ISDIR(file_stat.st_mode)) {
        remove_dir(file_name, err_info);
      } else {
        if (os_remove(file_name) != 0) {
          *err_info = FILE_ERROR;
        } /* if */
      } /* if */
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("END remove_any_file(\"%s\", %d)\n", file_name, *err_info);
#endif
  } /* remove_any_file */



#ifdef ANSI_C

static void copy_file (os_stritype from_name, os_stritype to_name, errinfotype *err_info)
#else

static void copy_file (from_name, to_name, err_info)
os_stritype from_name;
os_stritype to_name;
errinfotype *err_info;
#endif

  {
    FILE *from_file;
    FILE *to_file;
#ifdef USE_MMAP
    int file_no;
    os_fstat_struct file_stat;
    memsizetype file_length;
    ustritype file_content;
    booltype classic_copy = TRUE;
#endif
    char *buffer;
    size_t buffer_size;
    char reserve_buffer[SIZE_RESERVE_BUFFER];
    size_t bytes_read;

  /* copy_file */
#ifdef TRACE_CMD_RTL
    printf("BEGIN copy_file(\"%s\", \"%s\")\n", from_name, to_name);
#endif
    if ((from_file = os_fopen(from_name, os_mode_rb)) != NULL) {
      if ((to_file = os_fopen(to_name, os_mode_wb)) != NULL) {
#ifdef USE_MMAP
        file_no = fileno(from_file);
        if (file_no != -1 && os_fstat(file_no, &file_stat) == 0) {
          if (file_stat.st_size < MAX_MEMSIZETYPE && file_stat.st_size >= 0) {
            file_length = (memsizetype) file_stat.st_size;
            if ((file_content = (ustritype) mmap(NULL, file_length,
                PROT_READ, MAP_PRIVATE, fileno(from_file),
                0)) != (ustritype) -1) {
              if (fwrite(file_content, 1, file_length, to_file) != file_length) {
                *err_info = FILE_ERROR;
              } /* if */
              munmap(file_content, file_length);
              classic_copy = FALSE;
            } /* if */
          } /* if */
        } /* if */
        if (classic_copy) {
#endif
          if (ALLOC_BYTES(buffer, SIZE_NORMAL_BUFFER)) {
            buffer_size = SIZE_NORMAL_BUFFER;
          } else {
            buffer = reserve_buffer;
            buffer_size = SIZE_RESERVE_BUFFER;
          } /* if */
          while (*err_info == OKAY_NO_ERROR && (bytes_read =
              fread(buffer, 1, buffer_size, from_file)) != 0) {
            if (fwrite(buffer, 1, bytes_read, to_file) != bytes_read) {
              *err_info = FILE_ERROR;
            } /* if */
          } /* while */
          if (buffer != reserve_buffer) {
            FREE_BYTES(buffer, SIZE_NORMAL_BUFFER);
          } /* if */
#ifdef USE_MMAP
        } /* if */
#endif
        if (fclose(from_file) != 0) {
          *err_info = FILE_ERROR;
        } /* if */
        if (fclose(to_file) != 0) {
          *err_info = FILE_ERROR;
        } /* if */
        if (*err_info != OKAY_NO_ERROR) {
          os_remove(to_name);
        } /* if */
      } else {
        /* printf("errno=%d\n", errno);
        printf("EACCES=%d  EBUSY=%d  EEXIST=%d  ENOTEMPTY=%d  ENOENT=%d  ENOTDIR=%d  EROFS=%d\n",
            EACCES, EBUSY, EEXIST, ENOTEMPTY, ENOENT, ENOTDIR, EROFS);
        printf("EFAULT=%d  EISDIR=%d  ENAMETOOLONG=%d  ENODEV=%d  EINVAL=%d\n",
            EFAULT, EISDIR, ENAMETOOLONG, ENODEV, EINVAL); */
        /* printf("cannot open destination file: %s\n", to_name); */
        fclose(from_file);
        *err_info = FILE_ERROR;
      } /* if */
    } else {
      /* printf("errno=%d\n", errno);
      printf("EACCES=%d  EBUSY=%d  EEXIST=%d  ENOTEMPTY=%d  ENOENT=%d  ENOTDIR=%d  EROFS=%d\n",
          EACCES, EBUSY, EEXIST, ENOTEMPTY, ENOENT, ENOTDIR, EROFS);
      printf("EFAULT=%d  EISDIR=%d  ENAMETOOLONG=%d  ENODEV=%d  EINVAL=%d\n",
          EFAULT, EISDIR, ENAMETOOLONG, ENODEV, EINVAL); */
      /* printf("cannot open source file: %s\n", from_name); */
      *err_info = FILE_ERROR;
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("END copy_file(\"%s\", \"%s\", %d)\n",
        from_name, to_name, *err_info);
#endif
  } /* copy_file */



#ifdef ANSI_C

static void copy_dir (os_stritype from_name, os_stritype to_name,
    int flags, errinfotype *err_info)
#else

static void copy_dir (from_name, to_name, flags, err_info)
os_stritype from_name;
os_stritype to_name;
int flags;
errinfotype *err_info;
#endif

  {
    os_DIR *directory;
    os_dirent_struct *current_entry;
    size_t from_name_size;
    size_t to_name_size;
    size_t d_name_size;
    size_t from_path_capacity = 0;
    os_stritype from_path = NULL;
    size_t to_path_capacity = 0;
    os_stritype to_path = NULL;
    size_t new_size;
    os_stritype resized_path;
    booltype init_path = TRUE;

  /* copy_dir */
#ifdef TRACE_CMD_RTL
    printf("BEGIN copy_dir(\"%s\", \"%s\")\n", from_name, to_name);
#endif
    /* printf("opendir(%s);\n", from_name);
       fflush(stdout); */
    if ((directory = os_opendir(from_name)) == NULL) {
      *err_info = FILE_ERROR;
    } else {
      if (os_mkdir(to_name, (S_IRWXU | S_IRWXG | S_IRWXO)) != 0) {
        *err_info = FILE_ERROR;
      } else {
        do {
          current_entry = os_readdir(directory);
          /* printf("$%ld$\n", (long) current_entry);
             fflush(stdout); */
        } while (current_entry != NULL &&
            (memcmp(current_entry->d_name, dot,    sizeof(os_chartype) * 2) == 0 ||
             memcmp(current_entry->d_name, dotdot, sizeof(os_chartype) * 3) == 0));
        from_name_size = os_stri_strlen(from_name);
        to_name_size = os_stri_strlen(to_name);
        while (*err_info == OKAY_NO_ERROR && current_entry != NULL) {
          /* printf("!%s!\n", current_entry->d_name);
             fflush(stdout); */
          d_name_size = os_stri_strlen(current_entry->d_name);
          new_size = from_name_size + 1 + d_name_size;
          if (new_size > from_path_capacity) {
            resized_path = os_stri_realloc(from_path, new_size);
            if (resized_path != NULL) {
              from_path = resized_path;
              from_path_capacity = new_size;
            } else if (from_path != NULL) {
              os_stri_free(from_path);
              from_path = NULL;
            } /* if */
          } /* if */
          new_size = to_name_size + 1 + d_name_size;
          if (new_size > to_path_capacity) {
            resized_path = os_stri_realloc(to_path, new_size);
            if (resized_path != NULL) {
              to_path = resized_path;
              to_path_capacity = new_size;
            } else if (to_path != NULL) {
              os_stri_free(to_path);
              to_path = NULL;
            } /* if */
          } /* if */
          if (from_path != NULL && to_path != NULL) {
            if (init_path) {
              os_stri_strcpy(from_path, from_name);
              os_stri_strcpy(&from_path[from_name_size], slash);
              os_stri_strcpy(to_path, to_name);
              os_stri_strcpy(&to_path[to_name_size], slash);
              init_path = FALSE;
            } /* if */
            os_stri_strcpy(&from_path[from_name_size + 1], current_entry->d_name);
            os_stri_strcpy(&to_path[to_name_size + 1], current_entry->d_name);
            copy_any_file(from_path, to_path, flags, err_info);
          } else {
            *err_info = MEMORY_ERROR;
          } /* if */
          do {
            current_entry = os_readdir(directory);
            /* printf("$%ld$\n", (long) current_entry);
               fflush(stdout); */
          } while (current_entry != NULL &&
              (memcmp(current_entry->d_name, dot,    sizeof(os_chartype) * 2) == 0 ||
               memcmp(current_entry->d_name, dotdot, sizeof(os_chartype) * 3) == 0));
        } /* while */
        if (*err_info != OKAY_NO_ERROR) {
          remove_dir(to_name, err_info);
        } /* if */
        if (from_path != NULL) {
          os_stri_free(from_path);
        } /* if */
        if (to_path != NULL) {
          os_stri_free(to_path);
        } /* if */
      } /* if */
      os_closedir(directory);
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("END copy_dir(\"%s\", \"%s\", %d)\n",
        from_name, to_name, *err_info);
#endif
  } /* copy_dir */



#ifdef ANSI_C

static void copy_any_file (os_stritype from_name, os_stritype to_name,
    int flags, errinfotype *err_info)
#else

static void copy_any_file (from_name, to_name, flags, err_info)
os_stritype from_name;
os_stritype to_name;
int flags;
errinfotype *err_info;
#endif

  {
    os_stat_struct from_stat;
    int from_stat_result;
#ifdef HAS_SYMLINKS
    os_stritype link_destination;
    ssize_t readlink_result;
#endif
    os_utimbuf_struct to_utime;

  /* copy_any_file */
#ifdef TRACE_CMD_RTL
    printf("BEGIN copy_any_file(\"%s\", \"%s\")\n", from_name, to_name);
#endif
    if (flags & PRESERVE_SYMLINKS) {
      from_stat_result = os_lstat(from_name, &from_stat);
    } else {
      from_stat_result = os_stat(from_name, &from_stat);
    } /* if */
    if (from_stat_result != 0) {
      /* Source file does not exist */
      *err_info = FILE_ERROR;
    } else {
      if (S_ISLNK(from_stat.st_mode)) {
#ifdef HAS_SYMLINKS
        /* printf("link size=%lu\n", from_stat.st_size); */
        if (from_stat.st_size > MAX_SYMLINK_PATH_LENGTH || from_stat.st_size < 0) {
          *err_info = RANGE_ERROR;
        } else {
          if (!os_stri_alloc(link_destination, from_stat.st_size)) {
            *err_info = MEMORY_ERROR;
          } else {
            readlink_result = readlink(from_name, link_destination,
                                       (size_t) from_stat.st_size);
            if (readlink_result != -1) {
              link_destination[readlink_result] = '\0';
              /* printf("readlink_result=%lu\n", readlink_result);
                 printf("link=%s\n", link_destination); */
              if (symlink(link_destination, to_name) != 0) {
                *err_info = FILE_ERROR;
              } /* if */
            } else {
              *err_info = FILE_ERROR;
            } /* if */
            os_stri_free(link_destination);
          } /* if */
        } /* if */
#else
        *err_info = FILE_ERROR;
#endif
      } else if (S_ISREG(from_stat.st_mode)) {
        copy_file(from_name, to_name, err_info);
      } else if (S_ISDIR(from_stat.st_mode)) {
        copy_dir(from_name, to_name, flags, err_info);
      } else if (S_ISFIFO(from_stat.st_mode)) {
#ifdef HAS_FIFO_FILES
        if (mkfifo(to_name, (S_IRWXU | S_IRWXG | S_IRWXO)) != 0) {
          *err_info = FILE_ERROR;
        } /* if */
#else
        *err_info = FILE_ERROR;
#endif
      } else {
        *err_info = FILE_ERROR;
      } /* if */
      if (*err_info == OKAY_NO_ERROR && !S_ISLNK(from_stat.st_mode)) {
        if (flags & PRESERVE_TIMESTAMPS) {
          to_utime.actime = from_stat.st_atime;
          to_utime.modtime = from_stat.st_mtime;
          /* printf("copy_any_file: st_atime=%ld\n", from_stat.st_atime); */
          /* printf("copy_any_file: st_mtime=%ld\n", from_stat.st_mtime); */
          os_utime(to_name, &to_utime);
        } /* if */
        if (flags & PRESERVE_MODE) {
          os_chmod(to_name, from_stat.st_mode);
        } /* if */
        if (flags & PRESERVE_OWNERSHIP) {
          os_chown(to_name, from_stat.st_uid, from_stat.st_gid);
        } /* if */
      } /* if */
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("END copy_any_file(\"%s\", \"%s\", %d)\n",
        from_name, to_name, *err_info);
#endif
  } /* copy_any_file */



#ifdef ANSI_C

static void move_any_file (os_stritype from_name, os_stritype to_name, errinfotype *err_info)
#else

static void move_any_file (from_name, to_name, err_info)
os_stritype from_name;
os_stritype to_name;
errinfotype *err_info;
#endif

  {
    os_stat_struct to_stat;

  /* move_any_file */
#ifdef TRACE_CMD_RTL
    printf("BEGIN move_any_file(\"%s\", \"%s\")\n", from_name, to_name);
#endif
    if (os_stat(to_name, &to_stat) == 0) {
      /* Destination file exists already */
      *err_info = FILE_ERROR;
    } else {
      if (os_rename(from_name, to_name) != 0) {
        switch (errno) {
          case EXDEV:
            copy_any_file(from_name, to_name, PRESERVE_ALL, err_info);
            if (*err_info == OKAY_NO_ERROR) {
              remove_any_file(from_name, err_info);
            } /* if */
            break;
          default:
            /* printf("errno=%d\n", errno);
            printf("EACCES=%d  EBUSY=%d  EEXIST=%d  ENOTEMPTY=%d  ENOENT=%d  ENOTDIR=%d  EROFS=%d\n",
                EACCES, EBUSY, EEXIST, ENOTEMPTY, ENOENT, ENOTDIR, EROFS); */
            *err_info = FILE_ERROR;
            break;
        } /* switch */
      } /* if */
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("END move_any_file(\"%s\", \"%s\", %d)\n",
        from_name, to_name, *err_info);
#endif
  } /* move_any_file */



#ifdef ANSI_C

static rtlArraytype add_stri_to_array (const stritype stri,
    rtlArraytype work_array, inttype *used_max_position, errinfotype *err_info)
#else

static rtlArraytype add_stri_to_array (stri, work_array, used_max_position, err_info)
stritype stri;
rtlArraytype work_array;
inttype *used_max_position;
errinfotype *err_info;
#endif

  {
    rtlArraytype resized_work_array;

  /* add_stri_to_array */
    /* printf("add_stri_to_array\n"); */
    if (*used_max_position >= work_array->max_position) {
      if (work_array->max_position >= MAX_MEM_INDEX - ARRAY_SIZE_DELTA) {
        resized_work_array = NULL;
      } else {
        resized_work_array = REALLOC_RTL_ARRAY(work_array,
            (uinttype) work_array->max_position,
            (uinttype) work_array->max_position + ARRAY_SIZE_DELTA);
      } /* if */
      if (resized_work_array == NULL) {
        *err_info = MEMORY_ERROR;
      } else {
        work_array = resized_work_array;
        COUNT3_RTL_ARRAY((uinttype) work_array->max_position,
            (uinttype) work_array->max_position + ARRAY_SIZE_DELTA);
        work_array->max_position += ARRAY_SIZE_DELTA;
      } /* if */
    } /* if */
    if (*err_info == OKAY_NO_ERROR) {
      work_array->arr[*used_max_position].value.strivalue = stri;
      (*used_max_position)++;
    } /* if */
    return work_array;
  } /* add_stri_to_array */



#ifdef ANSI_C

static rtlArraytype complete_stri_array (rtlArraytype work_array, inttype used_max_position,
    errinfotype *err_info)
#else

static rtlArraytype complete_stri_array (work_array, used_max_position, err_info)
rtlArraytype work_array;
inttype used_max_position;
errinfotype *err_info;
#endif

  {
    rtlArraytype resized_work_array;
    memsizetype position;

  /* complete_stri_array */
    if (*err_info == OKAY_NO_ERROR) {
      resized_work_array = REALLOC_RTL_ARRAY(work_array,
          (uinttype) work_array->max_position, (uinttype) used_max_position);
      if (resized_work_array == NULL) {
        *err_info = MEMORY_ERROR;
      } else {
        work_array = resized_work_array;
        COUNT3_RTL_ARRAY((uinttype) work_array->max_position, (uinttype) used_max_position);
        work_array->max_position = used_max_position;
      } /* if */
    } /* if */
    if (*err_info != OKAY_NO_ERROR) {
      for (position = 0; position < (memsizetype) used_max_position; position++) {
        FREE_STRI(work_array->arr[position].value.strivalue,
            work_array->arr[position].value.strivalue->size);
      } /* for */
      FREE_RTL_ARRAY(work_array, (uinttype) work_array->max_position);
      work_array = NULL;
    } /* if */
    return work_array;
  } /* complete_stri_array */



#ifdef ANSI_C

static rtlArraytype read_dir (const const_stritype dir_name, errinfotype *err_info)
#else

static rtlArraytype read_dir (dir_name, err_info)
stritype dir_name;
errinfotype *err_info;
#endif

  {
    inttype used_max_position;
    dirtype directory;
    stritype nameStri;
    rtlArraytype dir_array;

  /* read_dir */
/*  printf("read_dir(");
    prot_stri(dir_name);
    printf(");\n");
    fflush(stdout); */
    if ((directory = dirOpen(dir_name)) != NULL) {
      if (ALLOC_RTL_ARRAY(dir_array, INITAL_ARRAY_SIZE)) {
        dir_array->min_position = 1;
        dir_array->max_position = INITAL_ARRAY_SIZE;
        used_max_position = 0;
        nameStri = dirRead(directory);
        while (*err_info == OKAY_NO_ERROR && nameStri != NULL) {
          dir_array = add_stri_to_array(nameStri, dir_array,
              &used_max_position, err_info);
          nameStri = dirRead(directory);
        } /* while */
        dir_array = complete_stri_array(dir_array, used_max_position, err_info);
      } else {
        *err_info = MEMORY_ERROR;
      } /* if */
      dirClose(directory);
    } else {
      dir_array = NULL;
      *err_info = FILE_ERROR;
    } /* if */
    return dir_array;
  } /* read_dir */



#ifdef ANSI_C

static rtlArraytype getSearchPath (errinfotype *err_info)
#else

static rtlArraytype getSearchPath (err_info)
errinfotype *err_info;
#endif

  {
    static const os_chartype path[] = {'P', 'A', 'T', 'H', 0};
    os_stritype path_environment_variable;
    memsizetype path_length;
    os_stritype path_copy;
    os_stritype path_start;
    os_stritype path_end;
    stritype pathStri;
    inttype used_max_position;
    rtlArraytype path_array;

  /* getSearchPath */
    /* printf("getSearchPath\n"); */
    if (ALLOC_RTL_ARRAY(path_array, INITAL_ARRAY_SIZE)) {
      path_array->min_position = 1;
      path_array->max_position = INITAL_ARRAY_SIZE;
      used_max_position = 0;
      path_environment_variable = os_getenv(path);
      if (path_environment_variable != NULL) {
        path_length = os_stri_strlen(path_environment_variable);
        if (unlikely(!os_stri_alloc(path_copy, path_length))) {
          *err_info = MEMORY_ERROR;
        } else {
          os_stri_strcpy(path_copy, path_environment_variable);
          path_start = path_copy;
          do {
            path_end = os_stri_strchr(path_start, SEARCH_PATH_DELIMITER);
            if (path_end != NULL) {
              *path_end = '\0';
            } /* if */
            pathStri = cp_from_os_path(path_start, err_info);
            if (*err_info == OKAY_NO_ERROR) {
              while (pathStri->size > 1 && pathStri->mem[pathStri->size - 1] == (chartype) '/') {
                pathStri->size--;
#ifdef WITH_STRI_CAPACITY
                COUNT3_STRI(pathStri->size + 1, pathStri->size);
#endif
              } /* while */
              path_array = add_stri_to_array(pathStri, path_array,
                  &used_max_position, err_info);
            } /* if */
            if (path_end == NULL) {
              path_start = NULL;
            } else {
              path_start = path_end + 1;
            } /* if */
          } while (path_start != NULL && *err_info == OKAY_NO_ERROR);
          os_stri_free(path_copy);
        } /* if */
        os_getenv_string_free(path_environment_variable);
        path_array = complete_stri_array(path_array, used_max_position, err_info);
      } /* if */
    } else {
      *err_info = MEMORY_ERROR;
    } /* if */
    return path_array;
  } /* getSearchPath */



#ifdef HAS_SYMLINKS
#ifdef ANSI_C

stritype followLink (stritype path)
#else

stritype followLink (path)
stritype path;
#endif

  {
    stritype startPath;
    stritype helpPath;
    int count = 5;

  /* followLink */
    if (cmdFileTypeSL(path) == 7) {
      /* printf("symbolic link: ");
         prot_stri(path);
         printf("\n"); */
      startPath = path;
      path = cmdReadlink(startPath);
      while (cmdFileTypeSL(path) == 7 && count != 0) {
        helpPath = path;
        path = cmdReadlink(helpPath);
        FREE_STRI(helpPath, helpPath->size);
        count--;
      } /* if */
      if (count == 0) {
        FREE_STRI(path, path->size);
        path = startPath;
      } else {
        FREE_STRI(startPath, startPath->size);
      } /* if */
    } /* if */
    return path;
  } /* followLink */
#endif



#ifdef EMULATE_ROOT_CWD
#ifdef ANSI_C

void initEmulatedCwd (errinfotype *err_info)
#else

void initEmulatedCwd (err_info)
errinfotype *err_info;
#endif

  {
    os_chartype buffer[PATH_MAX + 1];
    os_stritype cwd;
    memsizetype cwd_len;
    os_stritype new_cwd;

  /* initEmulatedCwd */
    if ((cwd = os_getcwd(buffer, PATH_MAX)) == NULL) {
      *err_info = FILE_ERROR;
    } else {
      cwd_len = os_stri_strlen(cwd);
      if (likely(!os_stri_alloc(new_cwd, cwd_len))) {
        *err_info = MEMORY_ERROR;
      } else {
        memcpy(new_cwd, cwd, (cwd_len + 1) * sizeof(os_chartype));
        setEmulatedCwd(new_cwd);
      } /* if */
    } /* if */
  } /* initEmulatedCwd */
#endif



#ifdef ANSI_C

biginttype cmdBigFileSize (const const_stritype file_name)
#else

biginttype cmdBigFileSize (file_name)
stritype file_name;
#endif

  {
    os_stritype os_path;
    os_stat_struct stat_buf;
    int stat_result;
    filetype aFile;
    int path_info = PATH_IS_NORMAL;
    errinfotype err_info = OKAY_NO_ERROR;
    biginttype result;

  /* cmdBigFileSize */
    os_path = cp_to_os_path(file_name, &path_info, &err_info);
    if (unlikely(err_info != OKAY_NO_ERROR)) {
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        result = bigIConv(0);
      } else {
        raise_error(err_info);
        result = NULL;
      } /* if */
#else
      raise_error(err_info);
      result = NULL;
#endif
    } else {
      stat_result = os_stat(os_path, &stat_buf);
      if (stat_result == 0 && S_ISREG(stat_buf.st_mode)) {
#if OS_OFF_T_SIZE == 32
        result = bigFromUInt32((uint32type) stat_buf.st_size);
#elif OS_OFF_T_SIZE == 64
        result = bigFromUInt64((uint64type) stat_buf.st_size);
#else
#error "sizeof(os_off_t) is neither 4 nor 8."
#endif
      } else if (stat_result == 0 && S_ISDIR(stat_buf.st_mode)) {
        result = bigIConv(0);
      } else {
        aFile = os_fopen(os_path, os_mode_rb);
        if (aFile == NULL) {
          /* if (stat_result == 0) {
            printf("stat_buf.st_blksize=%lu\n", stat_buf.st_blksize);
            printf("stat_buf.st_blocks=%llu\n", stat_buf.st_blocks);
            printf("stat_buf.st_size=%llu\n", stat_buf.st_size);
          } */
          err_info = FILE_ERROR;
          result = NULL;
        } else {
          result = getBigFileLengthUsingSeek(aFile);
          fclose(aFile);
        } /* if */
      } /* if */
      os_stri_free(os_path);
      if (unlikely(err_info != OKAY_NO_ERROR)) {
        raise_error(err_info);
      } /* if */
    } /* if */
    return result;
  } /* cmdBigFileSize */



#ifdef ANSI_C

void cmdChdir (const const_stritype dir_name)
#else

void cmdChdir (dir_name)
stritype dir_name;
#endif

  {
    os_stritype os_path;
    int path_info = PATH_IS_NORMAL;
    errinfotype err_info = OKAY_NO_ERROR;
    int chdir_result;

  /* cmdChdir */
    os_path = cp_to_os_path(dir_name, &path_info, &err_info);
    if (unlikely(err_info != OKAY_NO_ERROR)) {
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        setEmulatedCwd(emulated_root);
      } else {
        raise_error(err_info);
      } /* if */
    } else {
      chdir_result = os_chdir(os_path);
      if (unlikely(chdir_result != 0)) {
        os_stri_free(os_path);
        err_info = FILE_ERROR;
      } else {
        if (dir_name->size >= 1 && dir_name->mem[0] == '/') {
          setEmulatedCwd(os_path);
        } else {
          os_stri_free(os_path);
          initEmulatedCwd(&err_info);
        } /* if */
      } /* if */
#else
      raise_error(err_info);
    } else {
      chdir_result = os_chdir(os_path);
      if (unlikely(chdir_result != 0)) {
        err_info = FILE_ERROR;
      } /* if */
      os_stri_free(os_path);
#endif
      if (unlikely(err_info != OKAY_NO_ERROR)) {
        raise_error(err_info);
      } /* if */
    } /* if */
  } /* cmdChdir */



#ifdef ANSI_C

void cmdCloneFile (const const_stritype source_name, const const_stritype dest_name)
#else

void cmdCloneFile (source_name, dest_name)
stritype source_name;
stritype dest_name;
#endif

  {
    os_stritype os_source_name;
    os_stritype os_dest_name;
    os_stat_struct to_stat;
    int path_info;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdCloneFile */
    os_source_name = cp_to_os_path(source_name, &path_info, &err_info);
    if (likely(err_info == OKAY_NO_ERROR)) {
      os_dest_name = cp_to_os_path(dest_name, &path_info, &err_info);
      if (likely(err_info == OKAY_NO_ERROR)) {
        if (os_stat(os_dest_name, &to_stat) == 0) {
          /* Destination file exists already */
          err_info = FILE_ERROR;
        } else {
          copy_any_file(os_source_name, os_dest_name, PRESERVE_ALL, &err_info);
        } /* if */
        os_stri_free(os_dest_name);
      } /* if */
      os_stri_free(os_source_name);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdCloneFile */



#ifdef ANSI_C

stritype cmdConfigValue (const const_stritype name)
#else

stritype cmdConfigValue (name)
stritype name;
#endif

  {
    char opt_name[250];
    const_cstritype opt;
    char buffer[4];
    stritype result;

  /* cmdConfigValue */
    if (name->size >= 250 / MAX_UTF8_EXPANSION_FACTOR) {
      opt = "";
    } else {
      stri_export_utf8((ustritype) opt_name, name);
      if (strcmp(opt_name, "OBJECT_FILE_EXTENSION") == 0) {
        opt = OBJECT_FILE_EXTENSION;
      } else if (strcmp(opt_name, "LIBRARY_FILE_EXTENSION") == 0) {
        opt = LIBRARY_FILE_EXTENSION;
      } else if (strcmp(opt_name, "EXECUTABLE_FILE_EXTENSION") == 0) {
        opt = EXECUTABLE_FILE_EXTENSION;
      } else if (strcmp(opt_name, "C_COMPILER") == 0) {
#ifdef PATHS_RELATIVE_TO_EXECUTABLE
        result = relativeToProgramPath(programPath, "bin/call_gcc");
        opt = NULL;
#else
        opt = C_COMPILER;
#endif
      } else if (strcmp(opt_name, "CPLUSPLUS_COMPILER") == 0) {
#ifdef PATHS_RELATIVE_TO_EXECUTABLE
        result = relativeToProgramPath(programPath, "bin/call_gcc");
        opt = NULL;
#else
        opt = CPLUSPLUS_COMPILER;
#endif
      } else if (strcmp(opt_name, "C_COMPILER_VERSION") == 0) {
        opt = C_COMPILER_VERSION;
      } else if (strcmp(opt_name, "GET_CC_VERSION_INFO") == 0) {
        opt = GET_CC_VERSION_INFO;
      } else if (strcmp(opt_name, "CC_OPT_DEBUG_INFO") == 0) {
        opt = CC_OPT_DEBUG_INFO;
      } else if (strcmp(opt_name, "CC_OPT_NO_WARNINGS") == 0) {
        opt = CC_OPT_NO_WARNINGS;
      } else if (strcmp(opt_name, "CC_FLAGS") == 0) {
        opt = CC_FLAGS;
      } else if (strcmp(opt_name, "REDIRECT_C_ERRORS") == 0) {
        opt = REDIRECT_C_ERRORS;
      } else if (strcmp(opt_name, "LINKER_OPT_DEBUG_INFO") == 0) {
        opt = LINKER_OPT_DEBUG_INFO;
      } else if (strcmp(opt_name, "LINKER_OPT_NO_DEBUG_INFO") == 0) {
        opt = LINKER_OPT_NO_DEBUG_INFO;
      } else if (strcmp(opt_name, "LINKER_OPT_OUTPUT_FILE") == 0) {
        opt = LINKER_OPT_OUTPUT_FILE;
      } else if (strcmp(opt_name, "LINKER_FLAGS") == 0) {
        opt = LINKER_FLAGS;
      } else if (strcmp(opt_name, "SYSTEM_LIBS") == 0) {
        opt = SYSTEM_LIBS;
      } else if (strcmp(opt_name, "SYSTEM_CONSOLE_LIBS") == 0) {
        opt = SYSTEM_CONSOLE_LIBS;
      } else if (strcmp(opt_name, "SYSTEM_DRAW_LIBS") == 0) {
        opt = SYSTEM_DRAW_LIBS;
      } else if (strcmp(opt_name, "SEED7_LIB") == 0) {
        opt = SEED7_LIB;
      } else if (strcmp(opt_name, "CONSOLE_LIB") == 0) {
        opt = CONSOLE_LIB;
      } else if (strcmp(opt_name, "DRAW_LIB") == 0) {
        opt = DRAW_LIB;
      } else if (strcmp(opt_name, "COMP_DATA_LIB") == 0) {
        opt = COMP_DATA_LIB;
      } else if (strcmp(opt_name, "COMPILER_LIB") == 0) {
        opt = COMPILER_LIB;
      } else if (strcmp(opt_name, "S7_LIB_DIR") == 0) {
#ifdef PATHS_RELATIVE_TO_EXECUTABLE
        result = relativeToProgramPath(programPath, "bin");
        opt = NULL;
#else
        opt = S7_LIB_DIR;
#endif
      } else if (strcmp(opt_name, "INT32TYPE") == 0) {
        opt = INT32TYPE_STRI;
      } else if (strcmp(opt_name, "UINT32TYPE") == 0) {
        opt = UINT32TYPE_STRI;
      } else if (strcmp(opt_name, "INT64TYPE") == 0) {
        opt = INT64TYPE_STRI;
      } else if (strcmp(opt_name, "UINT64TYPE") == 0) {
        opt = UINT64TYPE_STRI;
      } else if (strcmp(opt_name, "INT32TYPE_LITERAL_SUFFIX") == 0) {
        opt = INT32TYPE_LITERAL_SUFFIX;
      } else if (strcmp(opt_name, "INT64TYPE_LITERAL_SUFFIX") == 0) {
        opt = INT64TYPE_LITERAL_SUFFIX;
      } else if (strcmp(opt_name, "INTTYPE_SIZE") == 0) {
        sprintf(buffer, "%d", INTTYPE_SIZE);
        opt = buffer;
      } else if (strcmp(opt_name, "FLOATTYPE_SIZE") == 0) {
        sprintf(buffer, "%d", FLOATTYPE_SIZE);
        opt = buffer;
      } else if (strcmp(opt_name, "POINTER_SIZE") == 0) {
        sprintf(buffer, "%d", POINTER_SIZE);
        opt = buffer;
      } else if (strcmp(opt_name, "FLOATTYPE_DOUBLE") == 0) {
#ifdef FLOATTYPE_DOUBLE
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "USE_SIGSETJMP") == 0) {
#ifdef USE_SIGSETJMP
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "ISNAN_WITH_UNDERLINE") == 0) {
#ifdef ISNAN_WITH_UNDERLINE
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "CHECK_INT_DIV_BY_ZERO") == 0) {
#ifdef CHECK_INT_DIV_BY_ZERO
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "CHECK_FLOAT_DIV_BY_ZERO") == 0) {
#ifdef CHECK_FLOAT_DIV_BY_ZERO
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "SIGILL_ON_OVERFLOW") == 0) {
#ifdef SIGILL_ON_OVERFLOW
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "WITH_STRI_CAPACITY") == 0) {
#ifdef WITH_STRI_CAPACITY
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "ALLOW_STRITYPE_SLICES") == 0) {
#ifdef ALLOW_STRITYPE_SLICES
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "RSHIFT_DOES_SIGN_EXTEND") == 0) {
#ifdef RSHIFT_DOES_SIGN_EXTEND
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "TWOS_COMPLEMENT_INTTYPE") == 0) {
#ifdef TWOS_COMPLEMENT_INTTYPE
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "LITTLE_ENDIAN_INTTYPE") == 0) {
#ifdef LITTLE_ENDIAN_INTTYPE
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "NAN_COMPARISON_WRONG") == 0) {
#ifdef NAN_COMPARISON_WRONG
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "POWER_OF_ZERO_WRONG") == 0) {
#ifdef POWER_OF_ZERO_WRONG
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "FLOAT_ZERO_DIV_ERROR") == 0) {
#ifdef FLOAT_ZERO_DIV_ERROR
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "LIMITED_CSTRI_LITERAL_LEN") == 0) {
#ifdef LIMITED_CSTRI_LITERAL_LEN
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "CC_SOURCE_UTF8") == 0) {
#ifdef CC_SOURCE_UTF8
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else if (strcmp(opt_name, "USE_WMAIN") == 0) {
#ifdef USE_WMAIN
        opt = "TRUE";
#else
        opt = "FALSE";
#endif
      } else {
        opt = "";
      } /* if */
    } /* if */
    if (opt != NULL) {
      result = cstri8_or_cstri_to_stri(opt);
    } /* if */
    if (unlikely(result == NULL)) {
      raise_error(MEMORY_ERROR);
    } /* if */
    return result;
  } /* cmdConfigValue */



#ifdef ANSI_C

void cmdCopyFile (const const_stritype source_name, const const_stritype dest_name)
#else

void cmdCopyFile (source_name, dest_name)
stritype source_name;
stritype dest_name;
#endif

  {
    os_stritype os_source_name;
    os_stritype os_dest_name;
    os_stat_struct to_stat;
    int path_info;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdCopyFile */
    os_source_name = cp_to_os_path(source_name, &path_info, &err_info);
    if (likely(err_info == OKAY_NO_ERROR)) {
      os_dest_name = cp_to_os_path(dest_name, &path_info, &err_info);
      if (likely(err_info == OKAY_NO_ERROR)) {
        if (os_stat(os_dest_name, &to_stat) == 0) {
          /* Destination file exists already */
          err_info = FILE_ERROR;
        } else {
          copy_any_file(os_source_name, os_dest_name, PRESERVE_NOTHING, &err_info);
        } /* if */
        os_stri_free(os_dest_name);
      } /* if */
      os_stri_free(os_source_name);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdCopyFile */



#ifdef ANSI_C

rtlArraytype cmdEnvironment (void)
#else

rtlArraytype cmdEnvironment ()
#endif

  {
#ifdef INITIALIZE_OS_ENVIRON
    static const os_chartype empty_os_stri[] = {0};
#endif
    inttype used_max_position;
    os_stritype *nameStartPos;
    os_stritype nameEndPos;
    stritype variableName;
    errinfotype err_info = OKAY_NO_ERROR;
    rtlArraytype environment_array;

  /* cmdEnvironment */
#ifdef INITIALIZE_OS_ENVIRON
    if (os_environ == NULL) {
      os_getenv(empty_os_stri);
    } /* if */
#endif
    if (ALLOC_RTL_ARRAY(environment_array, INITAL_ARRAY_SIZE)) {
      environment_array->min_position = 1;
      environment_array->max_position = INITAL_ARRAY_SIZE;
      used_max_position = 0;
      for (nameStartPos = os_environ; *nameStartPos != NULL; ++nameStartPos) {
        if ((*nameStartPos)[0] != '=' && (*nameStartPos)[0] != '\0') {
          nameEndPos = os_stri_strchr(*nameStartPos, '=');
          if (nameEndPos != NULL) {
            variableName = conv_from_os_stri(*nameStartPos, (memsizetype) (nameEndPos - *nameStartPos));
            if (unlikely(variableName == NULL)) {
              err_info = MEMORY_ERROR;
            } /* if */
          } else {
            variableName = os_stri_to_stri(*nameStartPos, &err_info);
          } /* if */
          if (err_info == OKAY_NO_ERROR) {
            environment_array = add_stri_to_array(variableName, environment_array,
                &used_max_position, &err_info);
          } /* if */
        } /* if */
      } /* for */
      environment_array = complete_stri_array(environment_array, used_max_position, &err_info);
      if (err_info != OKAY_NO_ERROR) {
        raise_error(err_info);
        environment_array = NULL;
      } /* if */
    } else {
      raise_error(MEMORY_ERROR);
    } /* if */
    return environment_array;
  } /* cmdEnvironment */



#ifdef ANSI_C

settype cmdFileMode (const const_stritype file_name)
#else

settype cmdFileMode (file_name)
stritype file_name;
#endif

  {
    os_stritype os_path;
    os_stat_struct stat_buf;
    int stat_result;
    int path_info = PATH_IS_NORMAL;
    errinfotype err_info = OKAY_NO_ERROR;
    settype result;

  /* cmdFileMode */
#ifdef TRACE_CMD_RTL
    printf("BEGIN cmdFileMode(");
    prot_stri(file_name);
    printf(")\n");
#endif
    os_path = cp_to_os_path(file_name, &path_info, &err_info);
    if (unlikely(err_info != OKAY_NO_ERROR)) {
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        result = setIConv(0444);
      } else {
        raise_error(err_info);
        result = NULL;
      } /* if */
#else
      raise_error(err_info);
      result = NULL;
#endif
    } else {
      stat_result = os_stat(os_path, &stat_buf);
      os_stri_free(os_path);
      if (unlikely(stat_result != 0)) {
        /* printf("errno=%d\n", errno);
        printf("EACCES=%d  EBADF=%d  EFAULT=%d  ELOOP=%d  ENAMETOOLONG=%d  ENOENT=%d\n",
            EACCES, EBADF, EFAULT, ELOOP, ENAMETOOLONG, ENOENT);
        printf("ENOMEM=%d  EOVERFLOW=%d\n",
            ENOMEM, EOVERFLOW); */
        raise_error(FILE_ERROR);
        result = NULL;
      } else {
        /* printf("cmdFileMode: st_mode=0%o\n", stat_buf.st_mode); */
#if MODE_BITS_NORMAL
        result = setIConv(0777 & stat_buf.st_mode);
#else
        /* Force the bits to the standard sequence */
        result = setIConv(        {
            (stat_buf.st_mode & S_IRUSR ? 0400 : 0) |
            (stat_buf.st_mode & S_IWUSR ? 0200 : 0) |
            (stat_buf.st_mode & S_IXUSR ? 0100 : 0) |
            (stat_buf.st_mode & S_IRGRP ? 0040 : 0) |
            (stat_buf.st_mode & S_IWGRP ? 0020 : 0) |
            (stat_buf.st_mode & S_IXGRP ? 0010 : 0) |
            (stat_buf.st_mode & S_IROTH ? 0004 : 0) |
            (stat_buf.st_mode & S_IWOTH ? 0002 : 0) |
            (stat_buf.st_mode & S_IXOTH ? 0001 : 0));
#endif
      } /* if */
    } /* if */
    return result;
  } /* cmdFileMode */



#ifdef ANSI_C

inttype cmdFileSize (const const_stritype file_name)
#else

inttype cmdFileSize (file_name)
stritype file_name;
#endif

  {
    os_stritype os_path;
    os_stat_struct stat_buf;
    int stat_result;
    filetype aFile;
    int path_info = PATH_IS_NORMAL;
    errinfotype err_info = OKAY_NO_ERROR;
    inttype result;

  /* cmdFileSize */
    os_path = cp_to_os_path(file_name, &path_info, &err_info);
    if (unlikely(err_info != OKAY_NO_ERROR)) {
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info != PATH_IS_EMULATED_ROOT) {
        raise_error(err_info);
      } /* if */
#else
      raise_error(err_info);
#endif
      result = 0;
    } else {
      stat_result = os_stat(os_path, &stat_buf);
      if (stat_result == 0 && S_ISREG(stat_buf.st_mode)) {
        if (stat_buf.st_size > INTTYPE_MAX || stat_buf.st_size < 0) {
          err_info = RANGE_ERROR;
          result = 0;
        } else {
          result = (inttype) stat_buf.st_size;
        } /* if */
      } else if (stat_result == 0 && S_ISDIR(stat_buf.st_mode)) {
        result = 0;
      } else {
        aFile = os_fopen(os_path, os_mode_rb);
        if (aFile == NULL) {
          /* if (stat_result == 0) {
            printf("stat_buf.st_blksize=%lu\n", stat_buf.st_blksize);
            printf("stat_buf.st_blocks=%llu\n", stat_buf.st_blocks);
            printf("stat_buf.st_size=%llu\n", stat_buf.st_size);
          } */
          err_info = FILE_ERROR;
          result = 0;
        } else {
          result = getFileLengthUsingSeek(aFile);
          fclose(aFile);
        } /* if */
      } /* if */
      os_stri_free(os_path);
      if (unlikely(err_info != OKAY_NO_ERROR)) {
        raise_error(err_info);
      } /* if */
    } /* if */
    return result;
  } /* cmdFileSize */



#ifdef ANSI_C

inttype cmdFileType (const const_stritype file_name)
#else

inttype cmdFileType (file_name)
stritype file_name;
#endif

  {
    os_stritype os_path;
    os_stat_struct stat_buf;
    int stat_result;
    int path_info = PATH_IS_NORMAL;
    errinfotype err_info = OKAY_NO_ERROR;
    inttype result;

  /* cmdFileType */
    os_path = cp_to_os_path(file_name, &path_info, &err_info);
    if (unlikely(err_info != OKAY_NO_ERROR)) {
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        result = 3;
      } else if (path_info == PATH_NOT_MAPPED) {
        result = 0;
      } else {
        raise_error(err_info);
        result = 0;
      } /* if */
#else
      raise_error(err_info);
      result = 0;
#endif
    } else {
      stat_result = os_stat(os_path, &stat_buf);
      os_stri_free(os_path);
      if (stat_result == 0) {
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
          raise_error(FILE_ERROR);
        } else if (S_ISSOCK(stat_buf.st_mode)) {
          result = 8;
        } else {
          result = 1;
        } /* if */
      } else {
        result = 0;
        if (unlikely(file_name->size != 0 && errno != ENOENT &&
            errno != ENOTDIR && errno != ENAMETOOLONG)) {
          /* printf("errno=%d\n", errno);
          printf("EACCES=%d  EBUSY=%d  EEXIST=%d  ENOTEMPTY=%d  ENOENT=%d\n",
              EACCES, EBUSY, EEXIST, ENOTEMPTY, ENOENT);
          printf("ENOTDIR=%d  EROFS=%d  EIO=%d  ELOOP=%d  ENAMETOOLONG=%d\n",
              ENOTDIR, EROFS, EIO, ELOOP, ENAMETOOLONG);
          printf("EOVERFLOW=%d  EBADF=%d  EFAULT=%d  ENOMEM=%d\n",
              EOVERFLOW, EBADF, EFAULT, ENOMEM); */
          /* printf("file_name->size=%lu\n", file_name->size); */
          /* printf("strlen(os_path)=%d\n", os_stri_strlen(os_path)); */
          raise_error(FILE_ERROR);
        } /* if */
      } /* if */
    } /* if */
    return result;
  } /* cmdFileType */



#ifdef ANSI_C

inttype cmdFileTypeSL (const const_stritype file_name)
#else

inttype cmdFileTypeSL (file_name)
stritype file_name;
#endif

  {
    os_stritype os_path;
    os_stat_struct stat_buf;
    int stat_result;
    int path_info = PATH_IS_NORMAL;
    errinfotype err_info = OKAY_NO_ERROR;
    inttype result;

  /* cmdFileTypeSL */
    os_path = cp_to_os_path(file_name, &path_info, &err_info);
    if (unlikely(err_info != OKAY_NO_ERROR)) {
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        result = 3;
      } else if (path_info == PATH_NOT_MAPPED) {
        result = 0;
      } else {
        raise_error(err_info);
        result = 0;
      } /* if */
#else
      raise_error(err_info);
      result = 0;
#endif
    } else {
      stat_result = os_lstat(os_path, &stat_buf);
      os_stri_free(os_path);
      if (stat_result == 0) {
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
        if (unlikely(file_name->size != 0 && errno != ENOENT &&
            errno != ENOTDIR && errno != ENAMETOOLONG)) {
          /* printf("errno=%d\n", errno);
          printf("EACCES=%d  EBUSY=%d  EEXIST=%d  ENOTEMPTY=%d  ENOENT=%d\n",
              EACCES, EBUSY, EEXIST, ENOTEMPTY, ENOENT);
          printf("ENOTDIR=%d  EROFS=%d  EIO=%d  ELOOP=%d  ENAMETOOLONG=%d\n",
              ENOTDIR, EROFS, EIO, ELOOP, ENAMETOOLONG);
          printf("EOVERFLOW=%d  EBADF=%d  EFAULT=%d  ENOMEM=%d\n",
              EOVERFLOW, EBADF, EFAULT, ENOMEM); */
          /* printf("file_name->size=%lu\n", file_name->size); */
          /* printf("strlen(os_path)=%d\n", os_stri_strlen(os_path)); */
          raise_error(FILE_ERROR);
        } /* if */
      } /* if */
    } /* if */
    return result;
  } /* cmdFileTypeSL */



#ifdef ANSI_C

stritype cmdGetcwd (void)
#else

stritype cmdGetcwd ()
#endif

  {
    os_chartype buffer[PATH_MAX + 1];
    os_stritype cwd;
    errinfotype err_info = OKAY_NO_ERROR;
    stritype result;

  /* cmdGetcwd */
#ifdef EMULATE_ROOT_CWD
    if (IS_EMULATED_ROOT(current_emulated_cwd)) {
      result = cp_from_os_path(current_emulated_cwd, &err_info);
      if (unlikely(err_info != OKAY_NO_ERROR)) {
        raise_error(err_info);
      } /* if */
    } else {
#endif
      if (unlikely((cwd = os_getcwd(buffer, PATH_MAX)) == NULL)) {
        raise_error(FILE_ERROR);
        result = NULL;
      } else {
        result = cp_from_os_path(cwd, &err_info);
        if (unlikely(err_info != OKAY_NO_ERROR)) {
          raise_error(err_info);
        } /* if */
      } /* if */
#ifdef EMULATE_ROOT_CWD
    } /* if */
#endif
    return result;
  } /* cmdGetcwd */



#ifdef ANSI_C

stritype cmdGetenv (const const_stritype name)
#else

stritype cmdGetenv (name)
stritype name;
#endif

  {
    os_stritype env_name;
    os_stritype env_value;
    errinfotype err_info = OKAY_NO_ERROR;
    stritype result;

  /* cmdGetenv */
    env_name = stri_to_os_stri(name, &err_info);
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
      result = NULL;
    } else {
      env_value = os_getenv(env_name);
      os_stri_free(env_name);
      if (env_value == NULL) {
        result = cstri_to_stri("");
        if (unlikely(result == NULL)) {
          err_info = MEMORY_ERROR;
        } /* if */
      } else {
        result = os_stri_to_stri(env_value, &err_info);
        os_getenv_string_free(env_value);
      } /* if */
      if (unlikely(err_info != OKAY_NO_ERROR)) {
        raise_error(err_info);
      } /* if */
    } /* if */
    return result;
  } /* cmdGetenv */



#ifdef ANSI_C

void cmdGetATime (const const_stritype file_name,
    inttype *year, inttype *month, inttype *day, inttype *hour,
    inttype *min, inttype *sec, inttype *micro_sec, inttype *time_zone,
    booltype *is_dst)
#else

void cmdGetATime (file_name,
    year, month, day, hour, min, sec, micro_sec, time_zone, is_dst)
stritype file_name;
inttype *year;
inttype *month;
inttype *day;
inttype *hour;
inttype *min;
inttype *sec;
inttype *micro_sec;
inttype *time_zone;
booltype *is_dst;
#endif

  {
    os_stritype os_path;
    os_stat_struct stat_buf;
    int stat_result;
    int path_info = PATH_IS_NORMAL;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdGetATime */
#ifdef TRACE_CMD_RTL
    printf("BEGIN cmdGetATime(");
    prot_stri(file_name);
    printf(")\n");
#endif
    os_path = cp_to_os_path(file_name, &path_info, &err_info);
    if (unlikely(err_info != OKAY_NO_ERROR)) {
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        timFromTimestamp(0,
            year, month, day, hour,
            min, sec, micro_sec, time_zone, is_dst);
      } else {
        raise_error(err_info);
      } /*if */
#else
      raise_error(err_info);
#endif
    } else {
      stat_result = os_stat(os_path, &stat_buf);
      os_stri_free(os_path);
      if (unlikely(stat_result != 0)) {
        /* printf("errno=%d\n", errno); */
        raise_error(FILE_ERROR);
      } else {
        /* printf("cmdGetATime: st_atime=%ld\n", stat_buf.st_atime); */
        timFromTimestamp(stat_buf.st_atime,
            year, month, day, hour,
            min, sec, micro_sec, time_zone, is_dst);
      } /* if */
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("END cmdGetATime(%04ld-%02ld-%02ld %02ld:%02ld:%02ld.%06ld %ld %d)\n",
        *year, *month, *day, *hour, *min, *sec,
        *micro_sec, *time_zone, *is_dst);
#endif
  } /* cmdGetATime */



#ifdef ANSI_C

void cmdGetCTime (const const_stritype file_name,
    inttype *year, inttype *month, inttype *day, inttype *hour,
    inttype *min, inttype *sec, inttype *micro_sec, inttype *time_zone,
    booltype *is_dst)
#else

void cmdGetCTime (file_name,
    year, month, day, hour, min, sec, micro_sec, time_zone, is_dst)
stritype file_name;
inttype *year;
inttype *month;
inttype *day;
inttype *hour;
inttype *min;
inttype *sec;
inttype *micro_sec;
inttype *time_zone;
booltype *is_dst;
#endif

  {
    os_stritype os_path;
    os_stat_struct stat_buf;
    int stat_result;
    int path_info = PATH_IS_NORMAL;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdGetCTime */
#ifdef TRACE_CMD_RTL
    printf("BEGIN cmdGetCTime(");
    prot_stri(file_name);
    printf(")\n");
#endif
    os_path = cp_to_os_path(file_name, &path_info, &err_info);
    if (unlikely(err_info != OKAY_NO_ERROR)) {
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        timFromTimestamp(0,
            year, month, day, hour,
            min, sec, micro_sec, time_zone, is_dst);
      } else {
        raise_error(err_info);
      } /*if */
#else
      raise_error(err_info);
#endif
    } else {
      stat_result = os_stat(os_path, &stat_buf);
      os_stri_free(os_path);
      if (unlikely(stat_result != 0)) {
        /* printf("errno=%d\n", errno); */
        raise_error(FILE_ERROR);
      } else {
        /* printf("cmdGetCTime: st_ctime=%ld\n", stat_buf.st_ctime); */
        timFromTimestamp(stat_buf.st_ctime,
            year, month, day, hour,
            min, sec, micro_sec, time_zone, is_dst);
      } /* if */
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("END cmdGetCTime(%04ld-%02ld-%02ld %02ld:%02ld:%02ld.%06ld %ld %d)\n",
        *year, *month, *day, *hour, *min, *sec,
        *micro_sec, *time_zone, *is_dst);
#endif
  } /* cmdGetCTime */



#ifdef ANSI_C

void cmdGetMTime (const const_stritype file_name,
    inttype *year, inttype *month, inttype *day, inttype *hour,
    inttype *min, inttype *sec, inttype *micro_sec, inttype *time_zone,
    booltype *is_dst)
#else

void cmdGetMTime (file_name,
    year, month, day, hour, min, sec, micro_sec, time_zone, is_dst)
stritype file_name;
inttype *year;
inttype *month;
inttype *day;
inttype *hour;
inttype *min;
inttype *sec;
inttype *micro_sec;
inttype *time_zone;
booltype *is_dst;
#endif

  {
    os_stritype os_path;
    os_stat_struct stat_buf;
    int stat_result;
    int path_info = PATH_IS_NORMAL;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdGetMTime */
#ifdef TRACE_CMD_RTL
    printf("BEGIN cmdGetMTime(");
    prot_stri(file_name);
    printf(")\n");
#endif
    os_path = cp_to_os_path(file_name, &path_info, &err_info);
    if (unlikely(err_info != OKAY_NO_ERROR)) {
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        timFromTimestamp(0,
            year, month, day, hour,
            min, sec, micro_sec, time_zone, is_dst);
      } else {
        raise_error(err_info);
      } /*if */
#else
      raise_error(err_info);
#endif
    } else {
      stat_result = os_stat(os_path, &stat_buf);
      os_stri_free(os_path);
      if (unlikely(stat_result != 0)) {
        /* printf("errno=%d\n", errno); */
        raise_error(FILE_ERROR);
      } else {
        /* printf("cmdGetMTime: st_mtime=%ld\n", stat_buf.st_mtime); */
        timFromTimestamp(stat_buf.st_mtime,
            year, month, day, hour,
            min, sec, micro_sec, time_zone, is_dst);
      } /* if */
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("END cmdGetMTime(%04ld-%02ld-%02ld %02ld:%02ld:%02ld.%06ld %ld %d)\n",
        *year, *month, *day, *hour, *min, *sec,
        *micro_sec, *time_zone, *is_dst);
#endif
  } /* cmdGetMTime */



#ifdef ANSI_C

rtlArraytype cmdGetSearchPath (void)
#else

rtlArraytype cmdGetSearchPath ()
#endif

  {
    errinfotype err_info = OKAY_NO_ERROR;
    rtlArraytype result;

  /* cmdGetSearchPath */
    result = getSearchPath(&err_info);
    if (unlikely(result == NULL)) {
      raise_error(err_info);
    } /* if */
    return result;
  } /* cmdGetSearchPath */



#ifdef ANSI_C

stritype cmdHomeDir (void)
#else

stritype cmdHomeDir ()
#endif

  {
    static const os_chartype home_dir_env_var[] = HOME_DIR_ENV_VAR;
    os_stritype os_home_dir;
#ifdef DEFAULT_HOME_DIR
    static const os_chartype default_home_dir[] = DEFAULT_HOME_DIR;
#endif
    errinfotype err_info = OKAY_NO_ERROR;
    stritype home_dir;

  /* cmdHomeDir */
    os_home_dir = os_getenv(home_dir_env_var);
    if (os_home_dir == NULL) {
#ifdef DEFAULT_HOME_DIR
      home_dir = cp_from_os_path(default_home_dir, &err_info);
#else
      raise_error(FILE_ERROR);
      home_dir = NULL;
#endif
    } else {
      home_dir = cp_from_os_path(os_home_dir, &err_info);
      os_getenv_string_free(os_home_dir);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
    return home_dir;
  } /* cmdHomeDir */



#ifdef ANSI_C

rtlArraytype cmdLs (const const_stritype dir_name)
#else

rtlArraytype cmdLs (dir_name)
stritype dir_name;
#endif

  {
    errinfotype err_info = OKAY_NO_ERROR;
    rtlArraytype result;

  /* cmdLs */
    result = read_dir(dir_name, &err_info);
    if (unlikely(result == NULL)) {
      raise_error(err_info);
    } else {
      qsort((void *) result->arr,
          (size_t) (result->max_position - result->min_position + 1),
          sizeof(rtlObjecttype), &cmp_mem);
    } /* if */
    return result;
  } /* cmdLs */



#ifdef ANSI_C

void cmdMkdir (const const_stritype dir_name)
#else

void cmdMkdir (dir_name)
stritype dir_name;
#endif

  {
    os_stritype os_path;
    int mkdir_result;
    int path_info;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdMkdir */
    os_path = cp_to_os_path(dir_name, &path_info, &err_info);
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } else {
      /* printf("mkdir(");
         prot_os_stri(os_path);
         printf(")\n"); */
      mkdir_result = os_mkdir(os_path, 0777);
      os_stri_free(os_path);
      if (unlikely(mkdir_result != 0)) {
        raise_error(FILE_ERROR);
      } /* if */
    } /* if */
  } /* cmdMkdir */



#ifdef ANSI_C

void cmdMove (const const_stritype source_name, const const_stritype dest_name)
#else

void cmdMove (source_name, dest_name)
stritype source_name;
stritype dest_name;
#endif

  {
    os_stritype os_source_name;
    os_stritype os_dest_name;
    int path_info;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdMove */
    os_source_name = cp_to_os_path(source_name, &path_info, &err_info);
    if (likely(err_info == OKAY_NO_ERROR)) {
      os_dest_name = cp_to_os_path(dest_name, &path_info, &err_info);
      if (likely(err_info == OKAY_NO_ERROR)) {
        move_any_file(os_source_name, os_dest_name, &err_info);
        os_stri_free(os_dest_name);
      } /* if */
      os_stri_free(os_source_name);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdMove */



#ifdef ANSI_C

stritype cmdReadlink (const const_stritype link_name)
#else

stritype cmdReadlink (link_name)
stritype link_name;
#endif

  {
#ifdef HAS_SYMLINKS
    os_stritype os_link_name;
    os_stat_struct link_stat;
    os_stritype link_destination;
    ssize_t readlink_result;
    int path_info;
#endif
    errinfotype err_info = OKAY_NO_ERROR;
    stritype result = NULL;

  /* cmdReadlink */
#ifdef HAS_SYMLINKS
    os_link_name = cp_to_os_path(link_name, &path_info, &err_info);
    if (likely(err_info == OKAY_NO_ERROR)) {
      if (os_lstat(os_link_name, &link_stat) != 0 || !S_ISLNK(link_stat.st_mode)) {
        err_info = FILE_ERROR;
      } else {
        /* printf("link size=%lu\n", link_stat.st_size); */
        if (link_stat.st_size > MAX_SYMLINK_PATH_LENGTH || link_stat.st_size < 0) {
          err_info = RANGE_ERROR;
        } else {
          if (!os_stri_alloc(link_destination, link_stat.st_size)) {
            err_info = MEMORY_ERROR;
          } else {
            readlink_result = readlink(os_link_name, link_destination,
                                       (size_t) link_stat.st_size);
            if (readlink_result != -1) {
              link_destination[readlink_result] = '\0';
              result = cp_from_os_path(link_destination, &err_info);
            } else {
              err_info = FILE_ERROR;
            } /* if */
            os_stri_free(link_destination);
          } /* if */
        } /* if */
      } /* if */
      os_stri_free(os_link_name);
    } /* if */
#else
    err_info = FILE_ERROR;
#endif
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
    return result;
  } /* cmdReadlink */



#ifdef ANSI_C

void cmdRemove (const const_stritype file_name)
#else

void cmdRemove (file_name)
stritype file_name;
#endif

  {
#ifdef REMOVE_FAILS_FOR_EMPTY_DIRS
    os_stat_struct file_stat;
#endif
    os_stritype os_file_name;
    int path_info;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdRemove */
#ifdef TRACE_CMD_RTL
    printf("BEGIN cmdRemove(");
    prot_stri(file_name);
    printf(")\n");
#endif
    os_file_name = cp_to_os_path(file_name, &path_info, &err_info);
    if (likely(err_info == OKAY_NO_ERROR)) {
#ifdef REMOVE_FAILS_FOR_EMPTY_DIRS
      if (os_lstat(os_file_name, &file_stat) != 0) {
        /* File does not exist */
        err_info = FILE_ERROR;
      } else {
        if (S_ISDIR(file_stat.st_mode)) {
          if (os_rmdir(os_file_name) != 0) {
            err_info = FILE_ERROR;
          } /* if */
        } else {
          if (os_remove(os_file_name) != 0) {
            /* printf("errno=%d\n", errno);
            printf("EACCES=%d  EBUSY=%d  EEXIST=%d  ENOTEMPTY=%d  ENOENT=%d  ENOTDIR=%d  EROFS=%d\n",
                EACCES, EBUSY, EEXIST, ENOTEMPTY, ENOENT, ENOTDIR, EROFS);
            printf("EFAULT=%d  EISDIR=%d  ENAMETOOLONG=%d  ENODEV=%d  EINVAL=%d\n",
                EFAULT, EISDIR, ENAMETOOLONG, ENODEV, EINVAL); */
            /* printf("file_name=%s\n", os_file_name); */
            err_info = FILE_ERROR;
          } /* if */
        } /* if */
      } /* if */
#else
      /* printf("os_remove(");
         prot_os_stri(os_file_name);
         printf(")\n"); */
      if (os_remove(os_file_name) != 0) {
        err_info = FILE_ERROR;
      } /* if */
#endif
      os_stri_free(os_file_name);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("END cmdRemove(");
    prot_stri(file_name);
    printf(")\n");
#endif
  } /* cmdRemove */



#ifdef ANSI_C

void cmdRemoveAnyFile (const const_stritype file_name)
#else

void cmdRemoveAnyFile (file_name)
stritype file_name;
#endif

  {
    os_stritype os_file_name;
    int path_info;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdRemoveAnyFile */
#ifdef TRACE_CMD_RTL
    printf("BEGIN cmdRemoveAnyFile\n");
#endif
    os_file_name = cp_to_os_path(file_name, &path_info, &err_info);
    if (likely(err_info == OKAY_NO_ERROR)) {
      remove_any_file(os_file_name, &err_info);
      os_stri_free(os_file_name);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("END cmdRemoveAnyFile\n");
#endif
  } /* cmdRemoveAnyFile */



#ifdef os_putenv
#ifdef ANSI_C

void cmdSetenv (const const_stritype name, const const_stritype value)
#else

void cmdSetenv (name, value)
stritype name;
stritype value;
#endif

  {
    memsizetype stri_size;
    stritype stri;
    os_stritype env_stri;
    int putenv_result;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdSetenv */
    if (unlikely(name->size > MAX_STRI_LEN - value->size - 1)) {
      /* number of bytes does not fit into memsizetype */
      raise_error(MEMORY_ERROR);
    } else {
      stri_size = name->size + value->size + 1;
      if (unlikely(!ALLOC_STRI_SIZE_OK(stri, stri_size))) {
        raise_error(MEMORY_ERROR);
      } else {
        stri->size = stri_size;
        memcpy(stri->mem, name->mem,
            name->size * sizeof(strelemtype));
        stri->mem[name->size] = (strelemtype) '=';
        memcpy(&stri->mem[name->size + 1], value->mem,
            value->size * sizeof(strelemtype));
        env_stri = stri_to_os_stri(stri, &err_info);
        FREE_STRI(stri, stri->size);;
        if (unlikely(env_stri == NULL)) {
          raise_error(MEMORY_ERROR);
        } else {
          putenv_result = os_putenv(env_stri);
          os_stri_free(env_stri);
          if (unlikely(putenv_result != 0)) {
            /* printf("errno=%d\n", errno); */
            raise_error(RANGE_ERROR);
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* cmdSetenv */

#else



#ifdef ANSI_C

void cmdSetenv (const const_stritype name, const const_stritype value)
#else

void cmdSetenv (name, value)
stritype name;
stritype value;
#endif

  {
    os_stritype env_name;
    os_stritype env_value;
    int setenv_result;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdSetenv */
    env_name = stri_to_os_stri(name, &err_info);
    if (unlikely(env_name == NULL)) {
      raise_error(err_info);
    } else {
      env_value = stri_to_os_stri(value, &err_info);
      if (unlikely(env_value == NULL)) {
        os_stri_free(env_name);
        raise_error(err_info);
      } else {
        setenv_result = os_setenv(env_name, env_value, 1);
        os_stri_free(env_name);
        os_stri_free(env_value);
        if (unlikely(setenv_result != 0)) {
          /* printf("errno=%d\n", errno); */
          if (errno == ENOMEM) {
            raise_error(MEMORY_ERROR);
          } else {
            raise_error(RANGE_ERROR);
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* cmdSetenv */

#endif



#ifdef ANSI_C

void cmdSetATime (const const_stritype file_name,
    inttype year, inttype month, inttype day, inttype hour,
    inttype min, inttype sec, inttype micro_sec, inttype time_zone)
#else

void cmdSetATime (file_name,
    year, month, day, hour, min, sec, micro_sec, time_zone)
 stritype file_name;
inttype year;
inttype month;
inttype day;
inttype hour;
inttype min;
inttype sec;
inttype micro_sec;
inttype time_zone;
#endif

  {
    os_stritype os_path;
    os_stat_struct stat_buf;
    os_utimbuf_struct utime_buf;
    int path_info;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdSetATime */
    os_path = cp_to_os_path(file_name, &path_info, &err_info);
    if (likely(err_info == OKAY_NO_ERROR)) {
      if (os_stat(os_path, &stat_buf) == 0) {
        utime_buf.actime = timToTimestamp(year, month, day, hour,
            min, sec, micro_sec, time_zone);
        /* printf("cmdSetATime: actime=%ld\n", utime_buf.actime); */
        utime_buf.modtime = stat_buf.st_mtime;
        if (utime_buf.actime == (time_t) -1) {
          err_info = RANGE_ERROR;
        } else if (os_utime(os_path, &utime_buf) != 0) {
          /* printf("errno=%d\n", errno);
          printf("EPERM=%d, EACCES=%d  EBUSY=%d  EEXIST=%d  ENOTEMPTY=%d  ENOENT=%d  ENOTDIR=%d  EROFS=%d\n",
              EPERM, EACCES, EBUSY, EEXIST, ENOTEMPTY, ENOENT, ENOTDIR, EROFS); */
          err_info = FILE_ERROR;
        } /* if */
      } else {
        err_info = FILE_ERROR;
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdSetATime */



#ifdef ANSI_C

void cmdSetFileMode (const const_stritype file_name, const const_settype mode)
#else

void cmdSetFileMode (file_name, mode)
stritype file_name;
settype mode;
#endif

  {
    os_stritype os_path;
    int int_mode;
    int chmod_result;
    int path_info;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdSetFileMode */
#ifdef TRACE_CMD_RTL
    printf("BEGIN cmdSetFileMode(");
    prot_stri(file_name);
    printf(")\n");
#endif
    os_path = cp_to_os_path(file_name, &path_info, &err_info);
    if (likely(err_info == OKAY_NO_ERROR)) {
      int_mode = setSConv(mode);
      /* printf("cmdSetFileMode: mode=0%o\n", int_mode); */
#if MODE_BITS_NORMAL
      chmod_result = os_chmod(os_path, int_mode);
#else
      /* Force the bits to the standard sequence */
      chmod_result = os_chmod(os_path,
          (int_mode & 0400 ? S_IRUSR : 0) |
          (int_mode & 0200 ? S_IWUSR : 0) |
          (int_mode & 0100 ? S_IXUSR : 0) |
          (int_mode & 0040 ? S_IRGRP : 0) |
          (int_mode & 0020 ? S_IWGRP : 0) |
          (int_mode & 0010 ? S_IXGRP : 0) |
          (int_mode & 0004 ? S_IROTH : 0) |
          (int_mode & 0002 ? S_IWOTH : 0) |
          (int_mode & 0001 ? S_IXOTH : 0));
#endif
      if (chmod_result != 0) {
        /* printf("\nerrno=%d %s\n", errno, strerror(errno)); */
        err_info = FILE_ERROR;
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdSetFileMode */



#ifdef ANSI_C

void cmdSetMTime (const const_stritype file_name,
    inttype year, inttype month, inttype day, inttype hour,
    inttype min, inttype sec, inttype micro_sec, inttype time_zone)
#else

void cmdSetMTime (file_name,
    year, month, day, hour, min, sec, micro_sec, time_zone)
stritype file_name;
inttype year;
inttype month;
inttype day;
inttype hour;
inttype min;
inttype sec;
inttype micro_sec;
inttype time_zone;
#endif

  {
    os_stritype os_path;
    os_stat_struct stat_buf;
    os_utimbuf_struct utime_buf;
    int path_info;
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdSetMTime */
    os_path = cp_to_os_path(file_name, &path_info, &err_info);
    if (likely(err_info == OKAY_NO_ERROR)) {
      if (os_stat(os_path, &stat_buf) == 0) {
        utime_buf.actime = stat_buf.st_atime;
        utime_buf.modtime = timToTimestamp(year, month, day, hour,
            min, sec, micro_sec, time_zone);
        /* printf("cmdSetMTime: modtime=%ld\n", utime_buf.modtime); */
        if (utime_buf.actime == (time_t) -1) {
          err_info = RANGE_ERROR;
        } else if (os_utime(os_path, &utime_buf) != 0) {
          /* printf("errno=%d\n", errno);
          printf("EPERM=%d, EACCES=%d  EBUSY=%d  EEXIST=%d  ENOTEMPTY=%d  ENOENT=%d  ENOTDIR=%d  EROFS=%d\n",
              EPERM, EACCES, EBUSY, EEXIST, ENOTEMPTY, ENOENT, ENOTDIR, EROFS); */
          err_info = FILE_ERROR;
        } /* if */
      } else {
        err_info = FILE_ERROR;
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdSetMTime */



#ifdef ANSI_C

inttype cmdShell (const const_stritype command, const const_stritype parameters)
#else

inttype cmdShell (command, parameters)
stritype command;
stritype parameters;
#endif

  {
    os_stritype os_command;
    errinfotype err_info = OKAY_NO_ERROR;
    inttype result;

  /* cmdShell */
    os_command = cp_to_command(command, parameters, &err_info);
    if (unlikely(os_command == NULL)) {
      raise_error(err_info);
      result = 0;
    } else {
      /* printf("os_command: \"%s\"\n", os_command); */
      result = (inttype) os_system(os_command);
      /* if (result != 0) {
        printf("errno=%d\n", errno);
        printf("E2BIG=%d  ENOENT=%d  ENOEXEC=%d  ENOMEM=%d\n",
            E2BIG, ENOENT, ENOEXEC, ENOMEM);
        printf("result=%d\n", result);
      } */
      os_stri_free(os_command);
    } /* if */
    return result;
  } /* cmdShell */



#ifdef ANSI_C

stritype cmdShellEscape (const const_stritype stri)
#else

stritype cmdShellEscape (stri)
stritype stri;
#endif

  {
    memsizetype inPos;
    memsizetype outPos;
    booltype quote_path;
#ifndef ESCAPE_SHELL_COMMANDS
    memsizetype countBackslash;
#endif
    errinfotype err_info = OKAY_NO_ERROR;
    stritype resized_result;
    stritype result;

  /* cmdShellEscape */
    if (unlikely(stri->size > (MAX_STRI_LEN - 2) / 3 ||
        !ALLOC_STRI_SIZE_OK(result, 3 * stri->size + 2))) {
      raise_error(MEMORY_ERROR);
      result = NULL;
    } else {
      quote_path = FALSE;
      for (inPos = 0, outPos = 0; inPos < stri->size; inPos++, outPos++) {
        switch (stri->mem[inPos]) {
#ifdef ESCAPE_SHELL_COMMANDS
          case '\t': case ' ':  case '!':  case '\"': case '$':
          case '&':  case '\'': case '(':  case ')':  case '*':
          case ',':  case ':':  case ';':  case '<':  case '=':
          case '>':  case '?':  case '[':  case '\\': case ']':
          case '^':  case '`':  case '{':  case '|':  case '}':
          case '~':
            result->mem[outPos] = '\\';
            outPos++;
            result->mem[outPos] = stri->mem[inPos];
            break;
          case '\0': case '\n':
            err_info = RANGE_ERROR;
            break;
#else
          case ' ':  case ',':  case ';':  case '=':  case '~':
          case 160:
            quote_path = TRUE;
            result->mem[outPos] = stri->mem[inPos];
            break;
          case '&':  case '<':  case '>':  case '^':  case '|':
            quote_path = TRUE;
            result->mem[outPos] = '^';
            outPos++;
            result->mem[outPos] = stri->mem[inPos];
            break;
          case '%':
            quote_path = TRUE;
            result->mem[outPos] = '\"';
            outPos++;
            result->mem[outPos] = stri->mem[inPos];
            outPos++;
            result->mem[outPos] = '\"';
            break;
          case '\"':
            quote_path = TRUE;
            result->mem[outPos] = '\\';
            outPos++;
            result->mem[outPos] = stri->mem[inPos];
            break;
          case '\\':
            quote_path = TRUE;
            inPos++;
            countBackslash = 1;
            while (stri->mem[inPos] == '\\') {
              inPos++;
              countBackslash++;
            } /* while */
            if (inPos == stri->size || stri->mem[inPos] == '"') {
              countBackslash *= 2;
            } /* if */
            inPos--;
            do {
              result->mem[outPos] = '\\';
              outPos++;
              countBackslash--;
            } while (countBackslash != 0);
            outPos--;
            break;
          case '\0': case '\n': case '\r':
            err_info = RANGE_ERROR;
            break;
#endif
          default:
            result->mem[outPos] = stri->mem[inPos];
            break;
        } /* switch */
      } /* for */
      if (unlikely(err_info != OKAY_NO_ERROR)) {
        FREE_STRI(result, 3 * stri->size + 2);
        raise_error(err_info);
        result = NULL;
      } else {
        if (quote_path) {
          memmove(&result->mem[1], result->mem, sizeof(strelemtype) * outPos);
          result->mem[0] = '\"';
          result->mem[outPos + 1] = '\"';
          outPos += 2;
        } /* if */
        REALLOC_STRI_SIZE_OK(resized_result, result, 3 * stri->size + 2, outPos);
        if (resized_result == NULL) {
          FREE_STRI(result, 3 * stri->size + 2);
          raise_error(MEMORY_ERROR);
          result = NULL;
        } else {
          result = resized_result;
          COUNT3_STRI(3 * stri->size + 2, outPos);
          result->size = outPos;
        } /* if */
      } /* if */
    } /* if */
    return result;
  } /* cmdShellEscape */



#ifdef ANSI_C

void cmdSymlink (const const_stritype source_name, const const_stritype dest_name)
#else

void cmdSymlink (source_name, dest_name)
stritype source_name;
stritype dest_name;
#endif

  {
#ifdef HAS_SYMLINKS
    os_stritype os_source_name;
    os_stritype os_dest_name;
    int path_info;
#endif
    errinfotype err_info = OKAY_NO_ERROR;

  /* cmdSymlink */
#ifdef HAS_SYMLINKS
    os_source_name = cp_to_os_path(source_name, &path_info, &err_info);
    if (likely(err_info == OKAY_NO_ERROR)) {
      os_dest_name = cp_to_os_path(dest_name, &path_info, &err_info);
      if (likely(err_info == OKAY_NO_ERROR)) {
        if (symlink(os_source_name, os_dest_name) != 0) {
          err_info = FILE_ERROR;
        } /* if */
        os_stri_free(os_dest_name);
      } /* if */
      os_stri_free(os_source_name);
    } /* if */
#else
    err_info = FILE_ERROR;
#endif
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdSymlink */



#ifdef ANSI_C

stritype cmdToOsPath (const const_stritype standardPath)
#else

stritype cmdToOsPath (standardPath)
stritype stri;
#endif

  {
    errinfotype err_info = OKAY_NO_ERROR;
    stritype result;

  /* cmdToOsPath */
#ifdef TRACE_CMD_RTL
    printf("cmdToOsPath(");
    prot_stri(standardPath);
    printf(")\n");
#endif
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
#ifdef FORBID_DRIVE_LETTERS
    if (unlikely(standardPath->size >= 2 && (standardPath->mem[standardPath->size - 1] == '/' ||
                 (standardPath->mem[1] == ':' &&
                 ((standardPath->mem[0] >= 'a' && standardPath->mem[0] <= 'z') ||
                  (standardPath->mem[0] >= 'A' && standardPath->mem[0] <= 'Z')))))) {
#else
    if (unlikely(standardPath->size >= 2 && standardPath->mem[standardPath->size - 1] == '/')) {
#endif
#else
    if (unlikely(standardPath->size >= 2 && standardPath->mem[standardPath->size - 1] == '/' &&
                 (standardPath->size != 3 || standardPath->mem[1] != ':' ||
                 ((standardPath->mem[0] < 'a' || standardPath->mem[0] > 'z') &&
                  (standardPath->mem[0] < 'A' || standardPath->mem[0] > 'Z'))))) {
#endif
      err_info = RANGE_ERROR;
    } else {
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (standardPath->size >= 1 && standardPath->mem[0] == '/') {
        /* Absolute path: Try to map the path to a drive letter */
        if (unlikely(standardPath->size == 1)) {
          /* "/"    cannot be mapped to a drive letter */
          err_info = RANGE_ERROR;
        } else if (standardPath->mem[1] >= 'a' && standardPath->mem[1] <= 'z') {
          if (standardPath->size == 2) {
            /* "/c"   is mapped to "c:/"  */
            if (unlikely(!ALLOC_STRI_SIZE_OK(result, 3))) {
              err_info = MEMORY_ERROR;
            } else {
              result->size = 3;
              result->mem[0] = standardPath->mem[1];
              result->mem[1] = ':';
              result->mem[2] = '/';
            } /* if */
          } else if (unlikely(standardPath->mem[2] != '/')) {
            /* "/cd"  cannot be mapped to a drive letter */
            err_info = RANGE_ERROR;
          } else {
            /* "/c/d" is mapped to "c:/d" */
            if (unlikely(!ALLOC_STRI_SIZE_OK(result, standardPath->size))) {
              err_info = MEMORY_ERROR;
            } else {
              result->size = standardPath->size;
              result->mem[0] = standardPath->mem[1];
              result->mem[1] = ':';
              result->mem[2] = '/';
              memcpy(&result->mem[3], &standardPath->mem[3], (standardPath->size - 3) * sizeof(strelemtype));
            } /* if */
          } /* if */
        } else {
          /* "/C"  cannot be mapped to a drive letter */
          err_info = RANGE_ERROR;
        } /* if */
      } else {
#endif
        if (unlikely(!ALLOC_STRI_SIZE_OK(result, standardPath->size))) {
          err_info = MEMORY_ERROR;
        } else {
          result->size = standardPath->size;
          memcpy(result->mem, standardPath->mem, standardPath->size * sizeof(strelemtype));
        } /* if */
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      } /* if */
#endif
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
      result = NULL;
    } else {
#if PATH_DELIMITER != '/'
      memsizetype position;

      for (position = 0; position < result->size; position++) {
        if (result->mem[position] == '/') {
          result->mem[position] = PATH_DELIMITER;
        } /* if */
      } /* for */
#endif
    } /* if */
#ifdef TRACE_CMD_RTL
    printf("cmdToOsPath(");
    prot_stri(standardPath);
    printf(") --> ");
    prot_stri(result);
    printf("\n");
#endif
    return result;
  } /* cmdToOsPath */
