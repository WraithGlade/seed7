/********************************************************************/
/*                                                                  */
/*  sql_odbc.c    Database access functions for the ODBC interface. */
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
/*  File: seed7/src/sql_odbc.c                                      */
/*  Changes: 2014  Thomas Mertes                                    */
/*  Content: Database access functions for the ODBC interface.      */
/*                                                                  */
/********************************************************************/

#include "version.h"

#ifdef ODBC_INCLUDE
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "limits.h"
#ifdef WINDOWS_ODBC
#include "windows.h"
#endif
#include ODBC_INCLUDE
#ifdef ODBC_INCLUDE_SQLEXT
#include "sqlext.h"
#endif

#include "common.h"
#include "data_rtl.h"
#include "striutl.h"
#include "heaputl.h"
#include "str_rtl.h"
#include "bst_rtl.h"
#include "int_rtl.h"
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
    SQLHENV      sql_environment;
    SQLHDBC      sql_connection;
  } dbRecord, *dbType;

typedef struct {
    int          buffer_type;
    memSizeType  buffer_length;
    memSizeType  buffer_capacity;
    void        *buffer;
    SQLLEN       length;
  } bindDataRecord, *bindDataType;

typedef struct {
    int                 buffer_type;
    memSizeType         buffer_length;
    void               *buffer;
    SQLLEN              length;
  } resultDataRecord, *resultDataType;

typedef struct {
    uintType       usage_count;
    sqlFuncType    sqlFunc;
    SQLHSTMT       ppStmt;
    memSizeType    param_array_capacity;
    memSizeType    param_array_size;
    bindDataType   param_array;
    memSizeType    result_array_size;
    resultDataType result_array;
    boolType       executeSuccessful;
    boolType       fetchOkay;
    boolType       fetchFinished;
  } preparedStmtRecord, *preparedStmtType;

static sqlFuncType sqlFunc = NULL;

/* ODBC provides two possibilities to encode decimal values.        */
/*  1. As string of decimal digits.                                 */
/*  2. As binary encoded data in the struct SQL_NUMERIC_STRUCT.     */
/* Some databases provide decimal values beyond the capabilities of */
/* SQL_NUMERIC_STRUCT. Additionally SQL_NUMERIC_STRUCT values are   */
/* not correctly supported by some drivers. Therefore decimal       */
/* encoding is the default and encoding DECODE_NUMERIC_STRUCT       */
/* should be used with care.                                        */
#undef DECODE_NUMERIC_STRUCT
#undef ENCODE_NUMERIC_STRUCT
/* Maximum number of decimal digits that fits in in SQL_NUMERIC_STRUCT */
#define MAX_NUMERIC_PRECISION 39
#define MAX_NUMERIC_SCALE 39
#define MAX_DECIMAL_LENGTH 67
#define DEFAULT_DECIMAL_SCALE 1000
#define DEFAULT_DECIMAL_PRECISION 1000
#define SQLLEN_MAX (SQLLEN) (((SQLULEN) 1 << (8 * sizeof(SQLLEN) - 1)) - 1)
#define SQLINTEGER_MAX (SQLINTEGER) (((SQLUINTEGER) 1 << (8 * sizeof(SQLINTEGER) - 1)) - 1)
#define SQLSMALLINT_MAX (SQLSMALLINT) (((SQLUSMALLINT) 1 << (8 * sizeof(SQLSMALLINT) - 1)) - 1)

#ifdef VERBOSE_EXCEPTIONS
#define logError(logStatements) printf(" *** "); logStatements
#else
#define logError(logStatements)
#endif


#ifdef ODBC_DLL

#ifndef STDCALL
#if defined(_WIN32)
#define STDCALL __stdcall
#else
#define STDCALL
#endif
#endif

typedef SQLRETURN (STDCALL *tp_SQLAllocHandle) (SQLSMALLINT HandleType,
                                                SQLHANDLE InputHandle, SQLHANDLE *OutputHandle);
typedef SQLRETURN (STDCALL *tp_SQLBindCol) (SQLHSTMT StatementHandle,
                                            SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                                            SQLPOINTER TargetValue, SQLLEN BufferLength,
                                            SQLLEN *StrLen_or_Ind);
typedef SQLRETURN (STDCALL *tp_SQLBindParameter) (SQLHSTMT hstmt,
                                                  SQLUSMALLINT ipar,
                                                  SQLSMALLINT  fParamType,
                                                  SQLSMALLINT  fCType,
                                                  SQLSMALLINT  fSqlType,
                                                  SQLULEN      cbColDef,
                                                  SQLSMALLINT  ibScale,
                                                  SQLPOINTER   rgbValue,
                                                  SQLLEN       cbValueMax,
                                                  SQLLEN      *pcbValue);
typedef SQLRETURN (STDCALL *tp_SQLColAttribute) (SQLHSTMT StatementHandle,
                                                 SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
                                                 SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
                                                 SQLSMALLINT *StringLength, SQLLEN *NumericAttribute);
typedef SQLRETURN (STDCALL *tp_SQLColAttributeW) (SQLHSTMT hstmt,
                                                  SQLUSMALLINT iCol,
                                                  SQLUSMALLINT iField,
                                                  SQLPOINTER  pCharAttr,
                                                  SQLSMALLINT  cbCharAttrMax,
                                                  SQLSMALLINT *pcbCharAttr,
                                                  SQLLEN  *pNumAttr);
typedef SQLRETURN (STDCALL *tp_SQLConnectW) (SQLHDBC ConnectionHandle,
                                             SQLWCHAR *ServerName, SQLSMALLINT NameLength1,
                                             SQLWCHAR *UserName, SQLSMALLINT NameLength2,
                                             SQLWCHAR *Authentication, SQLSMALLINT NameLength3);
typedef SQLRETURN (STDCALL *tp_SQLDataSources) (SQLHENV EnvironmentHandle,
                                                SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                                SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1,
                                                SQLCHAR *Description, SQLSMALLINT BufferLength2,
                                                SQLSMALLINT *NameLength2);
typedef SQLRETURN (STDCALL *tp_SQLDisconnect) (SQLHDBC ConnectionHandle);
typedef SQLRETURN (STDCALL *tp_SQLDrivers) (SQLHENV      henv,
                                            SQLUSMALLINT fDirection,
                                            SQLCHAR     *szDriverDesc,
                                            SQLSMALLINT  cbDriverDescMax,
                                            SQLSMALLINT *pcbDriverDesc,
                                            SQLCHAR     *szDriverAttributes,
                                            SQLSMALLINT  cbDrvrAttrMax,
                                            SQLSMALLINT *pcbDrvrAttr);
typedef SQLRETURN (STDCALL *tp_SQLExecute) (SQLHSTMT StatementHandle);
typedef SQLRETURN (STDCALL *tp_SQLFetch) (SQLHSTMT StatementHandle);
typedef SQLRETURN (STDCALL *tp_SQLFreeHandle) (SQLSMALLINT HandleType, SQLHANDLE Handle);
typedef SQLRETURN (STDCALL *tp_SQLFreeStmt) (SQLHSTMT StatementHandle, SQLUSMALLINT Option);
typedef SQLRETURN (STDCALL *tp_SQLGetDiagRec) (SQLSMALLINT HandleType, SQLHANDLE Handle,
                                               SQLSMALLINT RecNumber, SQLCHAR *Sqlstate,
                                               SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                               SQLSMALLINT BufferLength, SQLSMALLINT *TextLength);
typedef SQLRETURN (STDCALL *tp_SQLGetStmtAttr) (SQLHSTMT StatementHandle,
                                                SQLINTEGER Attribute, SQLPOINTER Value,
                                                SQLINTEGER BufferLength, SQLINTEGER *StringLength);
typedef SQLRETURN (STDCALL *tp_SQLNumResultCols) (SQLHSTMT StatementHandle,
                                                  SQLSMALLINT *ColumnCount);
typedef SQLRETURN (STDCALL *tp_SQLPrepareW) (SQLHSTMT   hstmt,
                                             SQLWCHAR  *szSqlStr,
                                             SQLINTEGER cbSqlStr);
typedef SQLRETURN (STDCALL *tp_SQLSetDescField) (SQLHDESC DescriptorHandle,
                                                 SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                                 SQLPOINTER Value, SQLINTEGER BufferLength);
typedef SQLRETURN (STDCALL *tp_SQLSetEnvAttr) (SQLHENV EnvironmentHandle,
                                               SQLINTEGER Attribute, SQLPOINTER Value,
                                               SQLINTEGER StringLength);

tp_SQLAllocHandle   ptr_SQLAllocHandle;
tp_SQLBindCol       ptr_SQLBindCol;
tp_SQLBindParameter ptr_SQLBindParameter;
tp_SQLColAttribute  ptr_SQLColAttribute;
tp_SQLColAttributeW ptr_SQLColAttributeW;
tp_SQLConnectW      ptr_SQLConnectW;
tp_SQLDataSources   ptr_SQLDataSources;
tp_SQLDisconnect    ptr_SQLDisconnect;
tp_SQLDrivers       ptr_SQLDrivers;
tp_SQLExecute       ptr_SQLExecute;
tp_SQLFetch         ptr_SQLFetch;
tp_SQLFreeHandle    ptr_SQLFreeHandle;
tp_SQLFreeStmt      ptr_SQLFreeStmt;
tp_SQLGetDiagRec    ptr_SQLGetDiagRec;
tp_SQLGetStmtAttr   ptr_SQLGetStmtAttr;
tp_SQLNumResultCols ptr_SQLNumResultCols;
tp_SQLPrepareW      ptr_SQLPrepareW;
tp_SQLSetDescField  ptr_SQLSetDescField;
tp_SQLSetEnvAttr    ptr_SQLSetEnvAttr;

#define SQLAllocHandle   ptr_SQLAllocHandle
#define SQLBindCol       ptr_SQLBindCol
#define SQLBindParameter ptr_SQLBindParameter
#define SQLColAttribute  ptr_SQLColAttribute
#define SQLColAttributeW ptr_SQLColAttributeW
#define SQLConnectW      ptr_SQLConnectW
#define SQLDataSources   ptr_SQLDataSources
#define SQLDisconnect    ptr_SQLDisconnect
#define SQLDrivers       ptr_SQLDrivers
#define SQLExecute       ptr_SQLExecute
#define SQLFetch         ptr_SQLFetch
#define SQLFreeHandle    ptr_SQLFreeHandle
#define SQLFreeStmt      ptr_SQLFreeStmt
#define SQLGetDiagRec    ptr_SQLGetDiagRec
#define SQLGetStmtAttr   ptr_SQLGetStmtAttr
#define SQLNumResultCols ptr_SQLNumResultCols
#define SQLPrepareW      ptr_SQLPrepareW
#define SQLSetDescField  ptr_SQLSetDescField
#define SQLSetEnvAttr    ptr_SQLSetEnvAttr



static boolType setupDll (const char *dllName)

  {
    static void *dbDll = NULL;

  /* setupDll */
    /* printf("setupDll(\"%s\")\n", dllName); */
    if (dbDll == NULL) {
      dbDll = dllOpen(dllName);
      if (dbDll != NULL) {
        if ((SQLAllocHandle   = (tp_SQLAllocHandle)   dllSym(dbDll, "SQLAllocHandle"))   == NULL ||
            (SQLBindCol       = (tp_SQLBindCol)       dllSym(dbDll, "SQLBindCol"))       == NULL ||
            (SQLBindParameter = (tp_SQLBindParameter) dllSym(dbDll, "SQLBindParameter")) == NULL ||
            (SQLColAttribute  = (tp_SQLColAttribute)  dllSym(dbDll, "SQLColAttribute"))  == NULL ||
            (SQLColAttributeW = (tp_SQLColAttributeW) dllSym(dbDll, "SQLColAttributeW")) == NULL ||
            (SQLConnectW      = (tp_SQLConnectW)      dllSym(dbDll, "SQLConnectW"))      == NULL ||
            (SQLDisconnect    = (tp_SQLDisconnect)    dllSym(dbDll, "SQLDisconnect"))    == NULL ||
            (SQLExecute       = (tp_SQLExecute)       dllSym(dbDll, "SQLExecute"))       == NULL ||
            (SQLFetch         = (tp_SQLFetch)         dllSym(dbDll, "SQLFetch"))         == NULL ||
            (SQLFreeHandle    = (tp_SQLFreeHandle)    dllSym(dbDll, "SQLFreeHandle"))    == NULL ||
            (SQLFreeStmt      = (tp_SQLFreeStmt)      dllSym(dbDll, "SQLFreeStmt"))      == NULL ||
            (SQLGetDiagRec    = (tp_SQLGetDiagRec)    dllSym(dbDll, "SQLGetDiagRec"))    == NULL ||
            (SQLGetStmtAttr   = (tp_SQLGetStmtAttr)   dllSym(dbDll, "SQLGetStmtAttr"))   == NULL ||
            (SQLNumResultCols = (tp_SQLNumResultCols) dllSym(dbDll, "SQLNumResultCols")) == NULL ||
            (SQLPrepareW      = (tp_SQLPrepareW)      dllSym(dbDll, "SQLPrepareW"))      == NULL ||
            (SQLSetDescField  = (tp_SQLSetDescField)  dllSym(dbDll, "SQLSetDescField"))  == NULL ||
            (SQLSetEnvAttr    = (tp_SQLSetEnvAttr)    dllSym(dbDll, "SQLSetEnvAttr"))    == NULL) {
          dbDll = NULL;
        } /* if */
      } /* if */
    } /* if */
    /* printf("setupDll --> %d\n", dbDll != NULL); */
    return dbDll != NULL;
  } /* setupDll */

#else

#define setupDll(dllName) TRUE
#define ODBC_DLL ""

#endif



static void sqlClose (databaseType database);



char *strfill (char *s, unsigned int len, int fill)

  { /* strfill */
    while (len--) {
      *s++ = (char) fill;
    } /* while */
    *s = '\0';
    return s;
  } /* strfill */



#ifdef VERBOSE_EXCEPTIONS
static void printError (SQLSMALLINT handleType, SQLHANDLE handle)

  {
    SQLRETURN returnCode;
    SQLCHAR sqlState[1024];
    SQLINTEGER nativeError;
    SQLCHAR *messageText;
    SQLSMALLINT bufferLength;

  /* printError */
    returnCode = SQLGetDiagRec(handleType,
                               handle,
                               1,
                               sqlState,
                               &nativeError,
                               NULL,
                               0,
                               &bufferLength);
    if (returnCode == SQL_NO_DATA) {
      printf(" *** SQLGetDiagRec returned: SQL_NO_DATA\n");
    } else if (returnCode != SQL_SUCCESS &&
               returnCode != SQL_SUCCESS_WITH_INFO) {
      printf(" *** SQLGetDiagRec returned: %d\n", returnCode);
    } else if (bufferLength < 0 || bufferLength >= SQLSMALLINT_MAX) {
      printf(" *** SQLGetDiagRec: BufferLength not in allowed range: %hd\n", bufferLength);
    } else {
      messageText = malloc((memSizeType) bufferLength + 1);
      if (messageText == NULL) {
        printf(" *** malloc failed\n");
      } else {
        if (SQLGetDiagRec(handleType,
                          handle,
                          1,
                          sqlState,
                          &nativeError,
                          messageText,
                          (SQLSMALLINT) (bufferLength + 1),
                          &bufferLength) != SQL_SUCCESS) {
          printf(" *** SQLGetDiagRec failed\n");
        } else {
          printf("%s\n", messageText);
          printf("SQLState: %s\n", sqlState);
          printf("NativeError: %d\n", nativeError);
        } /* if */
        free(messageText);
      } /* if */
    } /* if */
  } /* printError */
