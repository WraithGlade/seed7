/********************************************************************/
/*                                                                  */
/*  sql_ite.c     Database access functions for SQLite.             */
/*  Copyright (C) 1989 - 2014  Thomas Mertes                        */
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
/*  File: seed7/src/sql_ite.c                                       */
/*  Changes: 2013, 2014  Thomas Mertes                              */
/*  Content: Database access functions for SQLite.                  */
/*                                                                  */
/********************************************************************/

#include "version.h"

#ifdef SQLITE_INCLUDE
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "limits.h"
#include "float.h"
#include SQLITE_INCLUDE

#include "common.h"
#include "data_rtl.h"
#include "striutl.h"
#include "heaputl.h"
#include "str_rtl.h"
#include "tim_rtl.h"
#include "cmd_rtl.h"
#include "sql_util.h"
#include "big_drv.h"
#include "rtl_err.h"
#include "dll_drv.h"
#include "sql_drv.h"

#undef VERBOSE_EXCEPTIONS


typedef struct {
    uintType     usage_count;
    sqlFuncType  sqlFunc;
    sqlite3     *connection;
  } dbRecord, *dbType;

typedef struct {
    uintType      usage_count;
    sqlFuncType   sqlFunc;
    sqlite3_stmt *ppStmt;
    int           result_column_count;
    boolType      executeSuccessful;
    boolType      useStoredFetchResult;
    boolType      storedFetchResult;
    boolType      fetchOkay;
    boolType      fetchFinished;
  } preparedStmtRecord, *preparedStmtType;

static sqlFuncType sqlFunc = NULL;

#ifdef VERBOSE_EXCEPTIONS
#define logError(logStatements) printf(" *** "); logStatements
#else
#define logError(logStatements)
#endif


#ifdef SQLITE_DLL
int (*ptr_sqlite3_bind_blob) (sqlite3_stmt *pStmt,
                              int index,
                              const void *value,
                              int n,
                              void (*destruct) (void*));
int (*ptr_sqlite3_bind_double) (sqlite3_stmt *pStmt, int index, double value);
int (*ptr_sqlite3_bind_int) (sqlite3_stmt *pStmt, int index, int value);
int (*ptr_sqlite3_bind_int64) (sqlite3_stmt *pStmt, int index, sqlite3_int64 value);
int (*ptr_sqlite3_bind_null) (sqlite3_stmt *pStmt, int index);
int (*ptr_sqlite3_bind_text) (sqlite3_stmt *pStmt,
                              int index,
                              const char *value,
                              int n,
                              void (*destruct) (void*));
int (*ptr_sqlite3_close) (sqlite3 *db);
const void *(*ptr_sqlite3_column_blob) (sqlite3_stmt *pStmt, int iCol);
int (*ptr_sqlite3_column_bytes) (sqlite3_stmt *pStmt, int iCol);
int (*ptr_sqlite3_column_count) (sqlite3_stmt *pStmt);
double (*ptr_sqlite3_column_double) (sqlite3_stmt *pStmt, int iCol);
int (*ptr_sqlite3_column_int) (sqlite3_stmt *pStmt, int iCol);
sqlite3_int64 (*ptr_sqlite3_column_int64) (sqlite3_stmt *pStmt, int iCol);
const char *(*ptr_sqlite3_column_name) (sqlite3_stmt *pStmt, int N);
const unsigned char *(*ptr_sqlite3_column_text) (sqlite3_stmt *pStmt, int iCol);
int (*ptr_sqlite3_column_type) (sqlite3_stmt *pStmt, int iCol);
int (*ptr_sqlite3_finalize) (sqlite3_stmt *pStmt);
int (*ptr_sqlite3_open) (const char *filename, sqlite3 **ppDb);
int (*ptr_sqlite3_prepare) (sqlite3 *db,
                            const char *zSql,
                            int nByte,
                            sqlite3_stmt **ppStmt,
                            const char **pzTail);
int (*ptr_sqlite3_reset) (sqlite3_stmt *pStmt);
int (*ptr_sqlite3_step) (sqlite3_stmt *pStmt);

#define sqlite3_bind_blob     ptr_sqlite3_bind_blob
#define sqlite3_bind_double   ptr_sqlite3_bind_double
#define sqlite3_bind_int      ptr_sqlite3_bind_int
#define sqlite3_bind_int64    ptr_sqlite3_bind_int64
#define sqlite3_bind_null     ptr_sqlite3_bind_null
#define sqlite3_bind_text     ptr_sqlite3_bind_text
#define sqlite3_close         ptr_sqlite3_close
#define sqlite3_column_blob   ptr_sqlite3_column_blob
#define sqlite3_column_bytes  ptr_sqlite3_column_bytes
#define sqlite3_column_count  ptr_sqlite3_column_count
#define sqlite3_column_double ptr_sqlite3_column_double
#define sqlite3_column_int    ptr_sqlite3_column_int
#define sqlite3_column_int64  ptr_sqlite3_column_int64
#define sqlite3_column_name   ptr_sqlite3_column_name
#define sqlite3_column_text   ptr_sqlite3_column_text
#define sqlite3_column_type   ptr_sqlite3_column_type
#define sqlite3_finalize      ptr_sqlite3_finalize
#define sqlite3_open          ptr_sqlite3_open
#define sqlite3_prepare       ptr_sqlite3_prepare
#define sqlite3_reset         ptr_sqlite3_reset
#define sqlite3_step          ptr_sqlite3_step



static boolType setupDll (const char *dllName)

  {
    static void *dbDll = NULL;

  /* setupDll */
    /* printf("setupDll(\"%s\")\n", dllName); */
    if (dbDll == NULL) {
      dbDll = dllOpen(dllName);
      if (dbDll != NULL) {
        if ((ptr_sqlite3_bind_blob     = dllSym(dbDll, "sqlite3_bind_blob"))     == NULL ||
            (ptr_sqlite3_bind_double   = dllSym(dbDll, "sqlite3_bind_double"))   == NULL ||
            (ptr_sqlite3_bind_int      = dllSym(dbDll, "sqlite3_bind_int"))      == NULL ||
            (ptr_sqlite3_bind_int64    = dllSym(dbDll, "sqlite3_bind_int64"))    == NULL ||
            (ptr_sqlite3_bind_null     = dllSym(dbDll, "sqlite3_bind_null"))     == NULL ||
            (ptr_sqlite3_bind_text     = dllSym(dbDll, "sqlite3_bind_text"))     == NULL ||
            (ptr_sqlite3_close         = dllSym(dbDll, "sqlite3_close"))         == NULL ||
            (ptr_sqlite3_column_blob   = dllSym(dbDll, "sqlite3_column_blob"))   == NULL ||
            (ptr_sqlite3_column_bytes  = dllSym(dbDll, "sqlite3_column_bytes"))  == NULL ||
            (ptr_sqlite3_column_count  = dllSym(dbDll, "sqlite3_column_count"))  == NULL ||
            (ptr_sqlite3_column_double = dllSym(dbDll, "sqlite3_column_double")) == NULL ||
            (ptr_sqlite3_column_int    = dllSym(dbDll, "sqlite3_column_int"))    == NULL ||
            (ptr_sqlite3_column_int64  = dllSym(dbDll, "sqlite3_column_int64"))  == NULL ||
            (ptr_sqlite3_column_name   = dllSym(dbDll, "sqlite3_column_name"))   == NULL ||
            (ptr_sqlite3_column_text   = dllSym(dbDll, "sqlite3_column_text"))   == NULL ||
            (ptr_sqlite3_column_type   = dllSym(dbDll, "sqlite3_column_type"))   == NULL ||
            (ptr_sqlite3_finalize      = dllSym(dbDll, "sqlite3_finalize"))      == NULL ||
            (ptr_sqlite3_open          = dllSym(dbDll, "sqlite3_open"))          == NULL ||
            (ptr_sqlite3_prepare       = dllSym(dbDll, "sqlite3_prepare"))       == NULL ||
            (ptr_sqlite3_reset         = dllSym(dbDll, "sqlite3_reset"))         == NULL ||
            (ptr_sqlite3_step          = dllSym(dbDll, "sqlite3_step"))          == NULL) {
          dbDll = NULL;
        } /* if */
      } /* if */
    } /* if */
    /* printf("setupDll --> %d\n", dbDll != NULL); */
    return dbDll != NULL;
  } /* setupDll */

