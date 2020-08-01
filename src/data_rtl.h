/********************************************************************/
/*                                                                  */
/*  data_rtl.h    Basic type definitions and settings.              */
/*  Copyright (C) 1989 - 2005  Thomas Mertes                        */
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
/*  File: seed7/src/data_rtl.h                                      */
/*  Changes: 1990 - 1994, 2000, 2005  Thomas Mertes                 */
/*  Content: Type definitions for general data structures.          */
/*                                                                  */
/********************************************************************/

/* Since a function cannot return structs by value and */
/* cannot have struct value parameters rtlGenerictype  */
/* is used in these cases instead of rtlObjecttype.    */
/* To work reliably it must be assured that            */
/* sizeof(rtlGenerictype) == sizeof(rtlObjecttype)     */

#if INTTYPE_SIZE >= POINTER_SIZE
#if INTTYPE_SIZE >= FLOATTYPE_SIZE
#define GENERIC_SIZE INTTYPE_SIZE
#else
#define GENERIC_SIZE FLOATTYPE_SIZE
#endif
#elif POINTER_SIZE >= FLOATTYPE_SIZE
#define GENERIC_SIZE POINTER_SIZE
#else
#define GENERIC_SIZE FLOATTYPE_SIZE
#endif

#if   GENERIC_SIZE == 64
typedef uint64type rtlGenerictype;
#elif GENERIC_SIZE == 32
typedef uint32type rtlGenerictype;
#endif

typedef struct rtlTypestruct   *rtlTypetype;
typedef struct rtlListstruct   *rtlListtype;
typedef struct rtlArraystruct  *rtlArraytype;
typedef struct rtlStructstruct *rtlStructtype;
typedef struct rtlStructstruct *rtlInterfacetype;
typedef struct rtlHelemstruct  *rtlHelemtype;
typedef struct rtlHashstruct   *rtlHashtype;
typedef void *rtlPtrtype;

typedef const struct rtlTypestruct   *const_rtlTypetype;
typedef const struct rtlListstruct   *const_rtlListtype;
typedef const struct rtlArraystruct  *const_rtlArraytype;
typedef const struct rtlStructstruct *const_rtlStructtype;
typedef const struct rtlStructstruct *const_rtlInterfacetype;
typedef const struct rtlHelemstruct  *const_rtlHelemtype;
typedef const struct rtlHashstruct   *const_rtlHashtype;
typedef const void *const_rtlPtrtype;

typedef union {
  /* Genericvalue must be the first element in the union. */
  /* This allows initialisations of rtlValueunion values. */
    rtlGenerictype genericvalue;
/*    postype          pos;            ** SYMBOLOBJECT */
/*    nodetype         nodevalue;      ** MDULEOBJECT */
/*    booltype         boolvalue;      */
    rtlTypetype      typevalue;      /* TYPEOBJECT */
    inttype          intvalue;       /* INTOBJECT */
    biginttype       bigintvalue;    /* BIGINTOBJECT */
    chartype         charvalue;      /* CHAROBJECT */
    stritype         strivalue;      /* STRIOBJECT */
    bstritype        bstrivalue;     /* BSTRIOBJECT */
    rtlArraytype     arrayvalue;     /* ARRAYOBJECT */
    rtlHashtype      hashvalue;      /* HASHOBJECT */
    settype          setvalue;       /* SETOBJECT */
    rtlStructtype    structvalue;    /* STRUCTOBJECT */
    rtlInterfacetype interfacevalue; /* INTERFACEOBJECT */
    filetype         filevalue;      /* FILEOBJECT */
    sockettype       socketvalue;    /* SOCKETOBJECT */
    rtlListtype      listvalue;      /* LISTOBJECT, EXPROBJECT */
    wintype          winvalue;       /* WINOBJECT */
    rtlPtrtype       ptrvalue;
/*    enumtype         enumvalue;      */
/*    polltype         pollvalue;      */
/*    objreftype       objrefvalue;    */
/*    rtlObjecttype    objvalue;       ** ENUMLITERALOBJECT, CONSTENUMOBJECT */
                                     /* VARENUMOBJECT, VALUEPARAMOBJECT */
                                     /* REFPARAMOBJECT, RESULTOBJECT */
                                     /* LOCALVOBJECT, FORMPARAMOBJECT */
/*    blocktype        blockvalue;     ** BLOCKOBJECT */
/*    acttype          actvalue;       ** ACTOBJECT */
/*    progtype         progvalue;      ** PROGOBJECT */
#ifdef WITH_FLOAT
    floattype      floatvalue;   /* FLOATOBJECT */
#endif
  } rtlValueunion;

typedef struct rtlObjectstruct {
    rtlValueunion value;
  } rtlObjecttype;

typedef const struct rtlObjectstruct const_rtlObjecttype;

typedef struct rtlTypestruct {
    int dummy;
  } rtlTyperecord;

typedef struct rtlListstruct {
    rtlListtype next;
    rtlObjecttype obj;
  } rtlListrecord;

typedef struct rtlArraystruct {
    inttype min_position;
    inttype max_position;
    rtlObjecttype arr[1];
  } rtlArrayrecord;

struct rtlStructstruct {
    memsizetype usage_count;
    uint32type type_num;
    rtlObjecttype stru[1];
  };

typedef struct rtlHelemstruct {
    rtlHelemtype next_less;
    rtlHelemtype next_greater;
    rtlObjecttype key;
    rtlObjecttype data;
  } rtlHelemrecord;

typedef struct rtlHashstruct {
    unsigned int bits;
    unsigned int mask;
    unsigned int table_size;
    memsizetype size;
    rtlHelemtype table[1];
  } rtlHashrecord;

typedef inttype (*comparetype) (rtlGenerictype, rtlGenerictype);
typedef rtlGenerictype (*createfunctype) (rtlGenerictype);
typedef void (*destrfunctype) (rtlGenerictype);
typedef void (*copyfunctype) (rtlGenerictype *, rtlGenerictype);


inttype genericCmp (const rtlGenerictype value1, const rtlGenerictype value2);
void genericCpy (rtlGenerictype *const dest, const rtlGenerictype source);
rtlGenerictype genericCreate (rtlGenerictype source);
void genericDestr (rtlGenerictype old_value);