#endif



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
    memSizeType pos;

  /* freePreparedStmt */
    /* printf("begin freePreparedStmt(%lx)\n", (unsigned long) sqlStatement); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (preparedStmt->param_array != NULL) {
      for (pos = 0; pos < preparedStmt->param_array_size; pos++) {
        free(preparedStmt->param_array[pos].buffer);
      } /* for */
      FREE_TABLE(preparedStmt->param_array, bindDataRecord, preparedStmt->param_array_capacity);
    } /* if */
    if (preparedStmt->result_array != NULL) {
      for (pos = 0; pos < preparedStmt->result_array_size; pos++) {
        free(preparedStmt->result_array[pos].buffer);
      } /* for */
      FREE_TABLE(preparedStmt->result_array, resultDataRecord, preparedStmt->result_array_size);
    } /* if */
    if (preparedStmt->executeSuccessful &&
        (preparedStmt->fetchOkay || preparedStmt->fetchFinished)) {
      if (SQLFreeStmt(preparedStmt->ppStmt, SQL_CLOSE) != SQL_SUCCESS) {
        logError(printf("freePreparedStmt: SQLFreeStmt SQL_CLOSE:\n");
                 printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
        raise_error(FILE_ERROR);
      } /* if */
    } /* if */
    SQLFreeHandle(SQL_HANDLE_STMT, preparedStmt->ppStmt);
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
      case SQL_CHAR:                      typeName = "SQL_CHAR"; break;
      case SQL_VARCHAR:                   typeName = "SQL_VARCHAR"; break;
      case SQL_LONGVARCHAR:               typeName = "SQL_LONGVARCHAR"; break;
      case SQL_WCHAR:                     typeName = "SQL_WCHAR"; break;
      case SQL_WVARCHAR:                  typeName = "SQL_WVARCHAR"; break;
      case SQL_WLONGVARCHAR:              typeName = "SQL_WLONGVARCHAR"; break;
      case SQL_TINYINT:                   typeName = "SQL_TINYINT"; break;
      case SQL_SMALLINT:                  typeName = "SQL_SMALLINT"; break;
      case SQL_INTEGER:                   typeName = "SQL_INTEGER"; break;
      case SQL_BIGINT:                    typeName = "SQL_BIGINT"; break;
      case SQL_DECIMAL:                   typeName = "SQL_DECIMAL"; break;
      case SQL_NUMERIC:                   typeName = "SQL_NUMERIC"; break;
      case SQL_REAL:                      typeName = "SQL_REAL"; break;
      case SQL_FLOAT:                     typeName = "SQL_FLOAT"; break;
      case SQL_DOUBLE:                    typeName = "SQL_DOUBLE"; break;
      case SQL_TYPE_DATE:                 typeName = "SQL_TYPE_DATE"; break;
      case SQL_TYPE_TIME:                 typeName = "SQL_TYPE_TIME"; break;
      case SQL_DATETIME:                  typeName = "SQL_DATETIME"; break;
      case SQL_TYPE_TIMESTAMP:            typeName = "SQL_TYPE_TIMESTAMP"; break;
      case SQL_INTERVAL_YEAR:             typeName = "SQL_INTERVAL_YEAR"; break;
      case SQL_INTERVAL_MONTH:            typeName = "SQL_INTERVAL_MONTH"; break;
      case SQL_INTERVAL_DAY:              typeName = "SQL_INTERVAL_DAY"; break;
      case SQL_INTERVAL_HOUR:             typeName = "SQL_INTERVAL_HOUR"; break;
      case SQL_INTERVAL_MINUTE:           typeName = "SQL_INTERVAL_MINUTE"; break;
      case SQL_INTERVAL_SECOND:           typeName = "SQL_INTERVAL_SECOND"; break;
      case SQL_INTERVAL_YEAR_TO_MONTH:    typeName = "SQL_INTERVAL_YEAR_TO_MONTH"; break;
      case SQL_INTERVAL_DAY_TO_HOUR:      typeName = "SQL_INTERVAL_DAY_TO_HOUR"; break;
      case SQL_INTERVAL_DAY_TO_MINUTE:    typeName = "SQL_INTERVAL_DAY_TO_MINUTE"; break;
      case SQL_INTERVAL_DAY_TO_SECOND:    typeName = "SQL_INTERVAL_DAY_TO_SECOND"; break;
      case SQL_INTERVAL_HOUR_TO_MINUTE:   typeName = "SQL_INTERVAL_HOUR_TO_MINUTE"; break;
      case SQL_INTERVAL_HOUR_TO_SECOND:   typeName = "SQL_INTERVAL_HOUR_TO_SECOND"; break;
      case SQL_INTERVAL_MINUTE_TO_SECOND: typeName = "SQL_INTERVAL_MINUTE_TO_SECOND"; break;
      case SQL_BINARY:                    typeName = "SQL_BINARY"; break;
      case SQL_VARBINARY:                 typeName = "SQL_VARBINARY"; break;
      case SQL_LONGVARBINARY:             typeName = "SQL_LONGVARBINARY"; break;
      default:
        sprintf(buffer, "%d", buffer_type);
        typeName = buffer;
        break;
    } /* switch */
    /* printf("nameOfBufferType --> %s\n", typeName); */
    return typeName;
  } /* nameOfBufferType */



static const char *nameOfCType (int buffer_type)

  {
    static char buffer[50];
    const char *typeName;

  /* nameOfCType */
    switch (buffer_type) {
      case SQL_C_CHAR:                      typeName = "SQL_C_CHAR"; break;
      case SQL_C_WCHAR:                     typeName = "SQL_C_WCHAR"; break;
      case SQL_C_STINYINT:                  typeName = "SQL_C_STINYINT"; break;
      case SQL_C_SSHORT:                    typeName = "SQL_C_SSHORT"; break;
      case SQL_C_SLONG:                     typeName = "SQL_C_SLONG"; break;
      case SQL_C_SBIGINT:                   typeName = "SQL_C_SBIGINT"; break;
      case SQL_C_NUMERIC:                   typeName = "SQL_C_NUMERIC"; break;
      case SQL_C_FLOAT:                     typeName = "SQL_C_FLOAT"; break;
      case SQL_C_DOUBLE:                    typeName = "SQL_C_DOUBLE"; break;
      case SQL_C_TYPE_DATE:                 typeName = "SQL_C_TYPE_DATE"; break;
      case SQL_C_TYPE_TIME:                 typeName = "SQL_C_TYPE_TIME"; break;
      case SQL_C_TYPE_TIMESTAMP:            typeName = "SQL_C_TYPE_TIMESTAMP"; break;
      case SQL_C_INTERVAL_YEAR:             typeName = "SQL_C_INTERVAL_YEAR"; break;
      case SQL_C_INTERVAL_MONTH:            typeName = "SQL_C_INTERVAL_MONTH"; break;
      case SQL_C_INTERVAL_DAY:              typeName = "SQL_C_INTERVAL_DAY"; break;
      case SQL_C_INTERVAL_HOUR:             typeName = "SQL_C_INTERVAL_HOUR"; break;
      case SQL_C_INTERVAL_MINUTE:           typeName = "SQL_C_INTERVAL_MINUTE"; break;
      case SQL_C_INTERVAL_SECOND:           typeName = "SQL_C_INTERVAL_SECOND"; break;
      case SQL_C_INTERVAL_YEAR_TO_MONTH:    typeName = "SQL_C_INTERVAL_YEAR_TO_MONTH"; break;
      case SQL_C_INTERVAL_DAY_TO_HOUR:      typeName = "SQL_C_INTERVAL_DAY_TO_HOUR"; break;
      case SQL_C_INTERVAL_DAY_TO_MINUTE:    typeName = "SQL_C_INTERVAL_DAY_TO_MINUTE"; break;
      case SQL_C_INTERVAL_DAY_TO_SECOND:    typeName = "SQL_C_INTERVAL_DAY_TO_SECOND"; break;
      case SQL_C_INTERVAL_HOUR_TO_MINUTE:   typeName = "SQL_C_INTERVAL_HOUR_TO_MINUTE"; break;
      case SQL_C_INTERVAL_HOUR_TO_SECOND:   typeName = "SQL_C_INTERVAL_HOUR_TO_SECOND"; break;
      case SQL_C_INTERVAL_MINUTE_TO_SECOND: typeName = "SQL_C_INTERVAL_MINUTE_TO_SECOND"; break;
      case SQL_C_BINARY:                    typeName = "SQL_C_BINARY"; break;
      default:
        sprintf(buffer, "%d", buffer_type);
        typeName = buffer;
        break;
    } /* switch */
    return typeName;
  } /* nameOfCType */