#else

#define setupDll(dllName) TRUE
#define SQLITE_DLL ""

#endif

#ifndef SQLITE_DLL_PATH
#define SQLITE_DLL_PATH ""
#endif



static void sqlClose (databaseType database);



static void freeDatabase (databaseType database)

  {
    dbType db;

  /* freeDatabase */
    sqlClose(database);
    db = (dbType) database;
    FREE_RECORD(db, dbRecord, count.database);
  } /* freeDatabase */



static void freePreparedStmt (sqlStmtType sqlStatement)

  {
    preparedStmtType preparedStmt;

  /* freePreparedStmt */
    /* printf("begin freePreparedStmt(%lx)\n", (unsigned long) sqlStatement); */
    preparedStmt = (preparedStmtType) sqlStatement;
    sqlite3_finalize(preparedStmt->ppStmt);
    FREE_RECORD(preparedStmt, preparedStmtRecord, count.prepared_stmt);
    /* printf("end freePreparedStmt\n"); */
  } /* freePreparedStmt */



static const char *nameOfBufferType (int buffer_type)

  {
    static char buffer[50];
    const char *typeName;

  /* nameOfBufferType */
    /* printf("nameOfBuffer(%d)\n", buffer_type); */
    switch (buffer_type) {
      case SQLITE_INTEGER: typeName = "SQLITE_INTEGER"; break;
      case SQLITE_FLOAT:   typeName = "SQLITE_FLOAT"; break;
      case SQLITE_TEXT:    typeName = "SQLITE_TEXT"; break;
      case SQLITE_BLOB:    typeName = "SQLITE_BLOB"; break;
      case SQLITE_NULL:    typeName = "SQLITE_NULL"; break;
      default:
        sprintf(buffer, "%d", buffer_type);
        typeName = buffer;
        break;
    } /* switch */
    /* printf("nameOfBufferType --> %s\n", typeName); */
    return typeName;
  } /* nameOfBufferType */



static void freeText (void *text)

  { /* freeText */
    free(text);
  } /* freeText */



static memSizeType decimalPrecision (const const_ustriType decimal,
    const memSizeType length)

  {
    memSizeType digitPos;
    memSizeType precision;

  /* decimalPrecision */
    precision = length;
    while (decimal[precision - 1] == '0') {
      precision--; /* Subtract trailing zero characters. */
    } /* while */
    precision--; /* Subtract decimal point character. */
    if (decimal[precision] == '.') {
      /* Trailing zero characters left from the decimal point do */
      /* not reduce the precision. This works convenient for the */
      /* function roundDoubleToBigRat, that does the conversion  */
      /* back to a rational. RoundDoubleToBigRat does not round  */
      /* digits left of the decimal point. Integer numbers with  */
      /* more than DBL_DIG digits cannot be guaranteed to be     */
      /* without change when they are converted to a double and  */
      /* back. In this case trailing zero digits are not kept.   */
      /* Therefore such numbers must be stored as blob.          */
    } else {
      digitPos = decimal[0] == '-';
      if (decimal[digitPos] == '0') {
        precision--; /* Subtract leading zero character. */
        digitPos++;
        if (decimal[digitPos] == '.') {
          /* The decimal point character has already been subtracted. */
          digitPos++;
          while (decimal[digitPos] == '0') {
            precision--; /* Subtract leading zero characters. */
            digitPos++;
          } /* while */
        } /* if */
      } /* if */
    } /* if */
    if (decimal[0] == '-') {
      precision--; /* Subtract sign character. */
    } /* if */
    return precision;
  } /* decimalPrecision */