static void setupResultColumn (preparedStmtType preparedStmt,
    SQLSMALLINT column_num, resultDataType resultData,
    errInfoType *err_info)

  {
    SQLSMALLINT c_type;
    SQLLEN sqlDataType;
    SQLLEN dataLength;
    SQLLEN octetLength;
    SQLLEN precision;
    SQLLEN scale;
    SQLHDESC descriptorHandle;
    memSizeType column_size;

  /* setupResultColumn */
    /* printf("setupResultColumn: column_num=%d\n", column_num); */
    if (SQLColAttribute(preparedStmt->ppStmt,
                        (SQLUSMALLINT) column_num,
                        SQL_DESC_TYPE,
                        NULL,
                        0,
                        NULL,
                        &sqlDataType) != SQL_SUCCESS) {
      logError(printf("setupResultColumn: SQLColAttribute SQL_DESC_TYPE:\n");
               printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
      *err_info = FILE_ERROR;
    } else if (SQLColAttribute(preparedStmt->ppStmt,
                               (SQLUSMALLINT) column_num,
                               SQL_DESC_LENGTH,
                               NULL,
                               0,
                               NULL,
                               &dataLength) != SQL_SUCCESS) {
      logError(printf("setupResultColumn: SQLColAttribute SQL_DESC_LENGTH:\n");
               printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
      *err_info = FILE_ERROR;
    } else if (SQLColAttribute(preparedStmt->ppStmt,
                               (SQLUSMALLINT) column_num,
                               SQL_DESC_OCTET_LENGTH,
                               NULL,
                               0,
                               NULL,
                               &octetLength) != SQL_SUCCESS) {
      logError(printf("setupResultColumn: SQLColAttribute SQL_DESC_OCTET_LENGTH:\n");
               printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
      *err_info = FILE_ERROR;
    } else if (SQLColAttribute(preparedStmt->ppStmt,
                               (SQLUSMALLINT) column_num,
                               SQL_DESC_PRECISION,
                               NULL,
                               0,
                               NULL,
                               &precision) != SQL_SUCCESS) {
      logError(printf("setupResultColumn: SQLColAttribute SQL_DESC_PRECISION:\n");
               printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
      *err_info = FILE_ERROR;
    } else if (SQLColAttribute(preparedStmt->ppStmt,
                               (SQLUSMALLINT) column_num,
                               SQL_DESC_SCALE,
                               NULL,
                               0,
                               NULL,
                               &scale) != SQL_SUCCESS) {
      logError(printf("setupResultColumn: SQLColAttribute SQL_DESC_SCALE:\n");
               printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
      *err_info = FILE_ERROR;
    } else {
      /* printf("sqlDataType: %ld\n", sqlDataType); */
      resultData->buffer_type = (int) sqlDataType;
      /* printf("buffer_type: %s\n", nameOfBufferType(resultData->buffer_type)); */
      switch (resultData->buffer_type) {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
        case SQL_WCHAR:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:
          c_type = SQL_C_WCHAR;
          if (dataLength < 0) {
            logError(printf("setupResultColumn: DataLength negative: %ld\n",
                            dataLength););
            *err_info = FILE_ERROR;
          } else if ((SQLULEN) dataLength > (MAX_MEMSIZETYPE / 2) - 1) {
            logError(printf("setupResultColumn: DataLength too big: %ld\n",
                            dataLength););
            *err_info = MEMORY_ERROR;
          } else {
            column_size = ((memSizeType) dataLength + 1) * 2;
          } /* if */
          /* printf("%s:\n", nameOfBufferType(resultData->buffer_type));
          printf("octetLength: %ld\n", octetLength);
          printf("column_size: %lu\n", column_size);
          printf("precision: %d\n", precision);
          printf("scale: %d\n", scale); */
          break;
        case SQL_TINYINT:
          c_type = SQL_C_STINYINT;
          column_size = sizeof(int8Type);
          break;
        case SQL_SMALLINT:
          c_type = SQL_C_SSHORT;
          column_size = sizeof(int16Type);
          break;
        case SQL_INTEGER:
          c_type = SQL_C_SLONG;
          column_size = sizeof(int32Type);
          break;
        case SQL_BIGINT:
          c_type = SQL_C_SBIGINT;
          column_size = sizeof(int64Type);
          break;
        case SQL_DECIMAL:
          c_type = SQL_C_CHAR;
          if (octetLength < 0) {
            logError(printf("setupResultColumn: OctetLength negative: %ld\n",
                            octetLength););
            *err_info = FILE_ERROR;
          } else if ((SQLULEN) octetLength > MAX_MEMSIZETYPE - 1) {
            logError(printf("setupResultColumn: OctetLength too big: %ld\n",
                            octetLength););
            *err_info = MEMORY_ERROR;
          } else {
            /* Add place for zero byte. */
            column_size = (memSizeType) octetLength + 1;
          } /* if */
          /* printf("SQL_DECIMAL:\n");
          printf("octetLength: %ld\n", octetLength);
          printf("DataLength: %ld\n", dataLength);
          printf("column_size: %lu\n", column_size);
          printf("precision: %d\n", precision);
          printf("scale: %d\n", scale); */
          break;
        case SQL_NUMERIC:
#if 0
          printf("SQL_NUMERIC:\n");
          printf("octetLength: %ld\n", octetLength);
          printf("DataLength: %ld\n", dataLength);
          printf("precision: %ld\n", precision);
          printf("scale: %ld\n", scale);
          {
          SQLLEN displaySize;

          if (SQLColAttribute(preparedStmt->ppStmt,
                              (SQLUSMALLINT) column_num,
                              SQL_DESC_DISPLAY_SIZE,
                              NULL,
                              0,
                              NULL,
                              &displaySize) != SQL_SUCCESS) {
            logError(printf("setupResultColumn: SQLColAttribute SQL_DESC_DISPLAY_SIZE:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
          } else {
            printf("Display size: %ld\n", displaySize);
          }
          }
          {
          SQLLEN precisionRadix;

          if (SQLColAttribute(preparedStmt->ppStmt,
                              (SQLUSMALLINT) column_num,
                              SQL_DESC_NUM_PREC_RADIX ,
                              NULL,
                              0,
                              NULL,
                              &precisionRadix) != SQL_SUCCESS) {
            logError(printf("setupResultColumn: SQLColAttribute SQL_DESC_DISPLAY_SIZE:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
          } else {
            printf("Precision radix: %ld\n", precisionRadix);
          }
          }
          {
            SQLSMALLINT   NameLength;
            SQLSMALLINT   DataType;
            SQLULEN       ColumnSize;
            SQLSMALLINT   DecimalDigits;
            SQLSMALLINT   Nullable;

          if (SQLDescribeCol(preparedStmt->ppStmt,
                             (SQLUSMALLINT) column_num,
                             NULL,
                             0,
                             &NameLength,
                             &DataType,
                             &ColumnSize,
                             &DecimalDigits,
                             &Nullable) != SQL_SUCCESS) {
            printf("SQLDescribeCol failed\n");
          } else {
            printf("NameLength:    %d\n",  NameLength);
            printf("DataType:      %d\n",  DataType);
            printf("ColumnSize:    %ld\n", ColumnSize);
            printf("DecimalDigits: %d\n",  DecimalDigits);
            printf("Nullable:      %d\n",  Nullable);
          }
          }
          {
            SQLLEN precision2;

          if (SQLGetStmtAttr(preparedStmt->ppStmt,
                             SQL_ATTR_APP_ROW_DESC,
                             &descriptorHandle,
                             0, NULL) != SQL_SUCCESS) {
            logError(printf("setupResultColumn: SQLGetStmtAttr:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
            *err_info = FILE_ERROR;
          } /* if */
          if (SQLGetDescField(descriptorHandle,
                              column_num,
                              SQL_DESC_PRECISION,
                              (SQLPOINTER) &precision2,
                              sizeof(precision2),
                              NULL) != SQL_SUCCESS) {
            logError(printf("setupResultColumn: SQLGetDescField:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
          } else {
            printf("precision2: %ld\n", precision2);
          }
          }
#endif
#ifdef DECODE_NUMERIC_STRUCT
          c_type = SQL_C_NUMERIC;
          column_size = sizeof(SQL_NUMERIC_STRUCT);
#else
          c_type = SQL_C_CHAR;
          if (precision < 0 || (SQLULEN) precision < DEFAULT_DECIMAL_PRECISION) {
            column_size = DEFAULT_DECIMAL_PRECISION;
          } else {
            column_size = (SQLULEN) precision;
          } /* if */
          /* Place for sign, decimal point and zero byte. */
          column_size += 3;
#if 0
          if (precision < 0) {
            logError(printf("setupResultColumn: Precision negative: %ld\n",
                            precision););
            *err_info = FILE_ERROR;
          } else if (precision > MAX_MEMSIZETYPE - 3) {
            logError(printf("setupResultColumn: Precision too big: %ld\n",
                            precision););
            *err_info = MEMORY_ERROR;
          } else {
            /* Place for digits, sign, decimal point and zero byte. */
            column_size = (memSizeType) precision + 3;
          } /* if */
#endif
#endif
          break;
        case SQL_REAL:
          c_type = SQL_C_FLOAT;
          column_size = sizeof(float);
          break;
        case SQL_FLOAT:
        case SQL_DOUBLE:
          c_type = SQL_C_DOUBLE;
          column_size = sizeof(double);
          break;
        case SQL_TYPE_DATE:
          c_type = SQL_C_TYPE_DATE;
          column_size = sizeof(SQL_DATE_STRUCT);
          break;
        case SQL_TYPE_TIME:
          c_type = SQL_C_TYPE_TIME;
          column_size = sizeof(SQL_TIME_STRUCT);
          break;
        case SQL_DATETIME:
        case SQL_TYPE_TIMESTAMP:
          c_type = SQL_C_TYPE_TIMESTAMP;
          column_size = sizeof(SQL_TIMESTAMP_STRUCT);
          break;
        case SQL_INTERVAL_YEAR:
          c_type = SQL_C_INTERVAL_YEAR;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_MONTH:
          c_type = SQL_C_INTERVAL_MONTH;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_DAY:
          c_type = SQL_C_INTERVAL_DAY;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_HOUR:
          c_type = SQL_C_INTERVAL_HOUR;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_MINUTE:
          c_type = SQL_C_INTERVAL_MINUTE;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_SECOND:
          c_type = SQL_C_INTERVAL_SECOND;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_YEAR_TO_MONTH:
          c_type = SQL_C_INTERVAL_YEAR_TO_MONTH;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_DAY_TO_HOUR:
          c_type = SQL_C_INTERVAL_DAY_TO_HOUR;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_DAY_TO_MINUTE:
          c_type = SQL_C_INTERVAL_DAY_TO_MINUTE;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_DAY_TO_SECOND:
          c_type = SQL_C_INTERVAL_DAY_TO_SECOND;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_HOUR_TO_MINUTE:
          c_type = SQL_C_INTERVAL_HOUR_TO_MINUTE;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_HOUR_TO_SECOND:
          c_type = SQL_C_INTERVAL_HOUR_TO_SECOND;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_INTERVAL_MINUTE_TO_SECOND:
          c_type = SQL_C_INTERVAL_MINUTE_TO_SECOND;
          column_size = sizeof(SQL_INTERVAL_STRUCT);
          break;
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
          c_type = SQL_C_BINARY;
          if (dataLength < 0) {
            logError(printf("setupResultColumn: DataLength negative: %ld\n",
                            dataLength););
            *err_info = FILE_ERROR;
          } else if ((SQLULEN) dataLength > MAX_MEMSIZETYPE) {
            logError(printf("setupResultColumn: DataLength too big: %ld\n",
                            dataLength););
            *err_info = MEMORY_ERROR;
          } else {
            column_size = (memSizeType) dataLength;
          } /* if */
          break;
        default:
          logError(printf("setupResultColumn: Column %hd has the unknown type %s.\n",
                          column_num, nameOfBufferType(resultData->buffer_type)););
          *err_info = RANGE_ERROR;
          break;
      } /* switch */
      if (*err_info == OKAY_NO_ERROR) {
        resultData->buffer = malloc(column_size);
        if (resultData->buffer == NULL) {
          logError(printf("setupResultColumn: malloc(" FMT_U_MEM ") failed\n",
                          column_size););
          *err_info = MEMORY_ERROR;
        } else {
          memset(resultData->buffer, 0, column_size);
        } /* if */
      } /* if */
      if (*err_info == OKAY_NO_ERROR) {
        /* printf("c_type: %s\n", nameOfCType(c_type)); */
        resultData->buffer_length = column_size;
        if (SQLBindCol(preparedStmt->ppStmt,
                       (SQLUSMALLINT) column_num,
                       c_type,
                       resultData->buffer,
                       (SQLLEN) resultData->buffer_length,
                       &resultData->length) != SQL_SUCCESS) {
          logError(printf("setupResultColumn: SQLBindCol:\n");
                   printError(SQL_HANDLE_STMT, preparedStmt->ppStmt);
                   printf("c_type: %d\n", c_type);
                   printf("c_type: %s\n", nameOfCType(c_type)););
          *err_info = FILE_ERROR;
        } /* if */
        if (c_type == SQL_C_NUMERIC) {
          printf("SQL_C_NUMERIC:\n");
          printf("precision: %ld\n", precision);
          printf("scale: %ld\n", scale);
          /* Set the datatype, precision and scale. */
          /* Otherwise the driver defined default precision is used. */
          if (SQLGetStmtAttr(preparedStmt->ppStmt,
                             SQL_ATTR_APP_ROW_DESC,
                             &descriptorHandle,
                             0, NULL) != SQL_SUCCESS) {
            logError(printf("setupResultColumn: SQLGetStmtAttr:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
            *err_info = FILE_ERROR;
          } else if (SQLSetDescField(descriptorHandle,
                                     column_num,
                                     SQL_DESC_TYPE,
                                     (void *) SQL_C_NUMERIC,
                                     0) != SQL_SUCCESS) {
            logError(printf("setupResultColumn: SQLSetDescField SQL_DESC_TYPE:\n");
                     printError(SQL_HANDLE_DESC, descriptorHandle););
            *err_info = FILE_ERROR;
          } else if (SQLSetDescField(descriptorHandle,
                                     column_num,
                                     SQL_DESC_PRECISION,
                                     (void *) precision,
                                     0) != SQL_SUCCESS) {
            logError(printf("setupResultColumn: SQLSetDescField SQL_DESC_PRECISION:\n");
                     printError(SQL_HANDLE_DESC, descriptorHandle););
            *err_info = FILE_ERROR;
          } else if (SQLSetDescField(descriptorHandle,
                                     column_num,
                                     SQL_DESC_SCALE,
                                     (void *) scale,
                                     0) != SQL_SUCCESS) {
            logError(printf("setupResultColumn: SQLSetDescField SQL_DESC_SCALE:\n");
                     printError(SQL_HANDLE_DESC, descriptorHandle););
            *err_info = FILE_ERROR;
          } else if (SQLSetDescField(descriptorHandle,
                                     column_num,
                                     SQL_DESC_DATA_PTR,
                                     resultData->buffer,
                                     0) != SQL_SUCCESS) {
            logError(printf("setupResultColumn: SQLSetDescField SQL_DESC_DATA_PTR:\n");
                     printError(SQL_HANDLE_DESC, descriptorHandle););
            *err_info = FILE_ERROR;
          } else if (SQLSetDescField(descriptorHandle,
                                     column_num,
                                     SQL_DESC_INDICATOR_PTR,
                                     &resultData->length,
                                     0) != SQL_SUCCESS) {
            logError(printf("setupResultColumn: SQLSetDescField SQL_DESC_INDICATOR_PTR:\n");
                     printError(SQL_HANDLE_DESC, descriptorHandle););
            *err_info = FILE_ERROR;
          } else if (SQLSetDescField(descriptorHandle,
                                     column_num,
                                     SQL_DESC_OCTET_LENGTH_PTR,
                                     &resultData->length,
                                     0)!= SQL_SUCCESS) {
            logError(printf("setupResultColumn: SQLSetDescField SQL_DESC_OCTET_LENGTH_PTR:\n");
                     printError(SQL_HANDLE_DESC, descriptorHandle););
            *err_info = FILE_ERROR;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* setupResultColumn */



static void setupResult (preparedStmtType preparedStmt,
    errInfoType *err_info)

  {
    SQLSMALLINT num_columns;
    SQLSMALLINT column_index;

  /* setupResult */
    /* printf("begin setupResult\n"); */
    if (SQLNumResultCols(preparedStmt->ppStmt,
                         &num_columns) != SQL_SUCCESS) {
      logError(printf("setupResult: SQLNumResultCols:\n");
               printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
      *err_info = FILE_ERROR;
    } else if (num_columns < 0) {
      logError(printf("setupResult: SQLNumResultCols returns negative number: %hd\n",
                      num_columns););
      *err_info = FILE_ERROR;
    } else if (!ALLOC_TABLE(preparedStmt->result_array, resultDataRecord, (memSizeType) num_columns)) {
      *err_info = MEMORY_ERROR;
    } else {
      preparedStmt->result_array_size = (memSizeType) num_columns;
      memset(preparedStmt->result_array, 0, (memSizeType) num_columns * sizeof(resultDataRecord));
      for (column_index = 0; column_index < num_columns &&
           *err_info == OKAY_NO_ERROR; column_index++) {
        setupResultColumn(preparedStmt, (SQLSMALLINT) (column_index + 1),
                          &preparedStmt->result_array[column_index],
                          err_info);
      } /* for */
    } /* if */
    /* printf("end setupResult\n"); */
  } /* setupResult */



static void resizeBindArray (preparedStmtType preparedStmt, memSizeType pos)

  {
    memSizeType new_size;
    bindDataType resized_param_array;

  /* resizeBindArray */
    if (pos > preparedStmt->param_array_capacity) {
      new_size = (((pos - 1) >> 3) + 1) << 3;  /* Round to next multiple of 8. */
      resized_param_array = REALLOC_TABLE(preparedStmt->param_array, bindDataRecord, preparedStmt->param_array_capacity, new_size);
      if (resized_param_array == NULL) {
        FREE_TABLE(preparedStmt->param_array, bindDataRecord, preparedStmt->param_array_capacity);
        preparedStmt->param_array = NULL;
        preparedStmt->param_array_capacity = 0;
        raise_error(MEMORY_ERROR);
      } else {
        preparedStmt->param_array = resized_param_array;
        COUNT3_TABLE(bindDataRecord, preparedStmt->param_array_capacity, new_size);
        memset(&preparedStmt->param_array[preparedStmt->param_array_capacity], 0,
               (new_size - preparedStmt->param_array_capacity) * sizeof(bindDataRecord));
        preparedStmt->param_array_capacity = new_size;
      } /* if */
    } /* if */
    if (pos > preparedStmt->param_array_size) {
      preparedStmt->param_array_size = pos;
    } /* if */
  } /* resizeBindArray */



#ifdef DECODE_NUMERIC_STRUCT
static intType getNumericInt (const void *buffer)

  {
    SQL_NUMERIC_STRUCT *numStr;
    bigIntType bigIntValue;
    bigIntType powerOfTen;
    intType intValue = 0;

  /* getNumericInt */
    printf("getNumericInt\n");
    numStr = (SQL_NUMERIC_STRUCT *) buffer;
    if (numStr->scale > 0) {
      raise_error(RANGE_ERROR);
      intValue = 0;
    } else {
      printf("numStr->precision: %u\n", numStr->precision);
      printf("numStr->scale: %d\n", numStr->scale);
      printf("numStr->sign: %u\n", numStr->sign);
      printf("numStr->val:");
      { int idx;
        for (idx=0; idx < SQL_MAX_NUMERIC_LEN; idx++) {
          printf(" %u", ((unsigned char *) numStr->val)[idx]);
        }
        printf("\n");
      }
      bigIntValue = bigFromByteBufferLe(SQL_MAX_NUMERIC_LEN, numStr->val, FALSE);
      if (bigIntValue != NULL) {
        printf("numStr->val: ");
        prot_stri_unquoted(bigStr(bigIntValue));
        printf("\n");
      }
      if (bigIntValue != NULL && numStr->scale < 0) {
        powerOfTen = bigIPowSignedDigit(10, (intType) -numStr->scale);
        if (powerOfTen != NULL) {
          bigMultAssign(&bigIntValue, powerOfTen);
          bigDestr(powerOfTen);
        } /* if */
      } /* if */
      if (bigIntValue != NULL && numStr->sign != 1) {
        bigIntValue = bigNegateTemp(bigIntValue);
      } /* if */
      if (bigIntValue != NULL) {
        printf("numStr->val: ");
        prot_stri_unquoted(bigStr(bigIntValue));
        printf("\n");
#if INTTYPE_SIZE == 32
        intValue = bigToInt32(bigIntValue);
#elif INTTYPE_SIZE == 64
        intValue = bigToInt64(bigIntValue);
#endif
        bigDestr(bigIntValue);
      } /* if */
    } /* if */
    printf("getNumericInt --> " FMT_D "\n", intValue);
    return intValue;
  } /* getNumericInt */



static bigIntType getNumericBigInt (const void *buffer)

  {
    SQL_NUMERIC_STRUCT *numStr;
    bigIntType powerOfTen;
    bigIntType bigIntValue;

  /* getNumericBigInt */
    printf("getNumericBigInt\n");
    numStr = (SQL_NUMERIC_STRUCT *) buffer;
    if (numStr->scale > 0) {
      raise_error(RANGE_ERROR);
      bigIntValue = NULL;
    } else {
      bigIntValue = bigFromByteBufferLe(SQL_MAX_NUMERIC_LEN, numStr->val, FALSE);
      if (bigIntValue != NULL) {
        printf("numStr->precision: %u\n", numStr->precision);
        printf("numStr->scale: %d\n", numStr->scale);
        printf("numStr->sign: %u\n", numStr->sign);
        printf("numStr->val: ");
        prot_stri_unquoted(bigStr(bigIntValue));
        printf("\n");
      } /* if */
      if (bigIntValue != NULL && numStr->scale < 0) {
        powerOfTen = bigIPowSignedDigit(10, (intType) -numStr->scale);
        if (powerOfTen != NULL) {
          bigMultAssign(&bigIntValue, powerOfTen);
          bigDestr(powerOfTen);
        } /* if */
      } /* if */
      if (bigIntValue != NULL && numStr->sign != 1) {
        bigIntValue = bigNegateTemp(bigIntValue);
      } /* if */
    } /* if */
    printf("getNumericBigInt --> ");
      if (bigIntValue == NULL) {
        printf("NULL");
      } else {
        prot_stri_unquoted(bigStr(bigIntValue));
      }
      printf("\n");
    return bigIntValue;
 } /* getNumericBigInt */



static bigIntType getNumericBigRational (const void *buffer, bigIntType *denominator)

  {
    SQL_NUMERIC_STRUCT *numStr;
    bigIntType powerOfTen;
    bigIntType numerator;

  /* getNumericBigRational */
    printf("getNumericBigRational\n");
    numStr = (SQL_NUMERIC_STRUCT *) buffer;
    numerator = bigFromByteBufferLe(SQL_MAX_NUMERIC_LEN, numStr->val, FALSE);
    if (numerator != NULL) {
      printf("numStr->precision: %u\n", numStr->precision);
      printf("numStr->scale: %d\n", numStr->scale);
      printf("numStr->sign: %u\n", numStr->sign);
      printf("numStr->val: ");
      prot_stri_unquoted(bigStr(numerator));
      printf("\n");
    } /* if */
    if (numerator != NULL && numStr->sign != 1) {
      numerator = bigNegateTemp(numerator);
    } /* if */
    if (numerator != NULL) {
      if (numStr->scale < 0) {
        powerOfTen = bigIPowSignedDigit(10, (intType) -numStr->scale);
        if (powerOfTen != NULL) {
          bigMultAssign(&numerator, powerOfTen);
          bigDestr(powerOfTen);
        } /* if */
        *denominator = bigFromInt32(1);
      } else {
        *denominator = bigIPowSignedDigit(10, (intType) numStr->scale);
      } /* if */
    } /* if */
    return numerator;
  } /* getNumericBigRational */



static floatType getNumericFloat (const void *buffer)

  {
    bigIntType numerator;
    bigIntType denominator = NULL;
    floatType floatValue;

  /* getNumericFloat */
    /* printf("getNumericFloat\n"); */
    numerator = getNumericBigRational(buffer, &denominator);
    if (numerator == NULL || denominator == NULL) {
      floatValue = 0.0;
    } else {
      floatValue = bigRatToDouble(numerator, denominator);
    } /* if */
    bigDestr(numerator);
    bigDestr(denominator);
    return floatValue;
  } /* getNumericFloat */
#endif



static intType getInt (const void *buffer, memSizeType length)

  { /* getInt */
#ifdef DECODE_NUMERIC_STRUCT
    return getNumericInt(buffer);
#else
    return getDecimalInt(buffer, length);
#endif
  } /* getInt */



static bigIntType getBigInt (const void *buffer, memSizeType length)

  { /* getBigInt */
#ifdef DECODE_NUMERIC_STRUCT
    return getNumericBigInt(buffer);
#else
    return getDecimalBigInt(buffer, length);
#endif
  } /* getBigInt */



static bigIntType getBigRational (const void *buffer, memSizeType length,
    bigIntType *denominator)

  { /* getBigRational */
#ifdef DECODE_NUMERIC_STRUCT
    return getNumericBigRational(buffer, denominator);
#else
    return getDecimalBigRational(buffer, length, denominator);
#endif
  } /* getBigRational */



static floatType getFloat (const void *buffer, memSizeType length)

  { /* getFloat */
#ifdef DECODE_NUMERIC_STRUCT
    return getNumericFloat(buffer);
#else
    return getDecimalFloat(buffer, length);
#endif
  } /* getFloat */



#ifdef ENCODE_NUMERIC_STRUCT
static memSizeType setNumericBigInt (void **buffer, memSizeType *buffer_capacity,
    const const_bigIntType bigIntValue, errInfoType *err_info)

  {
    bigIntType absoluteValue;
    boolType negative;
    bstriType bstri;
    SQL_NUMERIC_STRUCT *numStr;

  /* setNumericBigInt */
    /* printf("setNumericBigInt(");
       prot_stri_unquoted(bigStr(bigIntValue));
       printf(")\n"); */
    if (*buffer == NULL) {
      if ((*buffer = malloc(sizeof(SQL_NUMERIC_STRUCT))) == NULL) {
        *err_info = MEMORY_ERROR;
      } else {
        *buffer_capacity = sizeof(SQL_NUMERIC_STRUCT);
      } /* if */
    } else if (*buffer_capacity != sizeof(SQL_NUMERIC_STRUCT)) {
      *err_info = MEMORY_ERROR;
    } /* if */
    if (*err_info == OKAY_NO_ERROR) {
      negative = bigCmpSignedDigit(bigIntValue, 0) < 0;
      if (negative) {
        absoluteValue = bigAbs(bigIntValue);
        if (absoluteValue == NULL) {
          bstri = NULL;
        } else {
          bstri = bigToBStriLe(absoluteValue, FALSE);
          bigDestr(absoluteValue);
        } /* if */
      } else {
        bstri = bigToBStriLe(bigIntValue, FALSE);
      } /* if */
      if (bstri == NULL) {
        *err_info = MEMORY_ERROR;
      } else {
        if (bstri->size > SQL_MAX_NUMERIC_LEN) {
          printf("bstri->size: " FMT_U_MEM "\n", bstri->size);
          printf("SQL_MAX_NUMERIC_LEN: %u\n", SQL_MAX_NUMERIC_LEN);
          *err_info = RANGE_ERROR;
        } else {
          numStr = (SQL_NUMERIC_STRUCT *) *buffer;
          numStr->precision = MAX_NUMERIC_PRECISION;
          numStr->scale = 0;
          numStr->sign = negative ? 0 : 1;
          memcpy(numStr->val, bstri->mem, bstri->size);
          memset(&numStr->val[bstri->size], 0, SQL_MAX_NUMERIC_LEN - bstri->size);
        } /* if */
        bstDestr(bstri);
      } /* if */
    } /* if */
    return sizeof(SQL_NUMERIC_STRUCT);
  } /* setNumericBigInt */



static memSizeType setNumericBigRat (void **buffer,
    const const_bigIntType numerator, const const_bigIntType denominator,
    errInfoType *err_info)

  {
    bigIntType number;
    bigIntType mantissaValue = NULL;
    bigIntType absoluteValue;
    boolType negative;
    bstriType bstri;
    SQL_NUMERIC_STRUCT *numStr;

  /* setNumericBigRat */
    /* printf("setNumericBigRat(");
       prot_stri_unquoted(bigStr(numerator));
       printf(", ");
       prot_stri_unquoted(bigStr(denominator));
       printf(")\n"); */
    if (*buffer == NULL) {
      if ((*buffer = malloc(sizeof(SQL_NUMERIC_STRUCT))) == NULL) {
        *err_info = MEMORY_ERROR;
      } /* if */
    } /* if */
    if (*err_info == OKAY_NO_ERROR) {
      if (bigEqSignedDigit(denominator, 0)) {
        /* Numeric values do not support Infinity and NaN. */
        logError(printf("setNumericBigRat: Decimal values do not support Infinity and NaN.\n"););
        raise_error(RANGE_ERROR);
      } else {
        number = bigIPowSignedDigit(10, MAX_NUMERIC_SCALE);
        if (number != NULL) {
          bigMultAssign(&number, numerator);
          mantissaValue = bigDiv(number, denominator);
          bigDestr(number);
        } /* if */
        if (mantissaValue != NULL) {
          negative = bigCmpSignedDigit(mantissaValue, 0) < 0;
          if (negative) {
            absoluteValue = bigAbs(mantissaValue);
            if (absoluteValue == NULL) {
              bstri = NULL;
            } else {
              bstri = bigToBStriLe(absoluteValue, FALSE);
              bigDestr(absoluteValue);
            } /* if */
          } else {
            bstri = bigToBStriLe(mantissaValue, FALSE);
          } /* if */
          if (bstri == NULL) {
            *err_info = MEMORY_ERROR;
          } else {
            if (bstri->size > SQL_MAX_NUMERIC_LEN) {
              logError(printf("setNumericBigRat: Data does not fit into a numeric.\n"););
              *err_info = RANGE_ERROR;
            } else {
              numStr = (SQL_NUMERIC_STRUCT *) *buffer;
              numStr->precision = MAX_NUMERIC_PRECISION;
              numStr->scale = MAX_NUMERIC_SCALE;
              numStr->sign = negative ? 0 : 1;
              memcpy(numStr->val, bstri->mem, bstri->size);
              memset(&numStr->val[bstri->size], 0, SQL_MAX_NUMERIC_LEN - bstri->size);
              /* {
                bigIntType numerator2;
                bigIntType denominator2;

                numerator2 = getNumericBigRational(*buffer, &denominator2);
                printf("stored: ");
                prot_stri_unquoted(bigStr(numerator2));
                printf(" / ");
                prot_stri_unquoted(bigStr(denominator2));
                printf("\n");
              } */
            } /* if */
            bstDestr(bstri);
          } /* if */
          bigDestr(mantissaValue);
        } /* if */
      } /* if */
    } /* if */
    return sizeof(SQL_NUMERIC_STRUCT);
  } /* setNumericBigRat */
#endif



static memSizeType setDecimalBigInt (void **buffer, memSizeType *buffer_capacity,
    const const_bigIntType bigIntValue, errInfoType *err_info)

  {
    striType stri;
    unsigned char *decimal;
    memSizeType srcIndex;
    memSizeType destIndex = 0;

  /* setDecimalBigInt */
    /* printf("setDecimalBigInt(");
       prot_stri_unquoted(bigStr(bigIntValue));
       printf(")\n"); */
    stri = bigStr(bigIntValue);
    if (stri == NULL) {
      *err_info = MEMORY_ERROR;
    } else {
      /* prot_stri(stri);
         printf("\n"); */
      if (*buffer == NULL) {
        if ((*buffer = malloc(stri->size + 1)) != NULL) {
          *buffer_capacity = stri->size + 1;
        } /* if */
      } else if (*buffer_capacity < stri->size + 1) {
        free(*buffer);
        if ((*buffer = malloc(stri->size + 1)) != NULL) {
          *buffer_capacity = stri->size + 1;
        } /* if */
      } /* if */
      if (*buffer == NULL) {
        *err_info = MEMORY_ERROR;
      } else {
        decimal = (unsigned char *) *buffer;
        for (srcIndex = 0; srcIndex < stri->size; srcIndex++) {
          decimal[destIndex] = (unsigned char) stri->mem[srcIndex];
          destIndex++;
        } /* for */
        decimal[destIndex] = '\0';
        /* printf("%s\n", decimal); */
      } /* if */
      FREE_STRI(stri, stri->size);
    } /* if */
    return destIndex;
  } /* setDecimalBigInt */



static memSizeType setDecimalBigRat (void **buffer,
    const const_bigIntType numerator, const const_bigIntType denominator,
    errInfoType *err_info)

  {
    memSizeType length;

  /* setDecimalBigRat */
    if (*buffer != NULL) {
      free(*buffer);
    } /* if */
    *buffer = bigRatToDecimal(numerator, denominator, DEFAULT_DECIMAL_SCALE,
                              &length, err_info);
    return length;
  } /* setDecimalBigRat */



static memSizeType setBigInt (void **buffer, memSizeType *buffer_capacity,
    const const_bigIntType bigIntValue, errInfoType *err_info)

  { /* setBigInt */
#ifdef ENCODE_NUMERIC_STRUCT
    return setNumericBigInt(buffer, buffer_capacity, bigIntValue, err_info);
#else
    return setDecimalBigInt(buffer, buffer_capacity, bigIntValue, err_info);
#endif
  } /* setBigInt */



static memSizeType setBigRat (void **buffer,
    const const_bigIntType numerator, const const_bigIntType denominator,
    errInfoType *err_info)

  { /* setBigRat */
#ifdef ENCODE_NUMERIC_STRUCT
    return setNumericBigRat(buffer, numerator, denominator, err_info);
#else
    return setDecimalBigRat(buffer, numerator, denominator, err_info);
#endif
  } /* setBigRat */



static void sqlBindBigInt (sqlStmtType sqlStatement, intType pos,
    const const_bigIntType value)

  {
    preparedStmtType preparedStmt;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindBigInt */
    /* printf("sqlBindBigInt(%lx, " FMT_D ", %s)\n",
       (unsigned long) sqlStatement, pos, bigHexCStri(value)); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || (uintType) pos > USHRT_MAX)) {
      logError(printf("sqlBindBigInt: pos: " FMT_D ", max pos: %u.\n",
                      pos, USHRT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      resizeBindArray(preparedStmt, (memSizeType) pos);
      if (preparedStmt->param_array != NULL) {
        if (preparedStmt->param_array[pos - 1].buffer == NULL) {
          preparedStmt->param_array[pos - 1].buffer_length =
              setBigInt(&preparedStmt->param_array[pos - 1].buffer,
                        &preparedStmt->param_array[pos - 1].buffer_capacity,
                        value, &err_info);
#ifdef ENCODE_NUMERIC_STRUCT
          preparedStmt->param_array[pos - 1].buffer_type = SQL_NUMERIC;
        } else if (preparedStmt->param_array[pos - 1].buffer_type != SQL_NUMERIC) {
#else
          preparedStmt->param_array[pos - 1].buffer_type = SQL_DECIMAL;
        } else if (preparedStmt->param_array[pos - 1].buffer_type != SQL_DECIMAL) {
#endif
          err_info = RANGE_ERROR;
        } else {
          preparedStmt->param_array[pos - 1].buffer_length =
              setBigInt(&preparedStmt->param_array[pos - 1].buffer,
                        &preparedStmt->param_array[pos - 1].buffer_capacity,
                        value, &err_info);
        } /* if */
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
        } else {
          if (SQLBindParameter(preparedStmt->ppStmt,
                               (SQLUSMALLINT) pos,
                               SQL_PARAM_INPUT,
#ifdef ENCODE_NUMERIC_STRUCT
                               SQL_C_NUMERIC,
#else
                               SQL_C_CHAR,
#endif
                               (SQLSMALLINT) preparedStmt->param_array[pos - 1].buffer_type,
#ifdef ENCODE_NUMERIC_STRUCT
                               MAX_NUMERIC_PRECISION,
#else
                               preparedStmt->param_array[pos - 1].buffer_length,
#endif
                               0,
                               preparedStmt->param_array[pos - 1].buffer,
                               (SQLLEN) preparedStmt->param_array[pos - 1].buffer_length,
                               NULL) != SQL_SUCCESS) {
            logError(printf("sqlBindBigInt: SQLBindParameter:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->executeSuccessful = FALSE;
            preparedStmt->fetchOkay = FALSE;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindBigInt */



static void sqlBindBigRat (sqlStmtType sqlStatement, intType pos,
    const const_bigIntType numerator, const const_bigIntType denominator)

  {
    preparedStmtType preparedStmt;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindBigRat */
    /* printf("sqlBindBigRat(%lx, " FMT_D ", %s/%s)\n",
       (unsigned long) sqlStatement, pos,
       bigHexCStri(numerator), bigHexCStri(denominator)); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || (uintType) pos > USHRT_MAX)) {
      logError(printf("sqlBindBigRat: pos: " FMT_D ", max pos: %u.\n",
                      pos, USHRT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      resizeBindArray(preparedStmt, (memSizeType) pos);
      if (preparedStmt->param_array != NULL) {
        if (preparedStmt->param_array[pos - 1].buffer == NULL) {
          preparedStmt->param_array[pos - 1].buffer_length =
              setBigRat(&preparedStmt->param_array[pos - 1].buffer,
                        numerator, denominator, &err_info);
#ifdef ENCODE_NUMERIC_STRUCT
          preparedStmt->param_array[pos - 1].buffer_type = SQL_NUMERIC;
        } else if (preparedStmt->param_array[pos - 1].buffer_type != SQL_NUMERIC) {
#else
          preparedStmt->param_array[pos - 1].buffer_type = SQL_DECIMAL;
        } else if (preparedStmt->param_array[pos - 1].buffer_type != SQL_DECIMAL) {
#endif
          err_info = RANGE_ERROR;
        } else {
          preparedStmt->param_array[pos - 1].buffer_length =
              setBigRat(&preparedStmt->param_array[pos - 1].buffer,
                        numerator, denominator, &err_info);
        } /* if */
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
        } else {
          if (SQLBindParameter(preparedStmt->ppStmt,
                               (SQLUSMALLINT) pos,
                               SQL_PARAM_INPUT,
#ifdef ENCODE_NUMERIC_STRUCT
                               SQL_C_NUMERIC,
#else
                               SQL_C_CHAR,
#endif
                               (SQLSMALLINT) preparedStmt->param_array[pos - 1].buffer_type,
#ifdef ENCODE_NUMERIC_STRUCT
                               MAX_NUMERIC_PRECISION,
#else
                               preparedStmt->param_array[pos - 1].buffer_length,
#endif
                               0,
                               preparedStmt->param_array[pos - 1].buffer,
                               (SQLLEN) preparedStmt->param_array[pos - 1].buffer_length,
                               NULL) != SQL_SUCCESS) {
            logError(printf("sqlBindBigRat: SQLBindParameter:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->executeSuccessful = FALSE;
            preparedStmt->fetchOkay = FALSE;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindBigRat */



static void sqlBindBool (sqlStmtType sqlStatement, intType pos, boolType value)

  {
    preparedStmtType preparedStmt;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindBool */
    /* printf("sqlBindBool(%lx, " FMT_D ", %ld)\n",
       (unsigned long) sqlStatement, pos, value); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || (uintType) pos > USHRT_MAX)) {
      logError(printf("sqlBindBool: pos: " FMT_D ", max pos: %u.\n",
                      pos, USHRT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      resizeBindArray(preparedStmt, (memSizeType) pos);
      if (preparedStmt->param_array != NULL) {
        if (preparedStmt->param_array[pos - 1].buffer == NULL) {
          if ((preparedStmt->param_array[pos - 1].buffer = malloc(1)) == NULL) {
            err_info = MEMORY_ERROR;
          } else {
            preparedStmt->param_array[pos - 1].buffer_type = SQL_VARCHAR;
            preparedStmt->param_array[pos - 1].buffer_length = 1;
          } /* if */
        } else if (preparedStmt->param_array[pos - 1].buffer_type != SQL_VARCHAR ||
                   preparedStmt->param_array[pos - 1].buffer_length != 1) {
          err_info = RANGE_ERROR;
        } /* if */
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
        } else {
          ((char *) preparedStmt->param_array[pos - 1].buffer)[0] = (char) ('0' + value);
          preparedStmt->param_array[pos - 1].length = 1;
          if (SQLBindParameter(preparedStmt->ppStmt,
                               (SQLUSMALLINT) pos,
                               SQL_PARAM_INPUT,
                               SQL_C_CHAR,
                               SQL_VARCHAR,
                               1,
                               0,
                               preparedStmt->param_array[pos - 1].buffer,
                               1,
                               &preparedStmt->param_array[pos - 1].length) != SQL_SUCCESS) {
            logError(printf("sqlBindBool: SQLBindParameter:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->executeSuccessful = FALSE;
            preparedStmt->fetchOkay = FALSE;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindBool */



static void sqlBindBStri (sqlStmtType sqlStatement, intType pos, bstriType bstri)

  {
    preparedStmtType preparedStmt;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindBStri */
    /* printf("sqlBindBStri(%lx, " FMT_D ", ",
       (unsigned long) sqlStatement, pos);
       prot_bstri(bstri);
       printf(")\n"); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || (uintType) pos > USHRT_MAX)) {
      logError(printf("sqlBindBStri: pos: " FMT_D ", max pos: %u.\n",
                      pos, USHRT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      resizeBindArray(preparedStmt, (memSizeType) pos);
      if (preparedStmt->param_array != NULL) {
        if (preparedStmt->param_array[pos - 1].buffer == NULL) {
          if ((preparedStmt->param_array[pos - 1].buffer = malloc(bstri->size)) == NULL) {
            err_info = MEMORY_ERROR;
          } else {
            preparedStmt->param_array[pos - 1].buffer_type = SQL_VARBINARY;
            preparedStmt->param_array[pos - 1].buffer_capacity = bstri->size;
          } /* if */
        } else if (preparedStmt->param_array[pos - 1].buffer_type != SQL_VARBINARY) {
          err_info = RANGE_ERROR;
        } else if (bstri->size > preparedStmt->param_array[pos - 1].buffer_capacity) {
          free(preparedStmt->param_array[pos - 1].buffer);
          if ((preparedStmt->param_array[pos - 1].buffer = malloc(bstri->size)) == NULL) {
            err_info = MEMORY_ERROR;
          } else {
            preparedStmt->param_array[pos - 1].buffer_capacity = bstri->size;
          } /* if */
        } /* if */
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
        } else if (bstri->size > SQLLEN_MAX) {
          free(preparedStmt->param_array[pos - 1].buffer);
          preparedStmt->param_array[pos - 1].buffer = NULL;
          raise_error(MEMORY_ERROR);
        } else {
          memcpy(preparedStmt->param_array[pos - 1].buffer, bstri->mem, bstri->size);
          preparedStmt->param_array[pos - 1].buffer_length = bstri->size;
          preparedStmt->param_array[pos - 1].length = (SQLLEN) bstri->size;
          if (SQLBindParameter(preparedStmt->ppStmt,
                               (SQLUSMALLINT) pos,
                               SQL_PARAM_INPUT,
                               SQL_C_BINARY,
                               SQL_VARBINARY,
                               bstri->size == 0 ? 1 : bstri->size,
                               0,
                               preparedStmt->param_array[pos - 1].buffer,
                               (SQLLEN) bstri->size,
                               &preparedStmt->param_array[pos - 1].length) != SQL_SUCCESS) {
            logError(printf("sqlBindBStri: SQLBindParameter:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->executeSuccessful = FALSE;
            preparedStmt->fetchOkay = FALSE;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindBStri */



static void sqlBindDuration (sqlStmtType sqlStatement, intType pos,
    intType year, intType month, intType day, intType hour,
    intType minute, intType second, intType micro_second)

  {
    preparedStmtType preparedStmt;
    SQLSMALLINT c_type = 0;
    SQLSMALLINT sql_type = 0;
    SQL_INTERVAL_STRUCT * interval;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindDuration */
    /* printf("sqlBindDuration(%lx, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld)\n",
       sqlStatement, pos, year, month, day, hour, minute, second, micro_second); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || (uintType) pos > USHRT_MAX)) {
      logError(printf("sqlBindDuration: pos: " FMT_D ", max pos: %u.\n",
                      pos, USHRT_MAX););
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
    } else {
      resizeBindArray(preparedStmt, (memSizeType) pos);
      if (preparedStmt->param_array != NULL) {
        if (preparedStmt->param_array[pos - 1].buffer == NULL) {
          if ((preparedStmt->param_array[pos - 1].buffer = malloc(sizeof(SQL_INTERVAL_STRUCT))) == NULL) {
            err_info = MEMORY_ERROR;
          } else {
            /* Use SQL_C_INTERVAL_SECOND internally for all interval types. */
            preparedStmt->param_array[pos - 1].buffer_type = SQL_INTERVAL_SECOND;
            preparedStmt->param_array[pos - 1].buffer_length = sizeof(SQL_INTERVAL_STRUCT);
          } /* if */
        } else if (preparedStmt->param_array[pos - 1].buffer_type != SQL_INTERVAL_SECOND ||
                   preparedStmt->param_array[pos - 1].buffer_length != sizeof(SQL_INTERVAL_STRUCT)) {
          err_info = RANGE_ERROR;
        } /* if */
        if (err_info == OKAY_NO_ERROR) {
          interval = (SQL_INTERVAL_STRUCT *) preparedStmt->param_array[pos - 1].buffer;
          memset(interval, 0, sizeof(SQL_INTERVAL_STRUCT));
          if (day == 0 && hour == 0 && minute == 0 && second == 0 && micro_second == 0) {
            if (year != 0) {
              if (month != 0) {
                c_type = SQL_C_INTERVAL_YEAR_TO_MONTH;
                sql_type = SQL_INTERVAL_YEAR_TO_MONTH;
                interval->interval_type = SQL_IS_YEAR_TO_MONTH;
                interval->interval_sign = year < 0 ? SQL_TRUE : SQL_FALSE;
                interval->intval.year_month.year  = (SQLUINTEGER) abs((int) year);
                interval->intval.year_month.month = (SQLUINTEGER) abs((int) month);
              } else {
                c_type = SQL_C_INTERVAL_YEAR;
                sql_type = SQL_INTERVAL_YEAR;
                interval->interval_type = SQL_IS_YEAR;
                interval->interval_sign = year < 0 ? SQL_TRUE : SQL_FALSE;
                interval->intval.year_month.year = (SQLUINTEGER) abs((int) year);
              } /* if */
            } else if (month != 0) {
              c_type = SQL_C_INTERVAL_MONTH;
              sql_type = SQL_INTERVAL_MONTH;
              interval->interval_type = SQL_IS_MONTH;
              interval->interval_sign = month < 0 ? SQL_TRUE : SQL_FALSE;
              interval->intval.year_month.month = (SQLUINTEGER) abs((int) month);
            } else {
              c_type = SQL_C_INTERVAL_SECOND;
              sql_type = SQL_INTERVAL_SECOND;
              interval->interval_type = SQL_IS_SECOND;
              interval->interval_sign = SQL_FALSE;
              interval->intval.day_second.second = 0;
            } /* if */
          } else if (year == 0 && month == 0) {
            if (day != 0) {
              if (second != 0) {
                c_type = SQL_C_INTERVAL_DAY_TO_SECOND;
                sql_type = SQL_INTERVAL_DAY_TO_SECOND;
                interval->interval_type = SQL_IS_DAY_TO_SECOND;
                interval->interval_sign = day < 0 ? SQL_TRUE : SQL_FALSE;
                interval->intval.day_second.day    = (SQLUINTEGER) abs((int) day);
                interval->intval.day_second.hour   = (SQLUINTEGER) abs((int) hour);
                interval->intval.day_second.minute = (SQLUINTEGER) abs((int) minute);
                interval->intval.day_second.second = (SQLUINTEGER) abs((int) second);
              } else if (minute != 0) {
                c_type = SQL_C_INTERVAL_DAY_TO_MINUTE;
                sql_type = SQL_INTERVAL_DAY_TO_MINUTE;
                interval->interval_type = SQL_IS_DAY_TO_MINUTE;
                interval->interval_sign = day < 0 ? SQL_TRUE : SQL_FALSE;
                interval->intval.day_second.day    = (SQLUINTEGER) abs((int) day);
                interval->intval.day_second.hour   = (SQLUINTEGER) abs((int) hour);
                interval->intval.day_second.minute = (SQLUINTEGER) abs((int) minute);
              } else if (hour != 0) {
                c_type = SQL_C_INTERVAL_DAY_TO_HOUR;
                sql_type = SQL_INTERVAL_DAY_TO_HOUR;
                interval->interval_type = SQL_IS_DAY_TO_HOUR;
                interval->interval_sign = day < 0 ? SQL_TRUE : SQL_FALSE;
                interval->intval.day_second.day  = (SQLUINTEGER) abs((int) day);
                interval->intval.day_second.hour = (SQLUINTEGER) abs((int) hour);
              } else {
                c_type = SQL_C_INTERVAL_DAY;
                sql_type = SQL_INTERVAL_DAY;
                interval->interval_type = SQL_IS_DAY;
                interval->interval_sign = day < 0 ? SQL_TRUE : SQL_FALSE;
                interval->intval.day_second.day = (SQLUINTEGER) abs((int) day);
              } /* if */
            } else if (hour != 0) {
              if (second != 0) {
                c_type = SQL_C_INTERVAL_HOUR_TO_SECOND;
                sql_type = SQL_INTERVAL_HOUR_TO_SECOND;
                interval->interval_type = SQL_IS_HOUR_TO_SECOND;
                interval->interval_sign = hour < 0 ? SQL_TRUE : SQL_FALSE;
                interval->intval.day_second.hour   = (SQLUINTEGER) abs((int) hour);
                interval->intval.day_second.minute = (SQLUINTEGER) abs((int) minute);
                interval->intval.day_second.second = (SQLUINTEGER) abs((int) second);
              } else if (minute != 0) {
                c_type = SQL_C_INTERVAL_HOUR_TO_MINUTE;
                sql_type = SQL_INTERVAL_HOUR_TO_MINUTE;
                interval->interval_type = SQL_IS_HOUR_TO_MINUTE;
                interval->interval_sign = hour < 0 ? SQL_TRUE : SQL_FALSE;
                interval->intval.day_second.hour   = (SQLUINTEGER) abs((int) hour);
                interval->intval.day_second.minute = (SQLUINTEGER) abs((int) minute);
              } else {
                c_type = SQL_C_INTERVAL_HOUR;
                sql_type = SQL_INTERVAL_HOUR;
                interval->interval_type = SQL_IS_HOUR;
                interval->interval_sign = hour < 0 ? SQL_TRUE : SQL_FALSE;
                interval->intval.day_second.hour = (SQLUINTEGER) abs((int) hour);
              } /* if */
            } else if (minute != 0) {
              if (second != 0) {
                c_type = SQL_C_INTERVAL_MINUTE_TO_SECOND;
                sql_type = SQL_INTERVAL_MINUTE_TO_SECOND;
                interval->interval_type = SQL_IS_MINUTE_TO_SECOND;
                interval->interval_sign = minute < 0 ? SQL_TRUE : SQL_FALSE;
                interval->intval.day_second.minute = (SQLUINTEGER) abs((int) minute);
                interval->intval.day_second.second = (SQLUINTEGER) abs((int) second);
              } else {
                c_type = SQL_C_INTERVAL_MINUTE;
                sql_type = SQL_INTERVAL_MINUTE;
                interval->interval_type = SQL_IS_MINUTE;
                interval->interval_sign = minute < 0 ? SQL_TRUE : SQL_FALSE;
                interval->intval.day_second.minute = (SQLUINTEGER) abs((int) minute);
              } /* if */
            } else {
              c_type = SQL_C_INTERVAL_SECOND;
              sql_type = SQL_INTERVAL_SECOND;
              interval->interval_type = SQL_IS_SECOND;
              interval->interval_sign = second < 0 ? SQL_TRUE : SQL_FALSE;
              interval->intval.day_second.second = (SQLUINTEGER) abs((int) second);
            } /* if */
            interval->intval.day_second.fraction = (SQLUINTEGER) micro_second;
          } else {
            err_info = RANGE_ERROR;
          } /* if */
        } /* if */
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
        } else {
          if (SQLBindParameter(preparedStmt->ppStmt,
                               (SQLUSMALLINT) pos,
                               SQL_PARAM_INPUT,
                               c_type,
                               sql_type,
                               0,
                               0,
                               preparedStmt->param_array[pos - 1].buffer,
                               sizeof(SQL_INTERVAL_STRUCT),
                               NULL) != SQL_SUCCESS) {
            logError(printf("sqlBindDuration: SQLBindParameter:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->executeSuccessful = FALSE;
            preparedStmt->fetchOkay = FALSE;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindDuration */



static void sqlBindFloat (sqlStmtType sqlStatement, intType pos, floatType value)

  {
    preparedStmtType preparedStmt;
#if FLOATTYPE_SIZE == 32
    const SQLSMALLINT c_type = SQL_C_FLOAT;
#elif FLOATTYPE_SIZE == 64
    const SQLSMALLINT c_type = SQL_C_DOUBLE;
#endif
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindFloat */
    /* printf("sqlBindFloat(%lx, " FMT_D ", " FMT_E ")\n",
       (unsigned long) sqlStatement, pos, value); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || (uintType) pos > USHRT_MAX)) {
      logError(printf("sqlBindFloat: pos: " FMT_D ", max pos: %u.\n",
                      pos, USHRT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      resizeBindArray(preparedStmt, (memSizeType) pos);
      if (preparedStmt->param_array != NULL) {
        if (preparedStmt->param_array[pos - 1].buffer == NULL) {
          if ((preparedStmt->param_array[pos - 1].buffer = malloc(sizeof(floatType))) == NULL) {
            err_info = MEMORY_ERROR;
          } else {
            preparedStmt->param_array[pos - 1].buffer_type = SQL_DOUBLE;
            preparedStmt->param_array[pos - 1].buffer_length = sizeof(floatType);
          } /* if */
        } else if (preparedStmt->param_array[pos - 1].buffer_type != SQL_DOUBLE ||
                   preparedStmt->param_array[pos - 1].buffer_length != sizeof(floatType)) {
          err_info = RANGE_ERROR;
        } /* if */
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
        } else {
          *(floatType *) preparedStmt->param_array[pos - 1].buffer = value;
          if (SQLBindParameter(preparedStmt->ppStmt,
                               (SQLUSMALLINT) pos,
                               SQL_PARAM_INPUT,
                               c_type,
                               SQL_DOUBLE,
                               0,
                               0,
                               preparedStmt->param_array[pos - 1].buffer,
                               sizeof(floatType),
                               NULL) != SQL_SUCCESS) {
            logError(printf("sqlBindFloat: SQLBindParameter:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->executeSuccessful = FALSE;
            preparedStmt->fetchOkay = FALSE;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindFloat */



static void sqlBindInt (sqlStmtType sqlStatement, intType pos, intType value)

  {
    preparedStmtType preparedStmt;
#if INTTYPE_SIZE == 32
    const SQLSMALLINT c_type = SQL_C_SLONG;
#elif INTTYPE_SIZE == 64
    const SQLSMALLINT c_type = SQL_C_SBIGINT;
#endif
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindInt */
    /* printf("sqlBindInt(%lx, " FMT_D ", " FMT_D ")\n",
       (unsigned long) sqlStatement, pos, value); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || (uintType) pos > USHRT_MAX)) {
      logError(printf("sqlBindInt: pos: " FMT_D ", max pos: %u.\n",
                      pos, USHRT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      resizeBindArray(preparedStmt, (memSizeType) pos);
      if (preparedStmt->param_array != NULL) {
        if (preparedStmt->param_array[pos - 1].buffer == NULL) {
          if ((preparedStmt->param_array[pos - 1].buffer = malloc(sizeof(intType))) == NULL) {
            err_info = MEMORY_ERROR;
          } else {
            preparedStmt->param_array[pos - 1].buffer_type = SQL_BIGINT;
            preparedStmt->param_array[pos - 1].buffer_length = sizeof(intType);
          } /* if */
        } else if (preparedStmt->param_array[pos - 1].buffer_type != SQL_BIGINT ||
                   preparedStmt->param_array[pos - 1].buffer_length != sizeof(intType)) {
          err_info = RANGE_ERROR;
        } /* if */
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
        } else {
          *(intType *) preparedStmt->param_array[pos - 1].buffer = value;
          if (SQLBindParameter(preparedStmt->ppStmt,
                               (SQLUSMALLINT) pos,
                               SQL_PARAM_INPUT,
                               c_type,
                               SQL_BIGINT,
                               0,
                               0,
                               preparedStmt->param_array[pos - 1].buffer,
                               sizeof(intType),
                               NULL) != SQL_SUCCESS) {
            logError(printf("sqlBindInt: SQLBindParameter:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->executeSuccessful = FALSE;
            preparedStmt->fetchOkay = FALSE;
          } /* if */
        } /* if */
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
    if (unlikely(pos < 1 || (uintType) pos > USHRT_MAX)) {
      logError(printf("sqlBindNull: pos: " FMT_D ", max pos: %u.\n",
                      pos, USHRT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      resizeBindArray(preparedStmt, (memSizeType) pos);
      if (preparedStmt->param_array != NULL) {
        preparedStmt->param_array[pos - 1].length = SQL_NULL_DATA;
        if (SQLBindParameter(preparedStmt->ppStmt,
                             (SQLUSMALLINT) pos,
                             SQL_PARAM_INPUT,
                             SQL_C_CHAR,
                             SQL_VARCHAR,
                             1,
                             0,
                             NULL,
                             0,
                             &preparedStmt->param_array[pos - 1].length) != SQL_SUCCESS) {
          logError(printf("sqlBindNull: SQLBindParameter:\n");
                   printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
          raise_error(FILE_ERROR);
        } else {
          preparedStmt->executeSuccessful = FALSE;
          preparedStmt->fetchOkay = FALSE;
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindNull */



static void sqlBindStri (sqlStmtType sqlStatement, intType pos, striType stri)

  {
    preparedStmtType preparedStmt;
    wstriType wstri;
    memSizeType length;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindStri */
    /* printf("sqlBindStri(%lx, " FMT_D ", ",
       (unsigned long) sqlStatement, pos);
       prot_stri(stri);
       printf(")\n"); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || (uintType) pos > USHRT_MAX)) {
      logError(printf("sqlBindStri: pos: " FMT_D ", max pos: %u.\n",
                      pos, USHRT_MAX););
      raise_error(RANGE_ERROR);
    } else {
      resizeBindArray(preparedStmt, (memSizeType) pos);
      if (preparedStmt->param_array != NULL) {
        if (preparedStmt->param_array[pos - 1].buffer == NULL) {
          wstri = stri_to_wstri_buf(stri, &length, &err_info);
          if (wstri != NULL) {
            preparedStmt->param_array[pos - 1].buffer_type = SQL_WVARCHAR;
          } /* if */
        } else if (preparedStmt->param_array[pos - 1].buffer_type != SQL_WVARCHAR) {
          err_info = RANGE_ERROR;
        } else {
          free(preparedStmt->param_array[pos - 1].buffer);
          wstri = stri_to_wstri_buf(stri, &length, &err_info);
        } /* if */
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
        } else if (length > SQLLEN_MAX >> 1) {
          free(preparedStmt->param_array[pos - 1].buffer);
          raise_error(MEMORY_ERROR);
        } else {
          preparedStmt->param_array[pos - 1].buffer = wstri;
          preparedStmt->param_array[pos - 1].buffer_length = length << 1;
          preparedStmt->param_array[pos - 1].length = (SQLLEN) (length << 1);
          if (SQLBindParameter(preparedStmt->ppStmt,
                               (SQLUSMALLINT) pos,
                               SQL_PARAM_INPUT,
                               SQL_C_WCHAR,
                               SQL_WVARCHAR,
                               length == 0 ? 1 : length << 1,
                               0,
                               preparedStmt->param_array[pos - 1].buffer,
                               (SQLLEN) (length << 1),
                               &preparedStmt->param_array[pos - 1].length) != SQL_SUCCESS) {
            logError(printf("sqlBindStri: SQLBindParameter:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->executeSuccessful = FALSE;
            preparedStmt->fetchOkay = FALSE;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindStri */



static void sqlBindTime (sqlStmtType sqlStatement, intType pos,
    intType year, intType month, intType day, intType hour,
    intType minute, intType second, intType micro_second)

  {
    preparedStmtType preparedStmt;
    SQL_TIMESTAMP_STRUCT *timestampValue;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlBindTime */
    /* printf("sqlBindTime(%lx, " FMT_D ", %ld, %ld, %ld, %ld, %ld, %ld, %ld)\n",
       sqlStatement, pos, year, month, day, hour, minute, second, micro_second); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(pos < 1 || (uintType) pos > USHRT_MAX)) {
      logError(printf("sqlBindTime: pos: " FMT_D ", max pos: %u.\n",
                      pos, USHRT_MAX););
      raise_error(RANGE_ERROR);
    } else if (year < SHRT_MIN || year > SHRT_MAX || month < 1 || month > 12 ||
               day < 1 || day > 31 || hour < 0 || hour > 24 || minute < 0 || minute > 60 ||
               second < 0 || second > 60 || micro_second < 0 || micro_second > 1000000) {
      logError(printf("sqlBindTime: Time not in allowed range.\n"););
      raise_error(RANGE_ERROR);
    } else {
      resizeBindArray(preparedStmt, (memSizeType) pos);
      if (preparedStmt->param_array != NULL) {
        if (preparedStmt->param_array[pos - 1].buffer == NULL) {
          if ((preparedStmt->param_array[pos - 1].buffer = malloc(sizeof(SQL_TIMESTAMP_STRUCT))) == NULL) {
            err_info = MEMORY_ERROR;
          } else {
            preparedStmt->param_array[pos - 1].buffer_type = SQL_TYPE_TIMESTAMP;
            preparedStmt->param_array[pos - 1].buffer_length = sizeof(SQL_TIMESTAMP_STRUCT);
          } /* if */
        } else if (preparedStmt->param_array[pos - 1].buffer_type != SQL_TYPE_TIMESTAMP ||
                   preparedStmt->param_array[pos - 1].buffer_length != sizeof(SQL_TIMESTAMP_STRUCT)) {
          err_info = RANGE_ERROR;
        } /* if */
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
        } else {
          timestampValue = (SQL_TIMESTAMP_STRUCT *) preparedStmt->param_array[pos - 1].buffer;
          timestampValue->year     = (SQLSMALLINT)  year;
          timestampValue->month    = (SQLUSMALLINT) month;
          timestampValue->day      = (SQLUSMALLINT) day;
          timestampValue->hour     = (SQLUSMALLINT) hour;
          timestampValue->minute   = (SQLUSMALLINT) minute;
          timestampValue->second   = (SQLUSMALLINT) second;
          timestampValue->fraction = (SQLUINTEGER)  micro_second;
          if (SQLBindParameter(preparedStmt->ppStmt,
                               (SQLUSMALLINT) pos,
                               SQL_PARAM_INPUT,
                               SQL_C_TYPE_TIMESTAMP,
                               SQL_TYPE_TIMESTAMP,
                               0,
                               0,
                               preparedStmt->param_array[pos - 1].buffer,
                               sizeof(SQL_TIMESTAMP_STRUCT),
                               NULL) != SQL_SUCCESS) {
            logError(printf("sqlBindTime: SQLBindParameter:\n");
                     printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
            raise_error(FILE_ERROR);
          } else {
            preparedStmt->executeSuccessful = FALSE;
            preparedStmt->fetchOkay = FALSE;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
  } /* sqlBindTime */



static void sqlClose (databaseType database)

  {
    dbType db;

  /* sqlClose */
    db = (dbType) database;
    if (db->sql_connection != SQL_NULL_HANDLE) {
      SQLDisconnect(db->sql_connection);
      SQLFreeHandle(SQL_HANDLE_DBC, db->sql_connection);
      db->sql_connection = SQL_NULL_HANDLE;
    } /* if */
    if (db->sql_environment != SQL_NULL_HANDLE) {
      SQLFreeHandle(SQL_HANDLE_ENV, db->sql_environment);
      db->sql_environment = SQL_NULL_HANDLE;
    } /* if */
  } /* sqlClose */



static bigIntType sqlColumnBigInt (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    bigIntType columnValue;

  /* sqlColumnBigInt */
    /* printf("sqlColumnBigInt(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_array_size)) {
      logError(printf("sqlColumnBigInt: Fetch okay: %d, column: " FMT_D ", max column: %lu.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_array_size););
      raise_error(RANGE_ERROR);
      columnValue = NULL;
    } else {
      if (preparedStmt->result_array[column - 1].length == SQL_NULL_DATA) {
        /* printf("Column is NULL -> Use default value: 0\n"); */
        columnValue = bigZero();
      } else if (preparedStmt->result_array[column - 1].length < 0) {
        logError(printf("sqlColumnBigInt: Column " FMT_D " has length: %ld\n",
                        column, preparedStmt->result_array[column - 1].length););
        raise_error(FILE_ERROR);
        columnValue = NULL;
      } else {
        /* printf("length: %ld\n", preparedStmt->result_array[column - 1].length); */
        /* printf("buffer_type: %s\n",
           nameOfBufferType(preparedStmt->result_array[column - 1].buffer_type)); */
        switch (preparedStmt->result_array[column - 1].buffer_type) {
          case SQL_TINYINT:
            columnValue = bigFromInt32((int32Type)
                *(int8Type *) preparedStmt->result_array[column - 1].buffer);
            break;
          case SQL_SMALLINT:
            columnValue = bigFromInt32((int32Type)
                *(int16Type *) preparedStmt->result_array[column - 1].buffer);
            break;
          case SQL_INTEGER:
            columnValue = bigFromInt32(
                *(int32Type *) preparedStmt->result_array[column - 1].buffer);
            break;
          case SQL_BIGINT:
            columnValue = bigFromInt64(
                *(int64Type *) preparedStmt->result_array[column - 1].buffer);
            break;
          case SQL_DECIMAL:
            columnValue = getDecimalBigInt(
                preparedStmt->result_array[column - 1].buffer,
                (memSizeType) preparedStmt->result_array[column - 1].length);
            break;
          case SQL_NUMERIC:
            columnValue = getBigInt(
                preparedStmt->result_array[column - 1].buffer,
                (memSizeType) preparedStmt->result_array[column - 1].length);
            break;
          default:
            logError(printf("sqlColumnBigInt: Column " FMT_D " has the unknown type %s.\n",
                            column, nameOfBufferType(
                            preparedStmt->result_array[column - 1].buffer_type)););
            raise_error(RANGE_ERROR);
            columnValue = NULL;
            break;
        } /* switch */
      } /* if */
    } /* if */
    /* printf("sqlColumnBigInt --> %s\n", bigHexCStri(columnValue)); */
    return columnValue;
  } /* sqlColumnBigInt */



static void sqlColumnBigRat (sqlStmtType sqlStatement, intType column,
    bigIntType *numerator, bigIntType *denominator)

  {
    preparedStmtType preparedStmt;
    float floatValue;
    double doubleValue;

  /* sqlColumnBigRat */
    /* printf("sqlColumnBigRat(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_array_size)) {
      logError(printf("sqlColumnBigRat: Fetch okay: %d, column: " FMT_D ", max column: %lu.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_array_size););
      raise_error(RANGE_ERROR);
    } else {
      if (preparedStmt->result_array[column - 1].length == SQL_NULL_DATA) {
        /* printf("Column is NULL -> Use default value: 0\n"); */
        *numerator = bigZero();
        *denominator = bigFromInt32(1);
      } else if (preparedStmt->result_array[column - 1].length < 0) {
        logError(printf("sqlColumnBigRat: Column " FMT_D " has length: %ld\n",
                        column, preparedStmt->result_array[column - 1].length););
        raise_error(FILE_ERROR);
        *numerator = NULL;
        *denominator = NULL;
      } else {
        /* printf("length: %ld\n", preparedStmt->result_array[column - 1].length); */
        /* printf("buffer_type: %s\n",
           nameOfBufferType(preparedStmt->result_array[column - 1].buffer_type)); */
        switch (preparedStmt->result_array[column - 1].buffer_type) {
          case SQL_TINYINT:
            *numerator = bigFromInt32((int32Type)
                *(int8Type *) preparedStmt->result_array[column - 1].buffer);
            *denominator = bigFromInt32(1);
            break;
          case SQL_SMALLINT:
            *numerator = bigFromInt32((int32Type)
                *(int16Type *) preparedStmt->result_array[column - 1].buffer);
            *denominator = bigFromInt32(1);
            break;
          case SQL_INTEGER:
            *numerator = bigFromInt32(
                *(int32Type *) preparedStmt->result_array[column - 1].buffer);
            *denominator = bigFromInt32(1);
            break;
          case SQL_BIGINT:
            *numerator = bigFromInt64(
                *(int64Type *) preparedStmt->result_array[column - 1].buffer);
            *denominator = bigFromInt32(1);
            break;
          case SQL_REAL:
            floatValue = *(float *) preparedStmt->result_array[column - 1].buffer;
            /* printf("sqlColumnBigRat: float: %f\n", floatValue); */
            *numerator = roundDoubleToBigRat(floatValue, FALSE, denominator);
            break;
          case SQL_FLOAT:
          case SQL_DOUBLE:
            doubleValue = *(double *) preparedStmt->result_array[column - 1].buffer;
            /* printf("sqlColumnBigRat: double: %f\n", doubleValue); */
            *numerator = roundDoubleToBigRat(doubleValue, TRUE, denominator);
            break;
          case SQL_DECIMAL:
            *numerator = getDecimalBigRational(
                preparedStmt->result_array[column - 1].buffer,
                (memSizeType) preparedStmt->result_array[column - 1].length,
                denominator);
            break;
          case SQL_NUMERIC:
            *numerator = getBigRational(
                preparedStmt->result_array[column - 1].buffer,
                (memSizeType) preparedStmt->result_array[column - 1].length,
                denominator);
            break;
          default:
            logError(printf("sqlColumnBigRat: Column " FMT_D " has the unknown type %s.\n",
                            column, nameOfBufferType(
                            preparedStmt->result_array[column - 1].buffer_type)););
            raise_error(RANGE_ERROR);
            break;
        } /* switch */
      } /* if */
    } /* if */
    /* printf("sqlColumnBigRat --> %s / %s\n",
       bigHexCStri(*numerator), bigHexCStri(*denominator)); */
  } /* sqlColumnBigRat */



static boolType sqlColumnBool (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    intType columnValue;

  /* sqlColumnBool */
    /* printf("sqlColumnBool(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_array_size)) {
      logError(printf("sqlColumnBool: Fetch okay: %d, column: " FMT_D ", max column: %lu.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_array_size););
      raise_error(RANGE_ERROR);
      columnValue = 0;
    } else {
      if (preparedStmt->result_array[column - 1].length == SQL_NULL_DATA) {
        /* printf("Column is NULL -> Use default value: FALSE\n"); */
        columnValue = 0;
      } else if (preparedStmt->result_array[column - 1].length < 0) {
        logError(printf("sqlColumnBool: Column " FMT_D " has length: %ld\n",
                        column, preparedStmt->result_array[column - 1].length););
        raise_error(FILE_ERROR);
        columnValue = 0;
      } else {
        /* printf("length: %ld\n", preparedStmt->result_array[column - 1].length); */
        /* printf("buffer_type: %s\n",
           nameOfBufferType(preparedStmt->result_array[column - 1].buffer_type)); */
        switch (preparedStmt->result_array[column - 1].buffer_type) {
          case SQL_CHAR:
          case SQL_VARCHAR:
          case SQL_LONGVARCHAR:
          case SQL_WCHAR:
          case SQL_WVARCHAR:
          case SQL_WLONGVARCHAR:
            if (preparedStmt->result_array[column - 1].length != 2) {
              logError(printf("sqlColumnBool: The size of a boolean field must be 1.\n"););
              raise_error(RANGE_ERROR);
              columnValue = 0;
            } else {
              columnValue = *(const_wstriType) preparedStmt->result_array[column - 1].buffer - '0';
            } /* if */
            break;
          case SQL_TINYINT:
            columnValue = *(int8Type *) preparedStmt->result_array[column - 1].buffer;
            break;
          case SQL_SMALLINT:
            columnValue = *(int16Type *) preparedStmt->result_array[column - 1].buffer;
            break;
          case SQL_INTEGER:
            columnValue = *(int32Type *) preparedStmt->result_array[column - 1].buffer;
            break;
          case SQL_BIGINT:
            columnValue = *(int64Type *) preparedStmt->result_array[column - 1].buffer;
            break;
          case SQL_DECIMAL:
            columnValue = getDecimalInt(
                preparedStmt->result_array[column - 1].buffer,
                (memSizeType) preparedStmt->result_array[column - 1].length);
            break;
          case SQL_NUMERIC:
            columnValue = getInt(
                preparedStmt->result_array[column - 1].buffer,
                (memSizeType) preparedStmt->result_array[column - 1].length);
            break;
          default:
            logError(printf("sqlColumnBool: Column " FMT_D " has the unknown type %s.\n",
                            column, nameOfBufferType(
                            preparedStmt->result_array[column - 1].buffer_type)););
            raise_error(RANGE_ERROR);
            columnValue = 0;
            break;
        } /* switch */
        if ((uintType) columnValue >= 2) {
          raise_error(RANGE_ERROR);
        } /* if */
      } /* if */
    } /* if */
    /* printf("sqlColumnBool --> %d\n", columnValue != 0); */
    return columnValue != 0;
  } /* sqlColumnBool */



static bstriType sqlColumnBStri (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    memSizeType length;
    bstriType bstri;

  /* sqlColumnBStri */
    /* printf("sqlColumnBStri(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_array_size)) {
      logError(printf("sqlColumnBStri: Fetch okay: %d, column: " FMT_D ", max column: %lu.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_array_size););
      raise_error(RANGE_ERROR);
      bstri = NULL;
    } else {
      if (preparedStmt->result_array[column - 1].length == SQL_NULL_DATA) {
        /* printf("Column is NULL -> Use default value: \"\"\n"); */
        if (unlikely(!ALLOC_BSTRI_SIZE_OK(bstri, 0))) {
          raise_error(MEMORY_ERROR);
        } else {
          bstri->size = 0;
        } /* if */
      } else if (preparedStmt->result_array[column - 1].length < 0) {
        logError(printf("sqlColumnBStri: Column " FMT_D " has length: %ld\n",
                        column, preparedStmt->result_array[column - 1].length););
        raise_error(FILE_ERROR);
        bstri = NULL;
      } else {
        /* printf("length: %ld\n", preparedStmt->result_array[column - 1].length); */
        /* printf("buffer_type: %s\n",
           nameOfBufferType(preparedStmt->result_array[column - 1].buffer_type)); */
        switch (preparedStmt->result_array[column - 1].buffer_type) {
          case SQL_BINARY:
          case SQL_VARBINARY:
          case SQL_LONGVARBINARY:
            length = (memSizeType) preparedStmt->result_array[column - 1].length;
            if (unlikely(!ALLOC_BSTRI_CHECK_SIZE(bstri, length))) {
              raise_error(MEMORY_ERROR);
            } else {
              bstri->size = length;
              memcpy(bstri->mem,
                     (ustriType) preparedStmt->result_array[column - 1].buffer,
                     length);
            } /* if */
            break;
          default:
            logError(printf("sqlColumnBStri: Column " FMT_D " has the unknown type %s.\n",
                            column, nameOfBufferType(
                            preparedStmt->result_array[column - 1].buffer_type)););
            raise_error(RANGE_ERROR);
            bstri = NULL;
            break;
        } /* switch */
      } /* if */
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
    SQL_INTERVAL_STRUCT *interval;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlColumnDuration */
    /* printf("sqlColumnDuration(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_array_size)) {
      logError(printf("sqlColumnDuration: Fetch okay: %d, column: " FMT_D ", max column: %lu.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_array_size););
      raise_error(RANGE_ERROR);
    } else {
      if (preparedStmt->result_array[column - 1].length == SQL_NULL_DATA) {
        /* printf("Column is NULL -> Use default value: 0-00-00 00:00:00\n"); */
        *year         = 0;
        *month        = 0;
        *day          = 0;
        *hour         = 0;
        *minute       = 0;
        *second       = 0;
        *micro_second = 0;
      } else if (preparedStmt->result_array[column - 1].length < 0) {
        logError(printf("sqlColumnDuration: Column " FMT_D " has length: %ld\n",
                        column, preparedStmt->result_array[column - 1].length););
        raise_error(FILE_ERROR);
      } else {
        /* printf("length: %ld\n", preparedStmt->result_array[column - 1].length); */
        /* printf("buffer_type: %s\n",
           nameOfBufferType(preparedStmt->result_array[column - 1].buffer_type)); */
        switch (preparedStmt->result_array[column - 1].buffer_type) {
          case SQL_INTERVAL_YEAR:
          case SQL_INTERVAL_MONTH:
          case SQL_INTERVAL_DAY:
          case SQL_INTERVAL_HOUR:
          case SQL_INTERVAL_MINUTE:
          case SQL_INTERVAL_SECOND:
          case SQL_INTERVAL_YEAR_TO_MONTH:
          case SQL_INTERVAL_DAY_TO_HOUR:
          case SQL_INTERVAL_DAY_TO_MINUTE:
          case SQL_INTERVAL_DAY_TO_SECOND:
          case SQL_INTERVAL_HOUR_TO_MINUTE:
          case SQL_INTERVAL_HOUR_TO_SECOND:
          case SQL_INTERVAL_MINUTE_TO_SECOND:
            *year         = 0;
            *month        = 0;
            *day          = 0;
            *hour         = 0;
            *minute       = 0;
            *second       = 0;
            *micro_second = 0;
            interval = (SQL_INTERVAL_STRUCT *) preparedStmt->result_array[column - 1].buffer;
            switch (interval->interval_type) {
              case SQL_IS_YEAR:
                *year = interval->intval.year_month.year;
                break;
              case SQL_IS_MONTH:
                *month = interval->intval.year_month.month;
                break;
              case SQL_IS_DAY:
                *day = interval->intval.day_second.day;
                break;
              case SQL_IS_HOUR:
                *hour = interval->intval.day_second.hour;
                break;
              case SQL_IS_MINUTE:
                *minute = interval->intval.day_second.minute;
                break;
              case SQL_IS_SECOND:
                *second = interval->intval.day_second.second;
                break;
              case SQL_IS_YEAR_TO_MONTH:
                *year = interval->intval.year_month.year;
                *month = interval->intval.year_month.month;
                break;
              case SQL_IS_DAY_TO_HOUR:
                *day = interval->intval.day_second.day;
                *hour = interval->intval.day_second.hour;
                break;
              case SQL_IS_DAY_TO_MINUTE:
                *day = interval->intval.day_second.day;
                *hour = interval->intval.day_second.hour;
                *minute = interval->intval.day_second.minute;
                break;
              case SQL_IS_DAY_TO_SECOND:
                *day = interval->intval.day_second.day;
                *hour = interval->intval.day_second.hour;
                *minute = interval->intval.day_second.minute;
                *second = interval->intval.day_second.second;
                break;
              case SQL_IS_HOUR_TO_MINUTE:
                *hour = interval->intval.day_second.hour;
                *minute = interval->intval.day_second.minute;
                break;
              case SQL_IS_HOUR_TO_SECOND:
                *hour = interval->intval.day_second.hour;
                *minute = interval->intval.day_second.minute;
                *second = interval->intval.day_second.second;
                break;
              case SQL_IS_MINUTE_TO_SECOND:
                *minute = interval->intval.day_second.minute;
                *second = interval->intval.day_second.second;
                break;
            } /* switch */
            if (interval->interval_sign == SQL_TRUE) {
              *year         = -*year;
              *month        = -*month;
              *day          = -*day;
              *hour         = -*hour;
              *minute       = -*minute;
              *second       = -*second;
              *micro_second = -*micro_second;
            } /* if */
            break;
          default:
            logError(printf("sqlColumnDuration: Column " FMT_D " has the unknown type %s.\n",
                            column, nameOfBufferType(
                            preparedStmt->result_array[column - 1].buffer_type)););
            err_info = RANGE_ERROR;
            break;
        } /* switch */
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
        } /* if */
      } /* if */
    } /* if */
    /* printf("sqlColumnDuration --> %d-%02d-%02d %02d:%02d:%02d\n",
       *year, *month, *day, *hour, *minute, *second); */
  } /* sqlColumnDuration */



static floatType sqlColumnFloat (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    floatType columnValue;

  /* sqlColumnFloat */
    /* printf("sqlColumnFloat(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_array_size)) {
      logError(printf("sqlColumnFloat: Fetch okay: %d, column: " FMT_D ", max column: %lu.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_array_size););
      raise_error(RANGE_ERROR);
      columnValue = 0.0;
    } else {
      if (preparedStmt->result_array[column - 1].length == SQL_NULL_DATA) {
        /* printf("Column is NULL -> Use default value: 0.0\n"); */
        columnValue = 0.0;
      } else if (preparedStmt->result_array[column - 1].length < 0) {
        logError(printf("sqlColumnFloat: Column " FMT_D " has length: %ld\n",
                        column, preparedStmt->result_array[column - 1].length););
        raise_error(FILE_ERROR);
        columnValue = 0.0;
      } else {
        /* printf("length: %ld\n", preparedStmt->result_array[column - 1].length); */
        /* printf("buffer_type: %s\n",
           nameOfBufferType(preparedStmt->result_array[column - 1].buffer_type)); */
        switch (preparedStmt->result_array[column - 1].buffer_type) {
          case SQL_REAL:
            columnValue = *(float *) preparedStmt->result_array[column - 1].buffer;
            /* printf("sqlColumnFloat: float: %f\n", columnValue); */
            break;
          case SQL_FLOAT:
          case SQL_DOUBLE:
            columnValue = *(double *) preparedStmt->result_array[column - 1].buffer;
            /* printf("sqlColumnFloat: double: %f\n", columnValue); */
            break;
          case SQL_DECIMAL:
            columnValue = getDecimalFloat(
                preparedStmt->result_array[column - 1].buffer,
                (memSizeType) preparedStmt->result_array[column - 1].length);
            break;
          case SQL_NUMERIC:
            columnValue = getFloat(
                preparedStmt->result_array[column - 1].buffer,
                (memSizeType) preparedStmt->result_array[column - 1].length);
            break;
          default:
            logError(printf("sqlColumnFloat: Column " FMT_D " has the unknown type %s.\n",
                            column, nameOfBufferType(
                            preparedStmt->result_array[column - 1].buffer_type)););
            raise_error(RANGE_ERROR);
            columnValue = 0.0;
            break;
        } /* switch */
      } /* if */
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
                 (uintType) column > preparedStmt->result_array_size)) {
      logError(printf("sqlColumnInt: Fetch okay: %d, column: " FMT_D ", max column: %lu.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_array_size););
      raise_error(RANGE_ERROR);
      columnValue = 0;
    } else {
      if (preparedStmt->result_array[column - 1].length == SQL_NULL_DATA) {
        /* printf("Column is NULL -> Use default value: 0\n"); */
        columnValue = 0;
      } else if (preparedStmt->result_array[column - 1].length < 0) {
        logError(printf("sqlColumnInt: Column " FMT_D " has length: %ld\n",
                        column, preparedStmt->result_array[column - 1].length););
        raise_error(FILE_ERROR);
        columnValue = 0;
      } else {
        /* printf("length: %ld\n", preparedStmt->result_array[column - 1].length); */
        /* printf("buffer_type: %s\n",
           nameOfBufferType(preparedStmt->result_array[column - 1].buffer_type)); */
        switch (preparedStmt->result_array[column - 1].buffer_type) {
          case SQL_TINYINT:
            columnValue = *(int8Type *) preparedStmt->result_array[column - 1].buffer;
            break;
          case SQL_SMALLINT:
            columnValue = *(int16Type *) preparedStmt->result_array[column - 1].buffer;
            break;
          case SQL_INTEGER:
            columnValue = *(int32Type *) preparedStmt->result_array[column - 1].buffer;
            break;
          case SQL_BIGINT:
            columnValue = *(int64Type *) preparedStmt->result_array[column - 1].buffer;
            break;
          case SQL_DECIMAL:
            columnValue = getDecimalInt(
                preparedStmt->result_array[column - 1].buffer,
                (memSizeType) preparedStmt->result_array[column - 1].length);
            break;
          case SQL_NUMERIC:
            columnValue = getInt(
                preparedStmt->result_array[column - 1].buffer,
                (memSizeType) preparedStmt->result_array[column - 1].length);
            break;
          default:
            logError(printf("sqlColumnInt: Column " FMT_D " has the unknown type %s.\n",
                            column, nameOfBufferType(
                            preparedStmt->result_array[column - 1].buffer_type)););
            raise_error(RANGE_ERROR);
            columnValue = 0;
            break;
        } /* switch */
      } /* if */
    } /* if */
    /* printf("sqlColumnInt --> " FMT_D "\n", columnValue); */
    return columnValue;
  } /* sqlColumnInt */



static striType sqlColumnStri (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    memSizeType length;
    wstriType wstri;
    errInfoType err_info = OKAY_NO_ERROR;
    striType stri;

  /* sqlColumnStri */
    /* printf("sqlColumnStri(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_array_size)) {
      logError(printf("sqlColumnStri: Fetch okay: %d, column: " FMT_D ", max column: %lu.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_array_size););
      raise_error(RANGE_ERROR);
      stri = NULL;
    } else {
      if (preparedStmt->result_array[column - 1].length == SQL_NULL_DATA) {
        /* printf("Column is NULL -> Use default value: \"\"\n"); */
        stri = strEmpty();
      } else if (preparedStmt->result_array[column - 1].length < 0) {
        logError(printf("sqlColumnStri: Column " FMT_D " has length: %ld\n",
                        column, preparedStmt->result_array[column - 1].length););
        raise_error(FILE_ERROR);
        stri = NULL;
      } else {
        /* printf("length: %ld\n", preparedStmt->result_array[column - 1].length); */
        /* printf("buffer_type: %s\n",
           nameOfBufferType(preparedStmt->result_array[column - 1].buffer_type)); */
        switch (preparedStmt->result_array[column - 1].buffer_type) {
          case SQL_VARCHAR:
          case SQL_LONGVARCHAR:
          case SQL_WVARCHAR:
          case SQL_WLONGVARCHAR:
            length = (memSizeType) preparedStmt->result_array[column - 1].length >> 1;
            stri = wstri_buf_to_stri(
                (wstriType) preparedStmt->result_array[column - 1].buffer,
                length,
                &err_info);
            if (err_info != OKAY_NO_ERROR) {
              raise_error(err_info);
            } /* if */
            break;
          case SQL_CHAR:
          case SQL_WCHAR:
            length = (memSizeType) preparedStmt->result_array[column - 1].length >> 1;
            wstri = (wstriType) preparedStmt->result_array[column - 1].buffer;
            while (length > 0 && wstri[length - 1] == ' ') {
              length--;
            } /* if */
            stri = wstri_buf_to_stri(wstri, length, &err_info);
            if (err_info != OKAY_NO_ERROR) {
              raise_error(err_info);
            } /* if */
            break;
          case SQL_BINARY:
          case SQL_VARBINARY:
          case SQL_LONGVARBINARY:
            stri = cstri_buf_to_stri(
                (cstriType) preparedStmt->result_array[column - 1].buffer,
                (memSizeType) preparedStmt->result_array[column - 1].length);
            if (stri == NULL) {
              raise_error(MEMORY_ERROR);
            } /* if */
            break;
          default:
            logError(printf("sqlColumnStri: Column " FMT_D " has the unknown type %s.\n",
                            column, nameOfBufferType(
                            preparedStmt->result_array[column - 1].buffer_type)););
            raise_error(RANGE_ERROR);
            stri = NULL;
            break;
        } /* switch */
      } /* if */
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
    SQL_DATE_STRUCT *dateValue;
    SQL_TIME_STRUCT *timeValue;
    SQL_TIMESTAMP_STRUCT *timestampValue;
    errInfoType err_info = OKAY_NO_ERROR;

  /* sqlColumnTime */
    /* printf("sqlColumnTime(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(!preparedStmt->fetchOkay || column < 1 ||
                 (uintType) column > preparedStmt->result_array_size)) {
      logError(printf("sqlColumnTime: Fetch okay: %d, column: " FMT_D ", max column: %lu.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_array_size););
      raise_error(RANGE_ERROR);
    } else {
      if (preparedStmt->result_array[column - 1].length == SQL_NULL_DATA) {
        /* printf("Column is NULL -> Use default value: 0-01-01 00:00:00\n"); */
        *year         = 0;
        *month        = 1;
        *day          = 1;
        *hour         = 0;
        *minute       = 0;
        *second       = 0;
        *micro_second = 0;
        *time_zone    = 0;
        *is_dst       = 0;
      } else if (preparedStmt->result_array[column - 1].length < 0) {
        logError(printf("sqlColumnTime: Column " FMT_D " has length: %ld\n",
                        column, preparedStmt->result_array[column - 1].length););
        raise_error(FILE_ERROR);
      } else {
        /* printf("length: %ld\n", preparedStmt->result_array[column - 1].length); */
        /* printf("buffer_type: %s\n",
           nameOfBufferType(preparedStmt->result_array[column - 1].buffer_type)); */
        switch (preparedStmt->result_array[column - 1].buffer_type) {
          case SQL_TYPE_DATE:
            dateValue = (SQL_DATE_STRUCT *) preparedStmt->result_array[column - 1].buffer;
            *year         = dateValue->year;
            *month        = dateValue->month;
            *day          = dateValue->day;
            *hour         = 0;
            *minute       = 0;
            *second       = 0;
            *micro_second = 0;
            timSetLocalTZ(*year, *month, *day, *hour, *minute, *second,
                          time_zone, is_dst);
            break;
          case SQL_TYPE_TIME:
            timeValue = (SQL_TIME_STRUCT *) preparedStmt->result_array[column - 1].buffer;
            *year         = 2000;
            *month        = 1;
            *day          = 1;
            *hour         = timeValue->hour;
            *minute       = timeValue->minute;
            *second       = timeValue->second;
            *micro_second = 0;
            timSetLocalTZ(*year, *month, *day, *hour, *minute, *second,
                          time_zone, is_dst);
            *year = 0;
            break;
          case SQL_DATETIME:
          case SQL_TYPE_TIMESTAMP:
            timestampValue = (SQL_TIMESTAMP_STRUCT *) preparedStmt->result_array[column - 1].buffer;
            *year         = timestampValue->year;
            *month        = timestampValue->month;
            *day          = timestampValue->day;
            *hour         = timestampValue->hour;
            *minute       = timestampValue->minute;
            *second       = timestampValue->second;
            *micro_second = timestampValue->fraction;
            timSetLocalTZ(*year, *month, *day, *hour, *minute, *second,
                          time_zone, is_dst);
            break;
          default:
            logError(printf("sqlColumnTime: Column " FMT_D " has the unknown type %s.\n",
                            column, nameOfBufferType(
                            preparedStmt->result_array[column - 1].buffer_type)););
            err_info = RANGE_ERROR;
            break;
        } /* switch */
        if (err_info != OKAY_NO_ERROR) {
          raise_error(err_info);
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
    SQLRETURN execute_result;

  /* sqlExecute */
    /* printf("begin sqlExecute(%lx)\n", (unsigned long) sqlStatement); */
    preparedStmt = (preparedStmtType) sqlStatement;
    /* printf("ppStmt: %lx\n", (unsigned long) preparedStmt->ppStmt); */
    if (preparedStmt->executeSuccessful &&
        (preparedStmt->fetchOkay || preparedStmt->fetchFinished)) {
      if (SQLFreeStmt(preparedStmt->ppStmt, SQL_CLOSE) != SQL_SUCCESS) {
        logError(printf("sqlExecute: SQLFreeStmt SQL_CLOSE:\n");
                 printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
        raise_error(FILE_ERROR);
      } /* if */
    } /* if */
    preparedStmt->fetchOkay = FALSE;
    /* showBindVars(preparedStmt); */
    /* showResultVars(preparedStmt); */
    execute_result = SQLExecute(preparedStmt->ppStmt);
    if (execute_result == SQL_NO_DATA || execute_result == SQL_SUCCESS) {
      preparedStmt->executeSuccessful = TRUE;
      preparedStmt->fetchFinished = FALSE;
    } else if (execute_result == SQL_ERROR || execute_result == SQL_SUCCESS_WITH_INFO) {
      logError(printf("sqlExecute: SQLExecute:\n");
               printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
      preparedStmt->executeSuccessful = FALSE;
      raise_error(FILE_ERROR);
    } else {
      logError(printf("sqlExecute: SQLExecute returns %d\n",
                      execute_result););
    } /* if */
    /* printf("end sqlExecute\n"); */
  } /* sqlExecute */



static boolType sqlFetch (sqlStmtType sqlStatement)

  {
    preparedStmtType preparedStmt;
    SQLRETURN fetch_result;

  /* sqlFetch */
    /* printf("begin sqlFetch(%lx)\n", (unsigned long) sqlStatement); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (!preparedStmt->executeSuccessful) {
      logError(printf("sqlFetch: Execute was not successful\n"););
      preparedStmt->fetchOkay = FALSE;
      raise_error(FILE_ERROR);
    } else if (preparedStmt->result_array_size == 0) {
      preparedStmt->fetchOkay = FALSE;
    } else if (!preparedStmt->fetchFinished) {
      /* printf("ppStmt: %lx\n", (unsigned long) preparedStmt->ppStmt); */
      fetch_result = SQLFetch(preparedStmt->ppStmt);
      if (fetch_result == SQL_SUCCESS) {
        /* printf("fetch success\n");
        showResultVars(preparedStmt); */
        preparedStmt->fetchOkay = TRUE;
      } else if (fetch_result == SQL_NO_DATA) {
        preparedStmt->fetchOkay = FALSE;
        preparedStmt->fetchFinished = TRUE;
      } else {
        logError(printf("sqlFetch: fetch_result: %d\n",
                        fetch_result);
                 printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
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
                 (uintType) column > preparedStmt->result_array_size)) {
      logError(printf("sqlIsNull: Fetch okay: %d, column: " FMT_D ", max column: %lu.\n",
                      preparedStmt->fetchOkay, column,
                      preparedStmt->result_array_size););
      raise_error(RANGE_ERROR);
      isNull = FALSE;
    } else {
      isNull = preparedStmt->result_array[column - 1].length == SQL_NULL_DATA;
    } /* if */
    /* printf("sqlIsNull --> %d\n", isNull); */
    return isNull;
  } /* sqlIsNull */



static sqlStmtType sqlPrepare (databaseType database, striType sqlStatementStri)

  {
    dbType db;
    wstriType query;
    memSizeType query_length;
    errInfoType err_info = OKAY_NO_ERROR;
    preparedStmtType preparedStmt;

  /* sqlPrepare */
    /* printf("begin sqlPrepare(database, ");
       prot_stri(sqlStatementStri);
       printf(")\n"); */
    db = (dbType) database;
    query = stri_to_wstri_buf(sqlStatementStri, &query_length, &err_info);
    if (query == NULL) {
      preparedStmt = NULL;
    } else {
      if (query_length > SQLINTEGER_MAX) {
        logError(printf("sqlPrepare: query_length > SQLINTEGER_MAX: (query_length = " FMT_U_MEM ")\n",
                        query_length););
        err_info = MEMORY_ERROR;
        preparedStmt = NULL;
      } else if (!ALLOC_RECORD(preparedStmt, preparedStmtRecord, count.prepared_stmt)) {
        err_info = MEMORY_ERROR;
      } else {
        memset(preparedStmt, 0, sizeof(preparedStmtRecord));
        if (SQLAllocHandle(SQL_HANDLE_STMT,
                           db->sql_connection,
                           &preparedStmt->ppStmt) != SQL_SUCCESS) {
          logError(printf("sqlPrepare: SQLAllocHandle:\n");
                   printError(SQL_HANDLE_DBC, db->sql_environment););
          FREE_RECORD(preparedStmt, preparedStmtRecord, count.prepared_stmt);
          err_info = FILE_ERROR;
          preparedStmt = NULL;
        } else if (SQLPrepareW(preparedStmt->ppStmt,
                               (SQLWCHAR *) query,
                               (SQLINTEGER) query_length) != SQL_SUCCESS) {
          logError(printf("sqlPrepare: SQLPrepare:\n");
                   printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
          FREE_RECORD(preparedStmt, preparedStmtRecord, count.prepared_stmt);
          err_info = FILE_ERROR;
          preparedStmt = NULL;
        } else {
          preparedStmt->usage_count = 1;
          preparedStmt->sqlFunc = db->sqlFunc;
          preparedStmt->executeSuccessful = FALSE;
          preparedStmt->fetchOkay = FALSE;
          preparedStmt->fetchFinished = TRUE;
          setupResult(preparedStmt, &err_info);
          if (err_info != OKAY_NO_ERROR) {
            freePreparedStmt((sqlStmtType) preparedStmt);
            preparedStmt = NULL;
          } /* if */
        } /* if */
      } /* if */
      free_wstri(query, sqlStatementStri);
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
    if (unlikely(preparedStmt->result_array_size > INTTYPE_MAX)) {
      raise_error(RANGE_ERROR);
      columnCount = 0;
    } else {
      columnCount = (intType) preparedStmt->result_array_size;
    } /* if */
    /* printf("sqlStmtColumnCount --> %d\n", columnCount); */
    return columnCount;
  } /* sqlStmtColumnCount */



static striType sqlStmtColumnName (sqlStmtType sqlStatement, intType column)

  {
    preparedStmtType preparedStmt;
    SQLRETURN returnCode;
    SQLSMALLINT stringLength;
    wstriType wideName;
    errInfoType err_info = OKAY_NO_ERROR;
    striType name;

  /* sqlStmtColumnName */
    /* printf("sqlStmtColumnName(%lx, " FMT_D ")\n", (unsigned long) sqlStatement, column); */
    preparedStmt = (preparedStmtType) sqlStatement;
    if (unlikely(column < 1 ||
                 (uintType) column > preparedStmt->result_array_size)) {
      logError(printf("sqlStmtColumnName: column: " FMT_D ", max column: %lu.\n",
                      column, preparedStmt->result_array_size););
      err_info = RANGE_ERROR;
      name = NULL;
    } else {
      returnCode = SQLColAttributeW(preparedStmt->ppStmt,
                                    (SQLUSMALLINT) column,
                                    SQL_DESC_NAME,
                                    NULL,
                                    0,
                                    &stringLength,
                                    NULL);
      if (returnCode != SQL_SUCCESS &&
          returnCode != SQL_SUCCESS_WITH_INFO) {
        logError(printf("sqlStmtColumnName: SQLColAttribute SQL_DESC_BASE_COLUMN_NAME:\n");
                 printf("returnCode: %d\n", returnCode);
                 printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
        err_info = FILE_ERROR;
        name = NULL;
      } else if (stringLength < 0 || stringLength > SQLSMALLINT_MAX - 2) {
        logError(printf("sqlStmtColumnName: SQLColAttribute returned a stringLength that is negative or too big.\n"););
        err_info = FILE_ERROR;
        name = NULL;
      } else if (unlikely(!ALLOC_WSTRI(wideName, (memSizeType) stringLength))) {
        err_info = MEMORY_ERROR;
        name = NULL;
      } else if (SQLColAttributeW(preparedStmt->ppStmt,
                                  (SQLUSMALLINT) column,
                                  SQL_DESC_NAME,
                                  wideName,
                                  (SQLSMALLINT) (stringLength + 2),
                                  NULL,
                                  NULL) != SQL_SUCCESS) {
        logError(printf("sqlStmtColumnName: SQLColAttribute SQL_DESC_BASE_COLUMN_NAME:\n");
                 printError(SQL_HANDLE_STMT, preparedStmt->ppStmt););
        UNALLOC_WSTRI(wideName, stringLength);
        err_info = FILE_ERROR;
        name = NULL;
      } else {
        name = wstri_buf_to_stri(wideName, (memSizeType) (stringLength >> 1), &err_info);
        UNALLOC_WSTRI(wideName, stringLength);
      } /* if */
    } /* if */
    if (err_info != OKAY_NO_ERROR) {
      raise_error(err_info);
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



static void listDrivers (SQLHENV sql_environment)

  {
    SQLCHAR driver[4096];
    SQLCHAR attr[4096];
    SQLSMALLINT driver_ret;
    SQLSMALLINT attr_ret;
    SQLUSMALLINT direction;

  /* listDrivers */
    printf("Available odbc drivers:\n");
    direction = SQL_FETCH_FIRST;
    while (SQLDrivers(sql_environment, direction,
                      driver, sizeof(driver), &driver_ret,
                      attr, sizeof(attr), &attr_ret) == SQL_SUCCESS) {
      direction = SQL_FETCH_NEXT;
      printf("%s - %s\n", driver, attr);
    } /* while */
    printf("--- end of list ---\n");
  } /* listDrivers */



static void listDataSources (SQLHENV sql_environment)

  {
    SQLCHAR dsn[4096];
    SQLCHAR desc[4096];
    SQLSMALLINT dsn_ret;
    SQLSMALLINT desc_ret;
    SQLUSMALLINT direction;

  /* listDataSources */
    printf("List of data sources:\n");
    direction = SQL_FETCH_FIRST;
    while (SQLDataSources(sql_environment, direction,
                          dsn, sizeof(dsn), &dsn_ret,
                          desc, sizeof(desc), &desc_ret) == SQL_SUCCESS) {
      direction = SQL_FETCH_NEXT;
      printf("%s - %s\n", dsn, desc);
    } /* while */
    printf("--- end of list ---\n");
  } /* listDataSources */



databaseType sqlOpenOdbc (const const_striType dbName,
    const const_striType user, const const_striType password)

  {
    wstriType dbNameW;
    memSizeType dbNameW_length;
    wstriType userW;
    memSizeType userW_length;
    wstriType passwordW;
    memSizeType passwordW_length;
    SQLHENV sql_environment;
    SQLHDBC sql_connection;
    errInfoType err_info = OKAY_NO_ERROR;
    dbType database;

  /* sqlOpenOdbc */
    /* printf("sqlOpenOdbc(");
       prot_stri(dbName);
       printf(", ");
       prot_stri(user);
       printf(", ");
       prot_stri(password);
       printf(")\n"); */
    if (!setupDll(ODBC_DLL)) {
      logError(printf("sqlOpenOdbc: setupDll(\"%s\") failed\n", ODBC_DLL););
      err_info = FILE_ERROR;
      database = NULL;
    } else {
      dbNameW = stri_to_wstri_buf(dbName, &dbNameW_length, &err_info);
      if (dbNameW == NULL) {
        err_info = MEMORY_ERROR;
        database = NULL;
      } else {
        userW = stri_to_wstri_buf(user, &userW_length, &err_info);
        if (userW == NULL) {
          err_info = MEMORY_ERROR;
          database = NULL;
        } else {
          passwordW = stri_to_wstri_buf(password, &passwordW_length, &err_info);
          if (passwordW == NULL) {
            err_info = MEMORY_ERROR;
            database = NULL;
          } else {
            /* printf("dbName:   %ls\n", dbNameW);
               printf("user:     %ls\n", userW);
               printf("password: %ls\n", passwordW); */
            if (dbNameW_length   > SHRT_MAX ||
                userW_length     > SHRT_MAX ||
                passwordW_length > SHRT_MAX) {
              err_info = MEMORY_ERROR;
              database = NULL;
            } else if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sql_environment) != SQL_SUCCESS) {
              logError(printf("sqlOpenOdbc: SQLAllocHandle failed\n"););
              err_info = FILE_ERROR;
              database = NULL;
            } else if (SQLSetEnvAttr(sql_environment,
                                     SQL_ATTR_ODBC_VERSION,
                                     (SQLPOINTER) SQL_OV_ODBC3,
                                     SQL_IS_INTEGER) != SQL_SUCCESS) {
              logError(printf("sqlOpenOdbc: SQLSetEnvAttr:\n");
                       printError(SQL_HANDLE_ENV, sql_environment););
              err_info = FILE_ERROR;
              SQLFreeHandle(SQL_HANDLE_ENV, sql_environment);
              database = NULL;
            } else if (SQLAllocHandle(SQL_HANDLE_DBC, sql_environment, &sql_connection) != SQL_SUCCESS) {
              logError(printf("sqlOpenOdbc: SQLAllocHandle:\n");
                       printError(SQL_HANDLE_ENV, sql_environment););
              err_info = FILE_ERROR;
              SQLFreeHandle(SQL_HANDLE_ENV, sql_environment);
              database = NULL;
            } else if (SQLConnectW(sql_connection,
                                   dbNameW, (SQLSMALLINT) dbNameW_length,
                                   userW, (SQLSMALLINT) userW_length,
                                   passwordW, (SQLSMALLINT) passwordW_length) != SQL_SUCCESS) {
              logError(printf("sqlOpenOdbc: SQLConnect:\n");
                       printError(SQL_HANDLE_DBC, sql_connection);
                       listDrivers(sql_environment);
                       listDataSources(sql_environment););
              err_info = FILE_ERROR;
              SQLFreeHandle(SQL_HANDLE_DBC, sql_connection);
              SQLFreeHandle(SQL_HANDLE_ENV, sql_environment);
              database = NULL;
            } else if (unlikely(!setupFuncTable() ||
                                !ALLOC_RECORD(database, dbRecord, count.database))) {
              err_info = MEMORY_ERROR;
              SQLDisconnect(sql_connection);
              SQLFreeHandle(SQL_HANDLE_DBC, sql_connection);
              SQLFreeHandle(SQL_HANDLE_ENV, sql_environment);
              database = NULL;
            } else {
              memset(database, 0, sizeof(dbRecord));
              database->usage_count = 1;
              database->sqlFunc = sqlFunc;
              database->sql_environment = sql_environment;
              database->sql_connection = sql_connection;
            } /* if */
            free_wstri(passwordW, password);
          } /* if */
          free_wstri(userW, user);
        } /* if */
        free_wstri(dbNameW, dbName);
      } /* if */
    } /* if */
    if (err_info != OKAY_NO_ERROR) {
      raise_error(err_info);
    } /* if */
    return (databaseType) database;
  } /* sqlOpenOdbc */

#endif