static void sqlBindBigInt (sqlStmtType sqlStatement, intType pos,
    const const_bigIntType value)

  {
    preparedStmtType preparedStmt;
    ustriType decimal;
    memSizeType length;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindBigInt */
    /* printf("sqlBindBigInt(%lx, " FMT_D ", %s)\n",
       (unsigned long) sqlStatement, pos, bigHexCStri(value)); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || pos > INT_MAX)) {
      logError(printf("sqlBindBigInt: pos: " FMT_D ", max pos: %u.\n",
                      pos, INT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      if (preparedStmt->executeSuccessful) {
        if (sqlite3_reset(preparedStmt->ppStmt) != SQLITE_OK) {
          logError(printf("sqlBindBigInt: sqlite3_reset error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->executeSuccessful = FALSE;
        } /* if */
      } /* if */
      if (bigBitLength(value) >= 64) {
        decimal = bigIntToDecimal(value, &length, &err_info);
        if (unlikely(err_info != OKAY_NO_ERROR)) {
          raise_error(err_info);
        } else if (length > INT_MAX) {
          free(decimal);
          raise_error(MEMORY_ERROR);
        } else {
          if (sqlite3_bind_blob(preparedStmt->ppStmt,
                                (int) pos,
                                (const void *) decimal,
                                (int) length,
                                &freeText) != SQLITE_OK) {
            logError(printf("sqlBindBigInt: sqlite3_bind_blob error: %s\n",
                            sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->fetchOkay = FALSE;
          } /* if */
        } /* if */
      } else {
        if (sqlite3_bind_int64(preparedStmt->ppStmt,
                               (int) pos,
                               bigToInt64(value)) != SQLITE_OK) {
          logError(printf("sqlBindBigInt: sqlite3_bind_int64 error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->fetchOkay = FALSE;
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindBigInt */



static void sqlBindBigRat (sqlStmtType sqlStatement, intType pos,
    const const_bigIntType numerator, const const_bigIntType denominator)

  {
    preparedStmtType preparedStmt;
    ustriType decimal;
    memSizeType length;
    memSizeType precision;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindBigRat */
    /* printf("sqlBindBigRat(%lx, " FMT_D ", %s/%s)\n",
       (unsigned long) sqlStatement, pos,
       bigHexCStri(numerator), bigHexCStri(denominator)); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || pos > INT_MAX)) {
      logError(printf("sqlBindBigRat: pos: " FMT_D ", max pos: %u.\n",
                      pos, INT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      if (preparedStmt->executeSuccessful) {
        if (sqlite3_reset(preparedStmt->ppStmt) != SQLITE_OK) {
          logError(printf("sqlBindBigRat: sqlite3_reset error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->executeSuccessful = FALSE;
        } /* if */
      } /* if */
      decimal = bigRatToDecimal(numerator, denominator, 1000,
                                &length, &err_info);
      if (unlikely(err_info != OKAY_NO_ERROR)) {
        raise_error(err_info);
      } else {
        /* printf("decimal: %s\n", decimal); */
        precision = decimalPrecision(decimal, length);
        /* printf("precision: " FMT_U_MEM "\n", precision); */
        if (precision > DBL_DIG) {
          if (length > INT_MAX) {
            free(decimal);
            raise_error(MEMORY_ERROR);
          } else {
            if (sqlite3_bind_blob(preparedStmt->ppStmt,
                                  (int) pos,
                                  (const void *) decimal,
                                  (int) length,
                                  &freeText) != SQLITE_OK) {
              logError(printf("sqlBindBigRat: sqlite3_bind_blob error: %s\n",
                              sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
              raise_error(FILE_ERROR);
            } else {
              preparedStmt->fetchOkay = FALSE;
            } /* if */
          } /* if */
        } else {
          if (sqlite3_bind_double(preparedStmt->ppStmt,
                                  (int) pos,
                                  bigRatToDouble(numerator, denominator)) != SQLITE_OK) {
            logError(printf("sqlBindBigRat: sqlite3_bind_double error: %s\n",
                            sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->fetchOkay = FALSE;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindBigRat */



static void sqlBindBool (sqlStmtType sqlStatement, intType pos, boolType value)

  {
    preparedStmtType preparedStmt;

  /* sqlBindBool */
    /* printf("sqlBindBool(%lx, " FMT_D ", %ld)\n",
       (unsigned long) sqlStatement, pos, value); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || pos > INT_MAX)) {
      logError(printf("sqlBindBool: pos: " FMT_D ", max pos: %u.\n",
                      pos, INT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      if (preparedStmt->executeSuccessful) {
        if (sqlite3_reset(preparedStmt->ppStmt) != SQLITE_OK) {
          logError(printf("sqlBindBool: sqlite3_reset error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->executeSuccessful = FALSE;
        } /* if */
      } /* if */
      if (sqlite3_bind_int(preparedStmt->ppStmt,
                           (int) pos,
                           (int) value) != SQLITE_OK) {
        logError(printf("sqlBindBool: sqlite3_bind_int error: %s\n",
                        sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
        raise_error(FILE_ERROR);
      } else {
        preparedStmt->fetchOkay = FALSE;
      } /* if */
    } /* if */
  } /* sqlBindBool */



static void sqlBindBStri (sqlStmtType sqlStatement, intType pos, bstriType bstri)

  {
    preparedStmtType preparedStmt;
    cstriType blob;

  /* sqlBindBStri */
    /* printf("sqlBindBStri(%lx, " FMT_D ", ",
       (unsigned long) sqlStatement, pos);
       prot_bstri(bstri);
       printf(")\n"); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || pos > INT_MAX)) {
      logError(printf("sqlBindBStri: pos: " FMT_D ", max pos: %u.\n",
                      pos, INT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      if (preparedStmt->executeSuccessful) {
        if (sqlite3_reset(preparedStmt->ppStmt) != SQLITE_OK) {
          logError(printf("sqlBindBStri: sqlite3_reset error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->executeSuccessful = FALSE;
        } /* if */
      } /* if */
      if (unlikely(bstri->size > INT_MAX ||
                   !ALLOC_CSTRI(blob, bstri->size))) {
        raise_error(MEMORY_ERROR);
      } else {
        memcpy(blob, bstri->mem, bstri->size);
        if (sqlite3_bind_blob(preparedStmt->ppStmt,
                              (int) pos,
                              (const void *) blob,
                              (int) bstri->size,
                              &freeText) != SQLITE_OK) {
          logError(printf("sqlBindBStri: sqlite3_bind_blob error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->fetchOkay = FALSE;
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindBStri */



static void sqlBindDuration (sqlStmtType sqlStatement, intType pos,
    intType year, intType month, intType day, intType hour,
    intType minute, intType second, intType micro_second)

  {
    preparedStmtType preparedStmt;
    cstriType isoDate;
    int length;

  /* sqlBindDuration */
    /* printf("sqlBindDuration(%lx, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld)\n",
       sqlStatement, pos, year, month, day, hour, minute, second, micro_second); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || pos > INT_MAX)) {
      logError(printf("sqlBindDuration: pos: " FMT_D ", max pos: %u.\n",
                      pos, INT_MAX););
      raise_error(RANGE_ERROR);
    } else if (year < -INT_MAX || year > INT_MAX || month < -12 || month > 12 ||
               day < -31 || day > 31 || hour < -24 || hour > 24 || minute < -60 || minute > 60 ||
               second < -60 || second > 60 || micro_second < -1000000 || micro_second > 1000000) {
      logError(printf("sqlBindDuration: Duration not in allowed range.\n"););
      raise_error(RANGE_ERROR);
    } else if (!((year >= 0 && month >= 0 && day >= 0 && hour >= 0 &&
                  minute >= 0 && second >= 0 && micro_second >= 0) ||
                 (year <= 0 && month <= 0 && day <= 0 && hour <= 0 &&
                  minute <= 0 && second <= 0 && micro_second <= 0))) {
      logError(printf("sqlBindDuration: Duration neither clearly positive nor negative.\n"););
      raise_error(RANGE_ERROR);
    } else if (unlikely(!ALLOC_CSTRI(isoDate, 23))) {
      raise_error(MEMORY_ERROR);
    } else {
      if (hour == 0 && minute == 0 && second == 0 && micro_second == 0) {
        sprintf(isoDate, "%04d-%02u-%02u",
                (int) year, (unsigned int) month, (unsigned int) day);
        length = 10;
      } else if (year == 0 && month == 1 && day == 1 && micro_second == 0) {
        sprintf(isoDate, "%02u:%02u:%02u",
                (unsigned int) hour, (unsigned int) minute,
                (unsigned int) second);
        length = 8;
      } else if (micro_second == 0) {
        sprintf(isoDate, "%04d-%02u-%02u %02u:%02u:%02u",
                (int) year, (unsigned int) month, (unsigned int) day,
                (unsigned int) hour, (unsigned int) minute,
                (unsigned int) second);
        length = 19;
      } else {
        sprintf(isoDate, "%04d-%02u-%02u %02u:%02u:%02u.%03u",
                (int) year, (unsigned int) month, (unsigned int) day,
                (unsigned int) hour, (unsigned int) minute,
                (unsigned int) second, (unsigned int) micro_second / 1000);
        length = 23;
      } /* if */
      if (preparedStmt->executeSuccessful) {
        if (sqlite3_reset(preparedStmt->ppStmt) != SQLITE_OK) {
          logError(printf("sqlBindDuration: sqlite3_reset error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->executeSuccessful = FALSE;
        } /* if */
      } /* if */
      if (sqlite3_bind_text(preparedStmt->ppStmt,
                            (int) pos,
                            (const void *) isoDate,
                            length,
                            &freeText) != SQLITE_OK) {
        logError(printf("sqlBindDuration: sqlite3_bind_text error: %s\n",
                        sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
        raise_error(FILE_ERROR);
      } else {
        preparedStmt->fetchOkay = FALSE;
      } /* if */
    } /* if */
  } /* sqlBindDuration */



static void sqlBindFloat (sqlStmtType sqlStatement, intType pos, floatType value)

  {
    preparedStmtType preparedStmt;

  /* sqlBindFloat */
    /* printf("sqlBindFloat(%lx, " FMT_D ", " FMT_E ")\n",
       (unsigned long) sqlStatement, pos, value); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || pos > INT_MAX)) {
      logError(printf("sqlBindFloat: pos: " FMT_D ", max pos: %u.\n",
                      pos, INT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      if (preparedStmt->executeSuccessful) {
        if (sqlite3_reset(preparedStmt->ppStmt) != SQLITE_OK) {
          logError(printf("sqlBindFloat: sqlite3_reset error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->executeSuccessful = FALSE;
        } /* if */
      } /* if */
      if (sqlite3_bind_double(preparedStmt->ppStmt,
                              (int) pos,
                              (double) value) != SQLITE_OK) {
        logError(printf("sqlBindFloat: sqlite3_bind_double error: %s\n",
                        sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
        raise_error(FILE_ERROR);
      } else {
        preparedStmt->fetchOkay = FALSE;
      } /* if */
    } /* if */
  } /* sqlBindFloat */



static void sqlBindInt (sqlStmtType sqlStatement, intType pos, intType value)

  {
    preparedStmtType preparedStmt;
    int bind_result;

  /* sqlBindInt */
    /* printf("sqlBindInt(%lx, " FMT_D ", " FMT_D ")\n",
       (unsigned long) sqlStatement, pos, value); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || pos > INT_MAX)) {
      logError(printf("sqlBindInt: pos: " FMT_D ", max pos: %u.\n",
                      pos, INT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      if (preparedStmt->executeSuccessful) {
        if (sqlite3_reset(preparedStmt->ppStmt) != SQLITE_OK) {
          logError(printf("sqlBindInt: sqlite3_reset error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->executeSuccessful = FALSE;
        } /* if */
      } /* if */
#if INTTYPE_SIZE == 32
      bind_result = sqlite3_bind_int(preparedStmt->ppStmt,
                                     (int) pos,
                                     (int) value);
#elif INTTYPE_SIZE == 64
      bind_result = sqlite3_bind_int64(preparedStmt->ppStmt,
                                       (int) pos,
                                       (sqlite3_int64) value);
#else
#error "INTTYPE_SIZE is neither 32 nor 64."
      bind_result = SQLITE_ERROR;
#endif
      if (bind_result != SQLITE_OK) {
        logError(printf("sqlBindInt: sqlite3_bind_int error: %s\n",
                        sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
        raise_error(FILE_ERROR);
      } else {
        preparedStmt->fetchOkay = FALSE;
      } /* if */
    } /* if */
  } /* sqlBindInt */



static void sqlBindNull (sqlStmtType sqlStatement, intType pos)

  {
    preparedStmtType preparedStmt;

  /* sqlBindNull */
    /* printf("sqlBindNull(%lx, %d)\n",
       (unsigned long) sqlStatement, pos); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || pos > INT_MAX)) {
      logError(printf("sqlBindNull: pos: " FMT_D ", max pos: %u.\n",
                      pos, INT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      if (preparedStmt->executeSuccessful) {
        if (sqlite3_reset(preparedStmt->ppStmt) != SQLITE_OK) {
          logError(printf("sqlBindNull: sqlite3_reset error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->executeSuccessful = FALSE;
        } /* if */
      } /* if */
      if (sqlite3_bind_null(preparedStmt->ppStmt,
                            (int) pos) != SQLITE_OK) {
        logError(printf("sqlBindNull: sqlite3_bind_null error: %s\n",
                        sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
        raise_error(FILE_ERROR);
      } else {
        preparedStmt->fetchOkay = FALSE;
      } /* if */
    } /* if */
  } /* sqlBindNull */



static void sqlBindStri (sqlStmtType sqlStatement, intType pos, striType stri)

  {
    preparedStmtType preparedStmt;
    cstriType stri8;
    memSizeType length;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindStri */
    /* printf("sqlBindStri(%lx, " FMT_D ", ",
       (unsigned long) sqlStatement, pos);
       prot_stri(stri);
       printf(")\n"); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || pos > INT_MAX)) {
      logError(printf("sqlBindStri: pos: " FMT_D ", max pos: %u.\n",
                      pos, INT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      stri8 = stri_to_cstri8_buf(stri, &length, &err_info);
      if (unlikely(err_info != OKAY_NO_ERROR)) {
        raise_error(err_info);
      } else if (length > INT_MAX) {
        free_cstri8(stri8, stri);
        raise_error(MEMORY_ERROR);
      } else {
        if (preparedStmt->executeSuccessful) {
          if (sqlite3_reset(preparedStmt->ppStmt) != SQLITE_OK) {
            logError(printf("sqlBindStri: sqlite3_reset error: %s\n",
                            sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->executeSuccessful = FALSE;
          } /* if */
        } /* if */
        if (sqlite3_bind_text(preparedStmt->ppStmt,
                              (int) pos,
                              (const void *) stri8,
                              (int) length,
                              &freeText) != SQLITE_OK) {
          logError(printf("sqlBindStri: sqlite3_bind_text error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->fetchOkay = FALSE;
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindStri */



static void sqlBindTime (sqlStmtType sqlStatement, intType pos,
    intType year, intType month, intType day, intType hour,
    intType minute, intType second, intType micro_second)

  {
    preparedStmtType preparedStmt;
    cstriType isoDate;
    int length;

  /* sqlBindTime */
    /* printf("sqlBindTime(%lx, " FMT_D ", %ld, %ld, %ld, %ld, %ld, %ld, %ld)\n",
       sqlStatement, pos, year, month, day, hour, minute, second, micro_second); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || pos > INT_MAX)) {
      logError(printf("sqlBindTime: pos: " FMT_D ", max pos: %u.\n",
                      pos, INT_MAX););
      raise_error(RANGE_ERROR);
    } else if (year < -999 || year > 9999 || month < 1 || month > 12 ||
               day < 1 || day > 31 || hour < 0 || hour > 24 || minute < 0 || minute > 60 ||
               second < 0 || second > 60 || micro_second < 0 || micro_second > 1000000) {
      logError(printf("sqlBindTime: Time not in allowed range.\n"););
      raise_error(RANGE_ERROR);
    } else if (unlikely(!ALLOC_CSTRI(isoDate, 23))) {
      raise_error(MEMORY_ERROR);
    } else {
      if (hour == 0 && minute == 0 && second == 0 && micro_second == 0) {
        sprintf(isoDate, "%04d-%02u-%02u",
                (int) year, (unsigned int) month, (unsigned int) day);
        length = 10;
      } else if (year == 0 && month == 1 && day == 1 && micro_second == 0) {
        sprintf(isoDate, "%02u:%02u:%02u",
                (unsigned int) hour, (unsigned int) minute,
                (unsigned int) second);
        length = 8;
      } else if (micro_second == 0) {
        sprintf(isoDate, "%04d-%02u-%02u %02u:%02u:%02u",
                (int) year, (unsigned int) month, (unsigned int) day,
                (unsigned int) hour, (unsigned int) minute,
                (unsigned int) second);
        length = 19;
      } else {
        sprintf(isoDate, "%04d-%02u-%02u %02u:%02u:%02u.%03u",
                (int) year, (unsigned int) month, (unsigned int) day,
                (unsigned int) hour, (unsigned int) minute,
                (unsigned int) second, (unsigned int) micro_second / 1000);
        length = 23;
      } /* if */
      if (preparedStmt->executeSuccessful) {
        if (sqlite3_reset(preparedStmt->ppStmt) != SQLITE_OK) {
          logError(printf("sqlBindTime: sqlite3_reset error: %s\n",
                          sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->executeSuccessful = FALSE;
        } /* if */
      } /* if */
      if (sqlite3_bind_text(preparedStmt->ppStmt,
                            (int) pos,
                            (const void *) isoDate,
                            length,
                            &freeText) != SQLITE_OK) {
        logError(printf("sqlBindTime: sqlite3_bind_text error: %s\n",
                        sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
        raise_error(FILE_ERROR);
      } else {
        preparedStmt->fetchOkay = FALSE;
      } /* if */
    } /* if */
  } /* sqlBindTime */



static void sqlClose (databaseType database)

  {
    dbType db;

  /* sqlClose */
    db = (dbType) database;
    if (db->connection != NULL) {
      sqlite3_close(db->connection);
    } /* if */
    db->connection = NULL;
  } /* sqlClose */



static bigIntType sqlColumnBigInt (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    int64Type columnValue64;
    const void *blob;
    int length;
    bigIntType columnValue;

  /* sqlColumnBigInt */
    /* printf("sqlColumnBigInt(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_column_count)) {
      logError(printf("sqlColumnBigInt: Fetch okay: %d, column: " FMT_D ", max column: %d.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_column_count););
      raise_error(RANGE_ERROR);
      columnValue = NULL;
    } else {
      /* printf("type: %s\n",
         nameOfBufferType(sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))); */
      switch (sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1)) {
        case SQLITE_NULL:
          columnValue = bigZero();
          break;
        case SQLITE_INTEGER:
          columnValue64 = sqlite3_column_int64(preparedStmt->ppStmt, (int) column - 1);
          columnValue = bigFromInt64(columnValue64);
          break;
        case SQLITE_BLOB:
          blob = sqlite3_column_blob(preparedStmt->ppStmt, (int) column - 1);
          length = sqlite3_column_bytes(preparedStmt->ppStmt, (int) column - 1);
          if (blob == NULL || length < 0) {
            raise_error(FILE_ERROR);
            columnValue = NULL;
          } else {
            columnValue = getDecimalBigInt(blob, (memSizeType) length);
          } /* if */
          break;
        default:
          logError(printf("sqlColumnBigInt: Column " FMT_D " has the unknown type %s.\n",
                          column, nameOfBufferType(
                          sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))););
          raise_error(RANGE_ERROR);
          columnValue = NULL;
          break;
      } /* switch */
    } /* if */
    /* printf("sqlColumnBigInt --> %s\n", bigHexCStri(columnValue)); */
    return columnValue;
  } /* sqlColumnBigInt */



static void sqlColumnBigRat (sqlStmtType sqlStatement, intType column,
    bigIntType *numerator, bigIntType *denominator)

  {
    preparedStmtType preparedStmt;
    int64Type columnValue64;
    const void *blob;
    int length;

  /* sqlColumnBigRat */
    /* printf("sqlColumnBigRat(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_column_count)) {
      logError(printf("sqlColumnBigRat: Fetch okay: %d, column: " FMT_D ", max column: %d.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_column_count););
      raise_error(RANGE_ERROR);
    } else {
      /* printf("type: %s\n",
         nameOfBufferType(sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))); */
      switch (sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1)) {
        case SQLITE_NULL:
          *numerator = bigZero();
          *denominator = bigFromInt32(1);
          break;
        case SQLITE_INTEGER:
          columnValue64 = sqlite3_column_int64(preparedStmt->ppStmt, (int) column - 1);
          *numerator = bigFromInt64(columnValue64);
          *denominator = bigFromInt32(1);
          break;
        case SQLITE_FLOAT:
          *numerator = roundDoubleToBigRat(
              sqlite3_column_double(preparedStmt->ppStmt, (int) column - 1),
              TRUE, denominator);
          break;
        case SQLITE_BLOB:
          blob = sqlite3_column_blob(preparedStmt->ppStmt, (int) column - 1);
          length = sqlite3_column_bytes(preparedStmt->ppStmt, (int) column - 1);
          if (blob == NULL || length < 0) {
            raise_error(FILE_ERROR);
          } else {
            *numerator = getDecimalBigRational(blob, (memSizeType) length,
                                               denominator);
          } /* if */
          break;
        default:
          logError(printf("sqlColumnBigRat: Column " FMT_D " has the unknown type %s.\n",
                          column, nameOfBufferType(
                          sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))););
          raise_error(RANGE_ERROR);
          break;
      } /* switch */
    } /* if */
    /* printf("sqlColumnBigRat --> %s / %s\n",
       bigHexCStri(*numerator), bigHexCStri(*denominator)); */
  } /* sqlColumnBigRat */



static boolType sqlColumnBool (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    const_ustriType stri;
    intType columnValue;

  /* sqlColumnBool */
    /* printf("sqlColumnBool(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_column_count)) {
      logError(printf("sqlColumnBool: Fetch okay: %d, column: " FMT_D ", max column: %d.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_column_count););
      raise_error(RANGE_ERROR);
      columnValue = 0;
    } else {
      /* printf("type: %s\n",
         nameOfBufferType(sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))); */
      switch (sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1)) {
        case SQLITE_NULL:
          columnValue = 0;
          break;
        case SQLITE_INTEGER:
          columnValue = sqlite3_column_int(preparedStmt->ppStmt, (int) column - 1);
          break;
        case SQLITE_TEXT:
          stri = sqlite3_column_text(preparedStmt->ppStmt, (int) column - 1);
          if (stri == NULL) {
            columnValue = 0;
          } else {
            columnValue = stri[0] - '0';
            if (sqlite3_column_bytes(preparedStmt->ppStmt, (int) column - 1) != 1) {
              logError(printf("sqlColumnBool: The size of a boolean field must be 1.\n"););
              raise_error(RANGE_ERROR);
            } /* if */
          } /* if */
          break;
        default:
          logError(printf("sqlColumnBool: Column " FMT_D " has the unknown type %s.\n",
                          column, nameOfBufferType(
                          sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))););
          raise_error(RANGE_ERROR);
          columnValue = 0;
          break;
      } /* switch */
      if ((uintType) columnValue >= 2) {
        raise_error(RANGE_ERROR);
      } /* if */
    } /* if */
    /* printf("sqlColumnBool --> %d\n", columnValue != 0); */
    return columnValue != 0;
  } /* sqlColumnBool */



static bstriType sqlColumnBStri (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    const_ustriType blob;
    const_ustriType stri8;
    int length;
    striType stri;
    errInfoType err_info = OKAY_NO_ERROR;
    bstriType bstri;

  /* sqlColumnBStri */
    /* printf("sqlColumnBStri(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_column_count)) {
      logError(printf("sqlColumnBStri: Fetch okay: %d, column: " FMT_D ", max column: %d.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_column_count););
      raise_error(RANGE_ERROR);
      bstri = NULL;
    } else {
      /* printf("type: %s\n",
         nameOfBufferType(sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))); */
      switch (sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1)) {
        case SQLITE_NULL:
          if (unlikely(!ALLOC_BSTRI_SIZE_OK(bstri, 0))) {
            raise_error(MEMORY_ERROR);
          } else {
            bstri->size = 0;
          } /* if */
          break;
        case SQLITE_BLOB:
          blob = sqlite3_column_blob(preparedStmt->ppStmt, (int) column - 1);
          length = sqlite3_column_bytes(preparedStmt->ppStmt, (int) column - 1);
          if (length < 0) {
            raise_error(FILE_ERROR);
            bstri = NULL;
          } else if (unlikely(!ALLOC_BSTRI_CHECK_SIZE(bstri, (memSizeType) length))) {
            raise_error(MEMORY_ERROR);
          } else {
            bstri->size = (memSizeType) length;
            if (blob != NULL) {
              memcpy(bstri->mem, blob, (memSizeType) length);
            } /* if */
          } /* if */
          break;
        case SQLITE_TEXT:
          stri8 = sqlite3_column_text(preparedStmt->ppStmt, (int) column - 1);
          if (stri8 == NULL) {
            if (unlikely(!ALLOC_BSTRI_SIZE_OK(bstri, 0))) {
              raise_error(MEMORY_ERROR);
            } else {
              bstri->size = 0;
            } /* if */
          } else {
            length = sqlite3_column_bytes(preparedStmt->ppStmt, (int) column - 1);
            if (length < 0) {
              raise_error(FILE_ERROR);
              bstri = NULL;
            } else {
              stri = cstri8_buf_to_stri((cstriType) stri8, (memSizeType) length, &err_info);
              if (err_info != OKAY_NO_ERROR) {
                raise_error(err_info);
                bstri = NULL;
              } else {
                bstri = stri_to_bstri(stri, &err_info);
                strDestr(stri);
                if (err_info != OKAY_NO_ERROR) {
                  raise_error(err_info);
                } /* if */
              } /* if */
            } /* if */
          } /* if */
          break;
        default:
          logError(printf("sqlColumnBStri: Column " FMT_D " has the unknown type %s.\n",
                          column, nameOfBufferType(
                          sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))););
          raise_error(RANGE_ERROR);
          bstri = NULL;
          break;
      } /* switch */
    } /* if */
    /* printf("sqlColumnBStri --> ");
       prot_bstri(bstri);
       printf("\n"); */
    return bstri;
  } /* sqlColumnBStri */



static void sqlColumnDuration (sqlStmtType sqlStatement, intType column,
    intType *year, intType *month, intType *day, intType *hour,
    intType *minute, intType *second, intType *micro_second)

  {
    preparedStmtType preparedStmt;
    const_ustriType isoDate;
    boolType okay;

  /* sqlColumnDuration */
    /* printf("sqlColumnDuration(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_column_count)) {
      logError(printf("sqlColumnDuration: Fetch okay: %d, column: " FMT_D ", max column: %d.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_column_count););
      raise_error(RANGE_ERROR);
    } else {
      /* printf("type: %s\n",
         nameOfBufferType(sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))); */
      isoDate = sqlite3_column_text(preparedStmt->ppStmt, (int) column - 1);
      /* printf("isoDate: %lx\n", (unsigned long int) isoDate); */
      if (isoDate == NULL) {
        *year         = 0;
        *month        = 0;
        *day          = 0;
        *hour         = 0;
        *minute       = 0;
        *second       = 0;
        *micro_second = 0;
      } else {
        switch (strlen((cstriType) isoDate)) {
          case 23:
            okay = sscanf((const_cstriType) isoDate,
                          "%04ld-%02lu-%02lu %02lu:%02lu:%02lu.%03lu",
                          year, month, day, hour, minute, second,
                          micro_second) == 7;
            if (okay) {
              *micro_second *= 1000;
            } /* if */
            break;
          case 19:
            okay = sscanf((const_cstriType) isoDate,
                          "%04ld-%02lu-%02lu %02lu:%02lu:%02lu",
                          year, month, day, hour, minute, second) == 6;
            *micro_second = 0;
            break;
          case 10:
            okay = sscanf((const_cstriType) isoDate,
                          "%04ld-%02lu-%02lu",
                          year, month, day) == 3;
            *hour         = 0;
            *minute       = 0;
            *second       = 0;
            *micro_second = 0;
            break;
          case 8:
            okay = sscanf((const_cstriType) isoDate,
                          "%02lu:%02lu:%02lu",
                          hour, minute, second) == 3;
            *year         = 0;
            *month        = 0;
            *day          = 0;
            *micro_second = 0;
            break;
          default:
            okay = FALSE;
            break;
        } /* switch */
        if (!okay) {
          raise_error(RANGE_ERROR);
        } /* if */
      } /* if */
    } /* if */
    /* printf("sqlColumnDuration --> %d-%02d-%02d %02d:%02d:%02d\n",
       *year, *month, *day, *hour, *minute, *second); */
  } /* sqlColumnDuration */



static floatType sqlColumnFloat (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    const_ustriType blob;
    int length;
    floatType columnValue;

  /* sqlColumnFloat */
    /* printf("sqlColumnFloat(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_column_count)) {
      logError(printf("sqlColumnFloat: Fetch okay: %d, column: " FMT_D ", max column: %d.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_column_count););
      raise_error(RANGE_ERROR);
      columnValue = 0.0;
    } else {
      /* printf("type: %s\n",
         nameOfBufferType(sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))); */
      switch (sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1)) {
        case SQLITE_NULL:
          columnValue = 0.0;
          break;
        case SQLITE_FLOAT:
          columnValue = sqlite3_column_double(preparedStmt->ppStmt, (int) column - 1);
          break;
        case SQLITE_BLOB:
          blob = sqlite3_column_blob(preparedStmt->ppStmt, (int) column - 1);
          length = sqlite3_column_bytes(preparedStmt->ppStmt, (int) column - 1);
          if (blob == NULL || length < 0) {
            raise_error(FILE_ERROR);
            columnValue = 0.0;
          } else {
            columnValue = getDecimalFloat(blob, (memSizeType) length);
          } /* if */
          break;
        default:
          logError(printf("sqlColumnFloat: Column " FMT_D " has the unknown type %s.\n",
                          column, nameOfBufferType(
                          sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))););
          raise_error(RANGE_ERROR);
          columnValue = 0.0;
          break;
      } /* switch */
    } /* if */
    /* printf("sqlColumnFloat --> %f\n", columnValue); */
    return columnValue;
  } /* sqlColumnFloat */



static intType sqlColumnInt (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    intType columnValue;

  /* sqlColumnInt */
    /* printf("sqlColumnInt(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_column_count)) {
      logError(printf("sqlColumnInt: Fetch okay: %d, column: " FMT_D ", max column: %d.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_column_count););
      raise_error(RANGE_ERROR);
      columnValue = 0;
    } else {
      /* printf("type: %s\n",
         nameOfBufferType(sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))); */
      switch (sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1)) {
        case SQLITE_NULL:
          columnValue = 0;
          break;
        case SQLITE_INTEGER:
#if INTTYPE_SIZE == 32
          columnValue = sqlite3_column_int(preparedStmt->ppStmt, (int) column - 1);
#elif INTTYPE_SIZE == 64
          columnValue = sqlite3_column_int64(preparedStmt->ppStmt, (int) column - 1);
#else
#error "INTTYPE_SIZE is neither 32 nor 64."
          raise_error(RANGE_ERROR);
#endif
          break;
        default:
          logError(printf("sqlColumnInt: Column " FMT_D " has the unknown type %s.\n",
                          column, nameOfBufferType(
                          sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))););
          raise_error(RANGE_ERROR);
          columnValue = 0;
          break;
      } /* switch */
    } /* if */
    /* printf("sqlColumnInt --> " FMT_D "\n", columnValue); */
    return columnValue;
  } /* sqlColumnInt */



static striType sqlColumnStri (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    const_ustriType stri8;
    const_ustriType blob;
    int length;
    errInfoType err_info = OKAY_NO_ERROR;
    striType stri;

  /* sqlColumnStri */
    /* printf("sqlColumnStri(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_column_count)) {
      logError(printf("sqlColumnStri: Fetch okay: %d, column: " FMT_D ", max column: %d.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_column_count););
      raise_error(RANGE_ERROR);
      stri = NULL;
    } else {
      /* printf("type: %s\n",
         nameOfBufferType(sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))); */
      switch (sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1)) {
        case SQLITE_NULL:
          stri = strEmpty();
          break;
        case SQLITE_TEXT:
          stri8 = sqlite3_column_text(preparedStmt->ppStmt, (int) column - 1);
          if (stri8 == NULL) {
            stri = strEmpty();
          } else {
            length = sqlite3_column_bytes(preparedStmt->ppStmt, (int) column - 1);
            if (length < 0) {
              raise_error(FILE_ERROR);
              stri = NULL;
            } else {
              stri = cstri8_buf_to_stri((cstriType) stri8, (memSizeType) length, &err_info);
              if (err_info != OKAY_NO_ERROR) {
                raise_error(err_info);
              } /* if */
            } /* if */
          } /* if */
          break;
        case SQLITE_BLOB:
          blob = sqlite3_column_blob(preparedStmt->ppStmt, (int) column - 1);
          if (blob == NULL) {
            stri = strEmpty();
          } else {
            length = sqlite3_column_bytes(preparedStmt->ppStmt, (int) column - 1);
            if (length < 0) {
              raise_error(FILE_ERROR);
              stri = NULL;
            } else {
              stri = cstri_buf_to_stri((const_cstriType) blob, (memSizeType) length);
              if (unlikely(stri == NULL)) {
                raise_error(MEMORY_ERROR);
              } /* if */
            } /* if */
          } /* if */
          break;
        default:
          logError(printf("sqlColumnStri: Column " FMT_D " has the unknown type %s.\n",
                          column, nameOfBufferType(
                          sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))););
          raise_error(RANGE_ERROR);
          stri = NULL;
          break;
      } /* switch */
    } /* if */
    /* printf("sqlColumnStri --> ");
       prot_stri(stri);
       printf("\n"); */
    return stri;
  } /* sqlColumnStri */



static void sqlColumnTime (sqlStmtType sqlStatement, intType column,
    intType *year, intType *month, intType *day, intType *hour,
    intType *minute, intType *second, intType *micro_second,
    intType *time_zone, boolType *is_dst)

  {
    preparedStmtType preparedStmt;
    const_ustriType isoDate;
    boolType okay;
    boolType setYearToZero = FALSE;

  /* sqlColumnTime */
    /* printf("sqlColumnTime(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_column_count)) {
      logError(printf("sqlColumnTime: Fetch okay: %d, column: " FMT_D ", max column: %d.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_column_count););
      raise_error(RANGE_ERROR);
    } else {
      /* printf("type: %s\n",
         nameOfBufferType(sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1))); */
      isoDate = sqlite3_column_text(preparedStmt->ppStmt, (int) column - 1);
      /* printf("isoDate: %lx\n", (unsigned long int) isoDate); */
      if (isoDate == NULL) {
        *year         = 0;
        *month        = 1;
        *day          = 1;
        *hour         = 0;
        *minute       = 0;
        *second       = 0;
        *micro_second = 0;
        *time_zone    = 0;
        *is_dst       = 0;
      } else {
        switch (strlen((cstriType) isoDate)) {
          case 23:
            okay = sscanf((const_cstriType) isoDate,
                          "%04ld-%02lu-%02lu %02lu:%02lu:%02lu.%03lu",
                          year, month, day, hour, minute, second,
                          micro_second) == 7;
            if (okay) {
              *micro_second *= 1000;
            } /* if */
            break;
          case 19:
            okay = sscanf((const_cstriType) isoDate,
                          "%04ld-%02lu-%02lu %02lu:%02lu:%02lu",
                          year, month, day, hour, minute, second) == 6;
            *micro_second = 0;
            break;
          case 10:
            okay = sscanf((const_cstriType) isoDate,
                          "%04ld-%02lu-%02lu",
                          year, month, day) == 3;
            *hour         = 0;
            *minute       = 0;
            *second       = 0;
            *micro_second = 0;
            break;
          case 8:
            okay = sscanf((const_cstriType) isoDate,
                          "%02lu:%02lu:%02lu",
                          hour, minute, second) == 3;
            *year         = 2000;
            *month        = 1;
            *day          = 1;
            *micro_second = 0;
            setYearToZero = TRUE;
            break;
          default:
            okay = FALSE;
            break;
        } /* switch */
        if (!okay) {
          raise_error(RANGE_ERROR);
        } else {
          timSetLocalTZ(*year, *month, *day, *hour, *minute, *second,
                        time_zone, is_dst);
          if (setYearToZero) {
            *year = 0;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
    /* printf("sqlColumnTime --> %d-%02d-%02d %02d:%02d:%02d.%06d %d\n",
       *year, *month, *day, *hour, *minute, *second, *micro_second, *time_zone); */
  } /* sqlColumnTime */



static void sqlCommit (databaseType database)

  {
    dbType db;

  /* sqlCommit */
    db = (dbType) database;
  } /* sqlCommit */



static void sqlExecute (sqlStmtType sqlStatement)

  {
    preparedStmtType preparedStmt;
    int step_result;

  /* sqlExecute */
    /* printf("begin sqlExecute(%lx)\n", (unsigned long) sqlStatement); */
    preparedStmt = (preparedStmtType) sqlStatement;
    /* printf("ppStmt: %lx\n", (unsigned long) preparedStmt->ppStmt); */
    preparedStmt->fetchOkay = FALSE;
    step_result = sqlite3_step(preparedStmt->ppStmt);
    if (step_result == SQLITE_ROW) {
      /* printf("set_result: SQLITE_ROW\n"); */
      preparedStmt->executeSuccessful = TRUE;
      preparedStmt->fetchFinished = FALSE;
      preparedStmt->useStoredFetchResult = TRUE;
      preparedStmt->storedFetchResult = TRUE;
    } else if (step_result == SQLITE_DONE) {
      /* printf("set_result: SQLITE_DONE\n"); */
      preparedStmt->executeSuccessful = TRUE;
      preparedStmt->fetchFinished = FALSE;
      preparedStmt->useStoredFetchResult = TRUE;
      preparedStmt->storedFetchResult = FALSE;
    } else {
      logError(printf("sqlExecute: sqlite3_step error: %s\n",
                      sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
      preparedStmt->executeSuccessful = FALSE;
      raise_error(FILE_ERROR);
    } /* if */
  } /* sqlExecute */



static boolType sqlFetch (sqlStmtType sqlStatement)

  {
    preparedStmtType preparedStmt;
    int step_result;

  /* sqlFetch */
    /* printf("begin sqlFetch(%lx)\n", (unsigned long) sqlStatement); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (!preparedStmt->executeSuccessful) {
      logError(printf("sqlFetch: Execute was not successful\n"););
      preparedStmt->fetchOkay = FALSE;
      raise_error(FILE_ERROR);
    } else if (preparedStmt->useStoredFetchResult) {
      preparedStmt->useStoredFetchResult = FALSE;
      preparedStmt->fetchOkay = preparedStmt->storedFetchResult;
    } else if (!preparedStmt->fetchFinished) {
      /* printf("ppStmt: %lx\n", (unsigned long) preparedStmt->ppStmt); */
      step_result = sqlite3_step(preparedStmt->ppStmt);
      if (step_result == SQLITE_ROW) {
        preparedStmt->fetchOkay = TRUE;
      } else if (step_result == SQLITE_DONE) {
        preparedStmt->fetchOkay = FALSE;
        preparedStmt->fetchFinished = TRUE;
      } else {
        logError(printf("sqlFetch: sqlite3_step error: %s\n",
                        sqlite3_errmsg(sqlite3_db_handle(preparedStmt->ppStmt))););
        preparedStmt->fetchOkay = FALSE;
        preparedStmt->fetchFinished = TRUE;
        raise_error(FILE_ERROR);
      } /* if */
    } /* if */
    /* printf("end sqlFetch --> %d\n", preparedStmt->fetchOkay); */
    return preparedStmt->fetchOkay;
  } /* sqlFetch */



static boolType sqlIsNull (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    boolType isNull;

  /* sqlIsNull */
    /* printf("sqlIsNull(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_column_count)) {
      logError(printf("sqlIsNull: Fetch okay: %d, column: " FMT_D ", max column: %d.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_column_count););
      raise_error(RANGE_ERROR);
      isNull = FALSE;
    } else {
      isNull = sqlite3_column_type(preparedStmt->ppStmt, (int) column - 1) == SQLITE_NULL;
    } /* if */
    /* printf("sqlIsNull --> %d\n", isNull); */
    return isNull;
  } /* sqlIsNull */



static sqlStmtType sqlPrepare (databaseType database, striType sqlStatementStri)

  {
    dbType db;
    cstriType query;
    int prepare_result;
    errInfoType err_info = OKAY_NO_ERROR;
    preparedStmtType preparedStmt;

  /* sqlPrepare */
    /* printf("begin sqlPrepare(database, ");
       prot_stri(sqlStatementStri);
       printf(")\n"); */
    db = (dbType) database;
    query = stri_to_cstri8(sqlStatementStri, &err_info);
    if (query == NULL) {
      preparedStmt = NULL;
    } else {
      if (!ALLOC_RECORD(preparedStmt, preparedStmtRecord, count.prepared_stmt)) {
        err_info = MEMORY_ERROR;
      } else {
        memset(preparedStmt, 0, sizeof(preparedStmtRecord));
        prepare_result = sqlite3_prepare(db->connection, query, -1, &preparedStmt->ppStmt, NULL);
        if (prepare_result != SQLITE_OK) {
          logError(printf("sqlPrepare: sqlite3_prepare error %d: %s\n",
                          prepare_result, sqlite3_errmsg((sqlite3 *) db)););
          FREE_RECORD(preparedStmt, preparedStmtRecord, count.prepared_stmt);
          err_info = FILE_ERROR;
          preparedStmt = NULL;
        } else {
          preparedStmt->usage_count = 1;
          preparedStmt->sqlFunc = db->sqlFunc;
          preparedStmt->executeSuccessful = FALSE;
          preparedStmt->fetchOkay = FALSE;
          preparedStmt->fetchFinished = TRUE;
          preparedStmt->result_column_count = sqlite3_column_count(preparedStmt->ppStmt);
        } /* if */
      } /* if */
      free_cstri8(query, sqlStatementStri);
    } /* if */
    if (err_info != OKAY_NO_ERROR) {
      raise_error(err_info);
    } /* if */
    /* printf("end sqlPrepare\n"); */
    return (sqlStmtType) preparedStmt;
  } /* sqlPrepare */



static intType sqlStmtColumnCount (sqlStmtType sqlStatement)

  {
    preparedStmtType preparedStmt;
    intType columnCount;

  /* sqlStmtColumnCount */
    /* printf("sqlStmtColumnCount(%lx)\n", (unsigned long) sqlStatement); */
    preparedStmt = (preparedStmtType) sqlStatement;
    columnCount = preparedStmt->result_column_count;
    /* printf("sqlStmtColumnCount --> %d\n", columnCount); */
    return columnCount;
  } /* sqlStmtColumnCount */



static striType sqlStmtColumnName (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    const_cstriType col_name;
    errInfoType err_info = OKAY_NO_ERROR;
    striType name;

  /* sqlStmtColumnName */
    /* printf("sqlStmtColumnName(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(column < 1 ||
                 (uintType) column > preparedStmt->result_column_count)) {
      logError(printf("sqlStmtColumnName: column: " FMT_D ", max column: %d.\n",
                      column, preparedStmt->result_column_count););
      raise_error(RANGE_ERROR);
      name = NULL;
    } else {
      col_name = sqlite3_column_name(preparedStmt->ppStmt, (int) column - 1);
      if (col_name == NULL) {
        raise_error(MEMORY_ERROR);
        name = NULL;
      } else {
        name = cstri8_to_stri(col_name, &err_info);
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
        } /* if */
      } /* if */
    } /* if */
    return name;
  } /* sqlStmtColumnName */



static boolType setupFuncTable (void)

  { /* setupFuncTable */
    if (sqlFunc == NULL) {
      if (ALLOC_RECORD(sqlFunc, sqlFuncRecord, cnt)) {
        memset(sqlFunc, 0, sizeof(sqlFuncRecord));
        sqlFunc->freeDatabase       = &freeDatabase;
        sqlFunc->freePreparedStmt   = &freePreparedStmt;
        sqlFunc->sqlBindBigInt      = &sqlBindBigInt;
        sqlFunc->sqlBindBigRat      = &sqlBindBigRat;
        sqlFunc->sqlBindBool        = &sqlBindBool;
        sqlFunc->sqlBindBStri       = &sqlBindBStri;
        sqlFunc->sqlBindDuration    = &sqlBindDuration;
        sqlFunc->sqlBindFloat       = &sqlBindFloat;
        sqlFunc->sqlBindInt         = &sqlBindInt;
        sqlFunc->sqlBindNull        = &sqlBindNull;
        sqlFunc->sqlBindStri        = &sqlBindStri;
        sqlFunc->sqlBindTime        = &sqlBindTime;
        sqlFunc->sqlClose           = &sqlClose;
        sqlFunc->sqlColumnBigInt    = &sqlColumnBigInt;
        sqlFunc->sqlColumnBigRat    = &sqlColumnBigRat;
        sqlFunc->sqlColumnBool      = &sqlColumnBool;
        sqlFunc->sqlColumnBStri     = &sqlColumnBStri;
        sqlFunc->sqlColumnDuration  = &sqlColumnDuration;
        sqlFunc->sqlColumnFloat     = &sqlColumnFloat;
        sqlFunc->sqlColumnInt       = &sqlColumnInt;
        sqlFunc->sqlColumnStri      = &sqlColumnStri;
        sqlFunc->sqlColumnTime      = &sqlColumnTime;
        sqlFunc->sqlCommit          = &sqlCommit;
        sqlFunc->sqlExecute         = &sqlExecute;
        sqlFunc->sqlFetch           = &sqlFetch;
        sqlFunc->sqlIsNull          = &sqlIsNull;
        sqlFunc->sqlPrepare         = &sqlPrepare;
        sqlFunc->sqlStmtColumnCount = &sqlStmtColumnCount;
        sqlFunc->sqlStmtColumnName  = &sqlStmtColumnName;
      } /* if */
    } /* if */
    return sqlFunc != NULL;
  } /* setupFuncTable */



databaseType sqlOpenLite (const const_striType dbName,
    const const_striType user, const const_striType password)

  {
    striType fileName;
    cstriType fileName8;
    const cstriType fileNameMemory = ":memory:";
    const strElemType dbExtension[] = {'.', 'd', 'b'};
    striType dbNameWithExtension = NULL;
    sqlite3 *connection;
    errInfoType err_info = OKAY_NO_ERROR;
    int open_result;
    dbType database;

  /* sqlOpenLite */
    /* printf("sqlOpenLite(");
       prot_stri(dbName);
       printf(", ");
       prot_stri(user);
       printf(", ");
       prot_stri(password);
       printf(")\n"); */
    if (!setupDll(SQLITE_DLL_PATH) && !setupDll(SQLITE_DLL)) {
      logError(printf("sqlOpenLite: setupDll(\"%s\") failed\n", SQLITE_DLL););
      err_info = FILE_ERROR;
      database = NULL;
    } else {
      if (dbName->size == 0) {
        /* Use in memory database: */
        fileName = NULL;
        fileName8 = fileNameMemory;
      } else {
        if (cmdFileType(dbName) == 2) {
          fileName = cmdToOsPath(dbName);
        } else if (dbName->size < 3 ||
                   memcmp(&dbName->mem[dbName->size - 3],
                          dbExtension, 3 * sizeof(strElemType)) != 0) {
          if (unlikely(dbName->size > MAX_STRI_LEN - 3 ||
                       !ALLOC_STRI_SIZE_OK(dbNameWithExtension, dbName->size + 3))) {
            err_info = MEMORY_ERROR;
            fileName = NULL;
          } else {
            dbNameWithExtension->size = dbName->size + 3;
            memcpy(dbNameWithExtension->mem, dbName->mem,
                   dbName->size * sizeof(strElemType));
            memcpy(&dbNameWithExtension->mem[dbName->size], dbExtension,
                   3 * sizeof(strElemType));
            fileName = cmdToOsPath(dbNameWithExtension);
            FREE_STRI(dbNameWithExtension, dbNameWithExtension->size);
          } /* if */
        } else {
          err_info = FILE_ERROR;
          fileName = NULL;
        } /* if */
        /* printf("fileName: ");
           prot_stri(fileName);
           printf("\n"); */
        if (fileName == NULL) {
          /* err_info was set before. */
          fileName8 = NULL;
        } else {
          fileName8 = stri_to_cstri8(fileName, &err_info);
          if (fileName8 == NULL) {
            strDestr(fileName);
          } /* if */
        } /* if */
      } /* if */
      if (fileName8 == NULL) {
        /* err_info was set before. */
        database = NULL;
      } else {
        open_result = sqlite3_open(fileName8, &connection);
        if (open_result != SQLITE_OK) {
          if (connection != NULL) {
            logError(printf("sqlOpenLite: sqlite3_open error %d: %s\n",
                            open_result, sqlite3_errmsg(connection)););
            sqlite3_close(connection);
          } /* if */
          err_info = FILE_ERROR;
          database = NULL;
        } else if (unlikely(!setupFuncTable() ||
                            !ALLOC_RECORD(database, dbRecord, count.database))) {
          err_info = MEMORY_ERROR;
          sqlite3_close(connection);
          database = NULL;
        } else {
          memset(database, 0, sizeof(dbRecord));
          database->usage_count = 1;
          database->sqlFunc = sqlFunc;
          database->connection = connection;
        } /* if */
      } /* if */
      if (fileName8 != NULL && fileName8 != fileNameMemory) {
        free_cstri8(fileName8, fileName);
      } /* if */
      if (fileName != NULL) {
        strDestr(fileName);
      } /* if */
    } /* if */
    if (err_info != OKAY_NO_ERROR) {
      raise_error(err_info);
    } /* if */
    return (databaseType) database;
  } /* sqlOpenLite */

#endif
