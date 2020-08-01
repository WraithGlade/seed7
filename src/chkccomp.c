/********************************************************************/
/*                                                                  */
/*  chkccomp   Check properties of C compiler and runtime.          */
/*  Copyright (C) 2010, 2011, 2012  Thomas Mertes                   */
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
/*  Module: Chkccomp                                                */
/*  File: seed7/src/chkccomp.c                                      */
/*  Changes: 2010, 2011, 2012  Thomas Mertes                        */
/*  Content: Program to Check properties of C compiler and runtime. */
/*                                                                  */
/********************************************************************/

#include "version.h"

/**
 *  From version.h the following defines are used (for details see: read_me.txt):
 *
 *  os_off_t
 *      Type used for os_fseek(), os_ftell(), offsetSeek(), offsetTell()
 *      and seekFileLength().
 *  TURN_OFF_FP_EXCEPTIONS
 *      Use the function _control87() to turn off floating point exceptions.
 *  DEFINE_MATHERR_FUNCTION
 *      Define the function _matherr() which handles floating point errors.
 *  PATH_DELIMITER:
 *      Path delimiter character used by the command shell of the operating system.
 *  QUOTE_WHOLE_SHELL_COMMAND:
 *      Defined when shell commands, starting with " need to be quoted a again.
 *  OBJECT_FILE_EXTENSION:
 *      The extension used by the C compiler for object files.
 *  EXECUTABLE_FILE_EXTENSION:
 *      The extension which is used by the operating system for executables.
 *  CC_NO_OPT_OUTPUT_FILE:
 *      Defined, when compiling and linking with one command cannot use -o.
 *  REDIRECT_C_ERRORS:
 *      The redirect command to redirect the errors of the C compiler to a file.
 *  LINKER_OPT_OUTPUT_FILE:
 *      Contains the linker option to provide the output filename (e.g.: "-o ").
 */

#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "time.h"
#include "float.h"
#include "math.h"
#include "sys/types.h"
#include "sys/stat.h"

#include "config.h"

/**
 *  From config.h the following defines are used (for details see: read_me.txt):
 *
 *  MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
 *      Map absolute paths to operating system paths with drive letter.
 */

#include "chkccomp.h"

/**
 *  The file chkccomp.h is NOT part of the Seed7 release.
 *  Instead chkccomp.h is generated by the makefile and
 *  removed after chkccomp was compiled and executed.
 *  In chkccomp.h the following macros might be defined:
 *
 *  mkdir(path,mode)
 *      Macro to replace the Posix function mkdir.
 *      E.g.: #define mkdir(path,mode) mkdir(path)
 *            #define mkdir(path,mode) _mkdir(path)
 *  rmdir
 *      Name of Posix function rmdir.
 *      E.g.: #define rmdir _rmdir
 *  WRITE_CC_VERSION_INFO
 *      Write the version of the C compiler to the file "cc_vers.txt".
 *      E.g.: #define WRITE_CC_VERSION_INFO system("$(GET_CC_VERSION_INFO) cc_vers.txt");
 *  LIST_DIRECTORY_CONTENTS
 *      Either "ls" or "dir".
 *      E.g.: #define LIST_DIRECTORY_CONTENTS "ls"
 *            #define LIST_DIRECTORY_CONTENTS "dir"
 *  The macros described above are only used in the program chkccomp.
 *  This macros are not used in the Seed7 Interpreter (s7) or in the
 *  Seed7 Runtime Library.
 */


#ifndef EXECUTABLE_FILE_EXTENSION
#define EXECUTABLE_FILE_EXTENSION ""
#endif

#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

#define xstr(s) str(s)
#define str(s) #s

char c_compiler[1024];

static const int alignmentTable[] = {
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  };

char *stack_base;



#ifdef DEFINE_MATHERR_FUNCTION
int _matherr (struct _exception *a)

  { /* _matherr */
    a->retval = a->retval;
    return 1;
  } /* _matherr */
#endif



void prepareCompileCommand (void)

  {
    int pos;
    int quote_command = 0;
    int len;

  /* prepareCompileCommand */
    strcpy(c_compiler, C_COMPILER);
#ifdef MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
    if (c_compiler[0] == '/') {
      c_compiler[0] = c_compiler[1];
      c_compiler[1] = ':';
    } /* if */
#endif
    for (pos = 0; c_compiler[pos] != '\0'; pos++) {
      if (c_compiler[pos] == '/') {
        c_compiler[pos] = PATH_DELIMITER;
      } else if (c_compiler[pos] == ' ') {
        quote_command = 1;
      } /* if */
    } /* for */
    if (quote_command) {
      len = strlen(c_compiler);
      memmove(&c_compiler[1], c_compiler, len);
      c_compiler[0] = '\"';
      c_compiler[len + 1] = '\"';
      c_compiler[len + 2] = '\0';
    } /* if */
  } /* prepareCompileCommand */



void cleanUpCompilation (void)

  {
    struct stat stat_buf;
    char fileName[1024];

  /* cleanUpCompilation */
    if (stat("ctest.c", &stat_buf) == 0 && S_ISREG(stat_buf.st_mode)) {
      remove("ctest.c");
    } /* if */
    if (stat("ctest.cerrs", &stat_buf) == 0 && S_ISREG(stat_buf.st_mode)) {
      remove("ctest.cerrs");
    } /* if */
    sprintf(fileName, "ctest%s", OBJECT_FILE_EXTENSION);
    if (stat(fileName, &stat_buf) == 0 && S_ISREG(stat_buf.st_mode)) {
      remove(fileName);
    } /* if */
    sprintf(fileName, "ctest%s", EXECUTABLE_FILE_EXTENSION);
    if (stat(fileName, &stat_buf) == 0 && S_ISREG(stat_buf.st_mode)) {
      remove(fileName);
    } /* if */
    if (stat("ctest.out", &stat_buf) == 0 && S_ISREG(stat_buf.st_mode)) {
      remove("ctest.out");
    } /* if */
  } /* cleanUpCompilation */



int compilationOkay (const char *content)

  {
    FILE *testFile;
    char command[1024];
    int len;
    struct stat stat_buf;
    char fileName[1024];
    int returncode;
    int result = 0;

  /* compilationOkay */
    /* printf("/\* %s *\/\n", content); */
    cleanUpCompilation();
    testFile = fopen("ctest.c", "w");
    if (testFile != NULL) {
      fprintf(testFile, "%s", content);
      fclose(testFile);
#ifdef CC_FLAGS
      sprintf(command, "%s %s ctest.c", c_compiler, CC_FLAGS);
#else
      sprintf(command, "%s ctest.c", c_compiler);
#endif
#if defined LINKER_OPT_OUTPUT_FILE && !defined CC_NO_OPT_OUTPUT_FILE
      sprintf(&command[strlen(command)], " %sctest%s",
              LINKER_OPT_OUTPUT_FILE, EXECUTABLE_FILE_EXTENSION);
#endif
#ifdef REDIRECT_C_ERRORS
      sprintf(&command[strlen(command)], " %sctest.cerrs",
              REDIRECT_C_ERRORS);
#endif
#ifdef QUOTE_WHOLE_SHELL_COMMAND
      if (command[0] == '\"') {
        len = strlen(command);
        memmove(&command[1], command, len);
        command[0] = '\"';
        command[len + 1] = '\"';
        command[len + 2] = '\0';
      } /* if */
#endif
      returncode = system(command);
      sprintf(fileName, "ctest%s", EXECUTABLE_FILE_EXTENSION);
      if (stat(fileName, &stat_buf) == 0 && S_ISREG(stat_buf.st_mode)) {
        if (returncode == 0) {
          result = 1;
        } else {
          puts("#define CC_FAILS_BUT_CREATES_EXECUTABLE");
        } /* if */
      } /* if */
#ifdef DEBUG_CHKCCOMP
      printf("/* command: %s */\n", command);
      printf("/* content: %s */\n", content);
      printf("/* returncode: %d */\n", returncode);
      printf("/* result: %d */\n", result);
#endif
    } /* if */
    return result;
  } /* compilationOkay */



int doTest (void)

  {
    char command[1024];
    FILE *outFile;
    int result = -1;

  /* doTest */
    sprintf(command, ".%cctest%s>ctest.out", PATH_DELIMITER, EXECUTABLE_FILE_EXTENSION);
    if (system(command) != -1 && (outFile = fopen("ctest.out", "r")) != NULL) {
      fscanf(outFile, "%d", &result);
      fclose(outFile);
    } /* if */
    return result;
  } /* doTest */



/**
 *  Determine if DEFINE_OS_ENVIRON or INITIALIZE_OS_ENVIRON must be defined.
 */
void determineEnvironDefines (void)

  {
    char buffer[4096];
    int define_os_environ = 0;

  /* determineEnvironDefines */
    buffer[0] = '\0';
    if (compilationOkay("#include <stdlib.h>\n#include \"version.h\"\nint main(int argc,char *argv[])"
                        "{os_environ;return 0;}\n")) {
      strcat(buffer, "#include <stdlib.h>\n");
    } else if (compilationOkay("#include <unistd.h>\n#include \"version.h\"\nint main(int argc,char *argv[])"
                               "{os_environ;return 0;}\n")) {
      strcat(buffer, "#include <unistd.h>\n");
    } else {
      printf("#define DEFINE_OS_ENVIRON\n");
      define_os_environ = 1;
    } /* if */
    strcat(buffer, "#include <stdio.h>\n");
    strcat(buffer, "#include \"version.h\"\n");
#ifdef OS_STRI_WCHAR
    strcat(buffer, "typedef wchar_t *os_stritype;\n");
#else
    strcat(buffer, "typedef char *os_stritype;\n");
#endif
    if (define_os_environ) {
      strcat(buffer, "extern os_stritype *os_environ;\n");
    } /* if */
#ifdef USE_WMAIN
    strcat(buffer, "int wmain(int argc,wchar_t *argv[])");
#else
    strcat(buffer, "int main(int argc,char *argv[])");
#endif
    strcat(buffer, "{printf(\"%d\\n\",os_environ==(os_stritype *)0);return 0;}\n");
    if (!compilationOkay(buffer) || doTest() == 1) {
      printf("#define INITIALIZE_OS_ENVIRON\n");
    } /* if */
  } /* determineEnvironDefines */



void determineMallocAlignment (void)

  {
    int count;
    unsigned long malloc_result;
    int alignment;
    int minAlignment = 7;

  /* determineMallocAlignment */
    for (count = 1; count <= 64; count++) {
      malloc_result = (unsigned long) malloc(count);
      alignment = alignmentTable[malloc_result & 0x3f];
      if (alignment < minAlignment) {
        minAlignment = alignment;
      } /* if */
    } /* for */
    printf("#define MALLOC_ALIGNMENT %d\n", minAlignment);
  } /* determineMallocAlignment */



void detemineStackDirection (void)

  {
    char aVariable;

  /* detemineStackDirection */
    if (stack_base < &aVariable) {
      puts("#define STACK_GROWS_UPWARD");
    } else {
      puts("#define STACK_GROWS_DOWNWARD");
    } /* if */
  } /* detemineStackDirection */



/**
 *  Program to Check properties of C compiler and runtime.
 */
int main (int argc, char **argv)

  {
    char aVariable;
    FILE *aFile;
    time_t timestamp;
    struct tm *local_time;
    char buffer[4096];
    long number;
    int ch;
    union {
      char           charvalue;
      unsigned long  genericvalue;
    } testUnion;
    int sigbus_signal_defined = 0;
    int zero_divide_triggers_signal = 0;
    float zero = 0.0;
    float negativeZero;
    float minusZero;
    float nanValue1;
    float nanValue2;
    float plusInf;
    float minusInf;
    int testResult;
    const char *define_read_buffer_empty;

  /* main */
    prepareCompileCommand();
#ifdef WRITE_CC_VERSION_INFO
    WRITE_CC_VERSION_INFO
#endif
    aFile = fopen("cc_vers.txt","r");
    if (aFile != NULL) {
      printf("#define C_COMPILER_VERSION \"");
      for (ch=getc(aFile); ch!=EOF && ch!=10 && ch!=13; ch=getc(aFile)) {
        if (ch>=' ' && ch<='~') {
          if (ch=='\"' || ch=='\'' || ch=='\\') {
            putchar('\\');
          } /* if */
          putchar(ch);
        } else {
          putchar('\\');
          printf("%3o", ch);
        } /* if */
      } /* for */
      puts("\"");
      fclose(aFile);
    } /* if */
    if (compilationOkay("#include <unistd.h>\nint main(int argc,char *argv[]){return 0;}\n")) {
      puts("#define UNISTD_H_PRESENT");
    } /* if */
    if (!compilationOkay("static inline int test(int a){return 2*a;}\n"
                        "int main(int argc,char *argv[]){return test(argc);}\n")) {
      puts("#define inline");
    } /* if */
    if (compilationOkay("#include <stdio.h>\nint main(int argc,char *argv[])\n"
                        "{if(__builtin_expect(1,1))puts(\"1\");else puts(\"0\");return 0;}\n")) {
      puts("#define likely(x)   __builtin_expect((x),1)");
      puts("#define unlikely(x) __builtin_expect((x),0)");
    } /* if */
    if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[])\n"
                        "{FILE *aFile; aFile=popen(\""
                        LIST_DIRECTORY_CONTENTS
                        "\", \"r\");\n"
                        "printf(\"%d\\n\", ftell(aFile) != -1); return 0;}\n") ||
        compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[])\n"
                        "{FILE *aFile; aFile=_popen(\""
                        LIST_DIRECTORY_CONTENTS
                        "\", \"r\");\n"
                        "printf(\"%d\\n\", ftell(aFile) != -1); return 0;}\n")) {
      if (doTest() == 1) {
        puts("#define FTELL_WRONG_FOR_PIPE");
      } /* if */
    } else {
      puts("#define POPEN_MISSING");
    } /* if */
    if ((aFile = fopen("tmp_test_file","w")) != NULL) {
      fwrite("asdf",1,4,aFile);
      fclose(aFile);
      if ((aFile = fopen("tmp_test_file","r")) != NULL) {
        if (fwrite("qwert",1,5,aFile) != 0) {
          puts("#define FWRITE_WRONG_FOR_READ_ONLY_FILES");
        } /* if */
        fclose(aFile);
      } /* if */
      remove("tmp_test_file");
    } /* if */
    mkdir("tmp_empty_dir",0x755);
    if (compilationOkay(
        "#include <stdio.h>\n#include <utime.h>\n#include <errno.h>\nint main(int argc,char *argv[])"
        "{struct utimbuf utime_buf;\n"
        "utime_buf.actime=1234567890;utime_buf.modtime=1234567890;\n"
        "printf(\"%d\\n\",utime(\"tmp_empty_dir\",&utime_buf)!=0&&errno==EACCES);return 0;}\n") &&
        doTest() == 1) {
      puts("#define USE_ALTERNATE_UTIME");
#ifdef os_utime
      printf("#define os_utime_orig %s\n", xstr(os_utime));
      puts("#undef os_utime");
#else
      puts("#define os_utime_orig utime");
#endif
      puts("#define os_utime alternate_utime");
    } else if (compilationOkay(
        "#include <stdio.h>\n#include <sys/utime.h>\n#include <errno.h>\nint main(int argc,char *argv[])"
        "{struct utimbuf utime_buf;\n"
        "utime_buf.actime=1234567890;utime_buf.modtime=1234567890;\n"
        "printf(\"%d\\n\",utime(\"tmp_empty_dir\",&utime_buf)!=0&&errno==EACCES);return 0;}\n") &&
        doTest() == 1) {
      puts("#define INCLUDE_SYS_UTIME");
      puts("#define USE_ALTERNATE_UTIME");
#ifdef os_utime
      printf("#define os_utime_orig %s\n", xstr(os_utime));
      puts("#undef os_utime");
#else
      puts("#define os_utime_orig utime");
#endif
      puts("#define os_utime alternate_utime");
    } /* if */
    if (remove("tmp_empty_dir") != 0) {
      puts("#define REMOVE_FAILS_FOR_EMPTY_DIRS");
      rmdir("tmp_empty_dir");
    } /* if */
    aFile = fopen(".","r");
    if (aFile != NULL) {
      puts("#define FOPEN_OPENS_DIRECTORIES");
      fclose(aFile);
    } /* if */
    printf("#define SHORT_SIZE %lu\n",    (long unsigned) (8 * sizeof(short)));
    printf("#define INT_SIZE %lu\n",      (long unsigned) (8 * sizeof(int)));
    printf("#define LONG_SIZE %lu\n",     (long unsigned) (8 * sizeof(long)));
    printf("#define POINTER_SIZE %lu\n",  (long unsigned) (8 * sizeof(char *)));
    printf("#define FLOAT_SIZE %lu\n",    (long unsigned) (8 * sizeof(float)));
    printf("#define DOUBLE_SIZE %lu\n",   (long unsigned) (8 * sizeof(double)));
    printf("#define OS_OFF_T_SIZE %lu\n", (long unsigned) (8 * sizeof(os_off_t)));
    printf("#define TIME_T_SIZE %lu\n",   (long unsigned) (8 * sizeof(time_t)));
    timestamp = -2147483647 - 1;
    local_time = localtime(&timestamp);
    if (local_time != NULL && local_time->tm_year == 1) {
      puts("#define TIME_T_SIGNED");
    } /* if */
    if (sizeof(int) == 4) {
      puts("#define INT32TYPE int");
      puts("#define INT32TYPE_STRI \"int\"");
      puts("#define UINT32TYPE unsigned int");
      puts("#define UINT32TYPE_STRI \"unsigned int\"");
    } else if (sizeof(long) == 4) {
      puts("#define INT32TYPE long");
      puts("#define INT32TYPE_STRI \"long\"");
      puts("#define UINT32TYPE unsigned long");
      puts("#define UINT32TYPE_STRI \"unsigned long\"");
      puts("#define INT32TYPE_SUFFIX_L");
      puts("#define INT32TYPE_FORMAT_L");
    } /* if */
    if (sizeof(long) == 8) {
      puts("#define INT64TYPE long");
      puts("#define INT64TYPE_STRI \"long\"");
      puts("#define UINT64TYPE unsigned long");
      puts("#define UINT64TYPE_STRI \"unsigned long\"");
      puts("#define INT64TYPE_SUFFIX_L");
      puts("#define INT64TYPE_FORMAT_L");
    } else if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[])"
                               "{printf(\"%d\\n\",sizeof(long long));return 0;}\n") && doTest() == 8) {
      /* The type long long is defined and it is a 64-bit type */
      puts("#define INT64TYPE long long");
      puts("#define INT64TYPE_STRI \"long long\"");
      puts("#define UINT64TYPE unsigned long long");
      puts("#define UINT64TYPE_STRI \"unsigned long long\"");
      if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[]){long long n=12345678LL;return 0;}\n")) {
        puts("#define INT64TYPE_SUFFIX_LL");
      } /* if */
      if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[])\n"
                          "{char b[99]; sprintf(b, \"A%lldB\", (long long) 1 << 32);\n"
                          "printf(\"%d\\n\", strcmp(b,\"A4294967296B\")==0);return 0;}\n") && doTest() == 1) {
        puts("#define INT64TYPE_FORMAT_LL");
      } else if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[])\n"
                                 "{char b[99]; sprintf(b, \"A%LdB\", (long long) 1 << 32);\n"
                                 "printf(\"%d\\n\", strcmp(b,\"A4294967296B\")==0);return 0;}\n") && doTest() == 1) {
        puts("#define INT64TYPE_FORMAT_CAPITAL_L");
      } else if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[])\n"
                                 "{char b[99]; sprintf(b, \"A%I64dB\", (long long) 1 << 32);\n"
                                 "printf(\"%d\\n\", strcmp(b,\"A4294967296B\")==0);return 0;}\n") && doTest() == 1) {
        puts("#define INT64TYPE_FORMAT_I64");
      } /* if */
    } else if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[])\n"
                               "{printf(\"%d\\n\",sizeof(__int64));return 0;}\n") && doTest() == 8) {
      /* The type __int64 is defined and it is a 64-bit type */
      puts("#define INT64TYPE __int64");
      puts("#define INT64TYPE_STRI \"__int64\"");
      puts("#define UINT64TYPE unsigned __int64");
      puts("#define UINT64TYPE_STRI \"unsigned __int64\"");
      if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[]){__int64 n=12345678LL;return 0;}\n")) {
        puts("#define INT64TYPE_SUFFIX_LL");
      } /* if */
      if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[])\n"
                          "{char b[99]; sprintf(b, \"A%lldB\", (__int64) 1 << 32);\n"
                          "printf(\"%d\\n\", strcmp(b,\"A4294967296B\")==0);return 0;}\n") && doTest() == 1) {
        puts("#define INT64TYPE_FORMAT_LL");
      } else if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[])\n"
                                 "{char b[99]; sprintf(b, \"A%LdB\", (__int64) 1 << 32);\n"
                                 "printf(\"%d\\n\", strcmp(b,\"A4294967296B\")==0);return 0;}\n") && doTest() == 1) {
        puts("#define INT64TYPE_FORMAT_CAPITAL_L");
      } else if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[])\n"
                                 "{char b[99]; sprintf(b, \"A%I64dB\", (__int64) 1 << 32);\n"
                                 "printf(\"%d\\n\", strcmp(b,\"A4294967296B\")==0);return 0;}\n") && doTest() == 1) {
        puts("#define INT64TYPE_FORMAT_I64");
      } /* if */
    } /* if */
    number = -1;
    if (number >> 1 == (long) -1) {
      puts("#define RSHIFT_DOES_SIGN_EXTEND");
    } /* if */
    if (~number == (long) 0) {
      puts("#define TWOS_COMPLEMENT_INTTYPE");
    } else if (~number == (long) 1) {
      puts("#define ONES_COMPLEMENT_INTTYPE");
    } /* if */
    number = 1;
    if (((char *) &number)[0] == 1) {
      puts("#define LITTLE_ENDIAN_INTTYPE");
    } else {
      puts("#define BIG_ENDIAN_INTTYPE");
    } /* if */
    determineMallocAlignment();
    if (compilationOkay("#include<signal.h>\nint main(int argc, char *argv[]){\n"
                        "signal(SIGBUS,SIG_DFL); return 0;}\n")) {
      if (compilationOkay("#include<stdlib.h>\n#include <stdio.h>\n#include<signal.h>\n"
                          "void handleSig(int sig){puts(\"2\");exit(0);}\n"
                          "int main(int argc, char *argv[]){\n"
                          "signal(SIGBUS,handleSig);\n"
                          "int p[3]={12,34,56}, q, *pp; pp=(int *)((char *)&p[1]+1); q=*pp;\n"
                          "printf(\"1\\n\"); return 0;}\n") && doTest() == 1) {
        puts("#define UNALIGNED_MEMORY_ACCESS_OKAY");
      } else {
        puts("#define UNALIGNED_MEMORY_ACCESS_FAILS");
      } /* if */
    } else {
      if (compilationOkay("#include <stdio.h>\nint main(int argc, char *argv[])\n"
                          "{int p[3]={12,34,56}, q, *pp; pp=(int *)((char *)&p[1]+1); q=*pp;\n"
                          "printf(\"1\\n\"); return 0;}\n") && doTest() == 1) {
        puts("#define UNALIGNED_MEMORY_ACCESS_OKAY");
      } else {
        puts("#define UNALIGNED_MEMORY_ACCESS_FAILS");
      } /* if */
    } /* if */
    memset(&testUnion, 0, sizeof(testUnion));
    testUnion.charvalue = 'X';
    if (testUnion.charvalue != (char) testUnion.genericvalue) {
      puts("#define CASTING_DOES_NOT_GET_A_UNION_ELEMENT");
    } /* if */
    stack_base = &aVariable;
    detemineStackDirection();
#ifdef INT_DIV_BY_ZERO_POPUP
    puts("#define CHECK_INT_DIV_BY_ZERO");
#else
    if (!compilationOkay("#include<stdio.h>\nint main(int argc,char *argv[]){printf(\"%d\", 1/0);return 0;}\n")) {
      puts("#define CHECK_INT_DIV_BY_ZERO");
    } else if (compilationOkay("#include<stdlib.h>\n#include<stdio.h>\n#include<signal.h>\n"
                               "void handleSig(int sig){puts(\"2\");exit(0);}\n"
                               "int main(int argc,char *argv[]){\n"
                               "signal(SIGFPE,handleSig);\n"
                               "printf(\"%d\\n\",1/0==0);return 0;}\n") && doTest() == 2 &&
               compilationOkay("#include<stdlib.h>\n#include<stdio.h>\n#include<signal.h>\n"
                               "void handleSig(int sig){puts(\"2\");exit(0);}\n"
                               "int main(int argc,char *argv[]){\n"
                               "int zero=0;\n"
                               "signal(SIGFPE,handleSig);\n"
                               "printf(\"%d\\n\",1/zero==0);return 0;}\n") && doTest() == 2) {
      puts("#define INT_DIV_BY_ZERO_SIGNALS");
#ifndef DO_SIGFPE_WITH_DIV_BY_ZERO
      puts("#define DO_SIGFPE_WITH_DIV_BY_ZERO");
#endif
    } else {
      puts("#define CHECK_INT_DIV_BY_ZERO");
    } /* if */
#endif
#ifdef TURN_OFF_FP_EXCEPTIONS
    _control87(MCW_EM, MCW_EM);
#endif
    sprintf(buffer, "%1.0f %1.0f %1.0f %1.1f %1.1f %1.2f %1.2f %1.0f %1.0f %1.0f %1.1f %1.1f %1.2f %1.2f",
            0.5, 1.5, 2.5, 1.25, 1.75, 0.125, 0.375, -0.5, -1.5, -2.5, -1.25, -1.75, -0.125, -0.375);
    if (strcmp(buffer, "0 2 2 1.2 1.8 0.12 0.38 0 -2 -2 -1.2 -1.8 -0.12 -0.38") == 0 ||
        strcmp(buffer, "0 2 2 1.2 1.8 0.12 0.38 -0 -2 -2 -1.2 -1.8 -0.12 -0.38") == 0) {
      puts("#define ROUND_HALF_TO_EVEN");
    } else if (strcmp(buffer, "1 2 3 1.3 1.8 0.13 0.38 -1 -2 -3 -1.3 -1.8 -0.13 -0.38") == 0) {
      puts("#define ROUND_HALF_AWAY_FROM_ZERO");
    } else if (strcmp(buffer, "1 2 3 1.3 1.8 0.13 0.38 0 -1 -2 -1.2 -1.7 -0.12 -0.37") == 0 ||
               strcmp(buffer, "1 2 3 1.3 1.8 0.13 0.38 -0 -1 -2 -1.2 -1.7 -0.12 -0.37") == 0) {
      puts("#define ROUND_HALF_UP");
    } /* if */
    if (!compilationOkay("#include<stdio.h>\nint main(int argc,char *argv[]){printf(\"%f\", 1.0/0.0);return 0;}\n") ||
        !compilationOkay("#include<stdlib.h>\n#include<stdio.h>\n#include<float.h>\n#include<signal.h>\n"
                         "void handleSig(int sig){puts(\"2\");exit(0);}\n"
                         "int main(int argc,char *argv[]){\n"
#ifdef TURN_OFF_FP_EXCEPTIONS
                         "_control87(MCW_EM, MCW_EM);\n"
#endif
                         "signal(SIGFPE,handleSig);\nsignal(SIGILL,handleSig);\nsignal(SIGINT,handleSig);\n"
                         "printf(\"%d\\n\",1.0/0.0==0.0);return 0;}\n") || doTest() == 2) {
      puts("#define FLOAT_ZERO_DIV_ERROR");
    } /* if */
    if (!compilationOkay("#include<stdlib.h>\n#include<stdio.h>\n#include<float.h>\n#include<signal.h>\n"
                         "void handleSig(int sig){puts(\"2\");exit(0);}\n"
                         "int main(int argc,char *argv[]){\n"
                         "float zero=0.0;\n"
#ifdef TURN_OFF_FP_EXCEPTIONS
                         "_control87(MCW_EM, MCW_EM);\n"
#endif
                         "signal(SIGFPE,handleSig);\nsignal(SIGILL,handleSig);\nsignal(SIGINT,handleSig);\n"
                         "printf(\"%d\\n\",1.0/zero==0.0);return 0;}\n") || doTest() == 2) {
      puts("#define CHECK_FLOAT_DIV_BY_ZERO");
      zero_divide_triggers_signal = 1;
      if (sizeof(float) == sizeof(int)) {
        union {
          unsigned int i;
          float f;
        } transfer;
        transfer.i = 0xffc00000;
        nanValue1 = transfer.f;
        transfer.i = 0x7f800000;
        plusInf = transfer.f;
        transfer.i = 0xff800000;
        minusInf = transfer.f;
        transfer.i = 0x80000000;
        negativeZero = transfer.f;
      } else if (sizeof(float) == sizeof(long)) {
        union {
          unsigned long i;
          float f;
        } transfer;
        transfer.i = 0xffc00000;
        nanValue1 = transfer.f;
        transfer.i = 0x7f800000;
        plusInf = transfer.f;
        transfer.i = 0xff800000;
        minusInf = transfer.f;
        transfer.i = 0x80000000;
        negativeZero = transfer.f;
      } /* if */
      nanValue2 = nanValue1;
    } else {
      nanValue1 = 0.0 / zero;
      nanValue2 = 0.0 / zero;
      plusInf = 1.0 / zero;
      minusInf = -plusInf;
      negativeZero = -1.0 / plusInf;
      if (plusInf == minusInf || -1.0 / zero != minusInf) {
        puts("#define CHECK_FLOAT_DIV_BY_ZERO");
      } /* if */
    } /* if */
    if (nanValue1 == nanValue2 ||
        nanValue1 <  nanValue2 || nanValue1 >  nanValue2 ||
        nanValue1 <= nanValue2 || nanValue1 <= nanValue2) {
      puts("#define NAN_COMPARISON_WRONG");
    } /* if */
    minusZero = -zero;
    if (zero_divide_triggers_signal ||
        memcmp(&negativeZero, &minusZero, sizeof(float)) != 0) {
      puts("#define USE_NEGATIVE_ZERO_BITPATTERN");
    } /* if */
    if (pow(zero, -2.0) != plusInf || pow(negativeZero, -1.0) != minusInf) {
      puts("#define POWER_OF_ZERO_WRONG");
    } /* if */
    if (!compilationOkay("#include<float.h>\n#include<math.h>\nint main(int argc,char *argv[])"
                         "{float f=0.0; isnan(f); return 0;}\n") &&
        compilationOkay("#include<float.h>\n#include<math.h>\nint main(int argc,char *argv[])"
                        "{float f=0.0; _isnan(f); return 0;}\n")) {
      puts("#define ISNAN_WITH_UNDERLINE");
    } /* if */
    if (compilationOkay("#include<stdlib.h>\n#include<stdio.h>\n#include<float.h>\n#include<signal.h>\n"
                        "void handleSig(int sig){puts(\"2\");exit(0);}\n"
                        "int main(int argc,char *argv[]){\n"
                        "float zero=1.0E37;\n"
#ifdef TURN_OFF_FP_EXCEPTIONS
                        "_control87(MCW_EM, MCW_EM);\n"
#endif
                        "signal(SIGFPE,handleSig);\nsignal(SIGILL,handleSig);\nsignal(SIGINT,handleSig);\n"
                        "printf(\"%d\\n\",(int) 1.0E37);return 0;}\n")) {
      testResult = doTest();
      if ((sizeof(int) == 4 && (long) testResult == 2147483647L) ||
          (sizeof(int) == 2 && testResult == 32767)) {
        puts("#define FLOAT_TO_INT_OVERFLOW_SATURATES");
      } else if (testResult == 2) {
        puts("#define FLOAT_TO_INT_OVERFLOW_SIGNALS");
      } else if (testResult == 0) {
        puts("#define FLOAT_TO_INT_OVERFLOW_ZERO");
      } else {
        printf("#define FLOAT_TO_INT_OVERFLOW_GARBAGE %d\n", testResult);
      } /* if */
    } /* if */
#ifdef USE_ALTERNATE_LOCALTIME_R
    puts("#define USE_LOCALTIME_R");
#else
    if (compilationOkay("#include<time.h>\nint main(int argc,char *argv[])"
                        "{time_t ts;struct tm res;struct tm*lt;lt=localtime_r(&ts,&res);return 0;}\n")) {
      puts("#define USE_LOCALTIME_R");
    } else if (compilationOkay("#include<time.h>\nint main(int argc,char *argv[])"
                               "{time_t ts;struct tm res;localtime_s(&res,&ts);return 0;}\n")) {
      puts("#define USE_LOCALTIME_S");
    } /* if */
#endif
    determineEnvironDefines();
    if (getenv("USERPROFILE") != NULL) {
      /* When USERPROFILE is defined then it is used, even when HOME is defined. */
      puts("#define HOME_DIR_ENV_VAR {'U', 'S', 'E', 'R', 'P', 'R', 'O', 'F', 'I', 'L', 'E', 0}");
    } else if (getenv("HOME") != NULL) {
      puts("#define HOME_DIR_ENV_VAR {'H', 'O', 'M', 'E', 0}");
#ifdef OS_PATH_HAS_DRIVE_LETTERS
    } else {
      puts("#define HOME_DIR_ENV_VAR {'H', 'O', 'M', 'E', 0}");
      puts("#define DEFAULT_HOME_DIR {'C', ':', '\\\\', 0}");
#endif
    } /* if */
    if (compilationOkay("#include<poll.h>\nint main(int argc,char *argv[])"
                        "{struct pollfd pollFd[1];poll(pollFd, 1, 0);return 0;}\n")) {
      puts("#define HAS_POLL");
    } /* if */
    if (compilationOkay("#include<stdio.h>\nint main(int argc,char *argv[]){FILE*fp;fp->_IO_read_ptr>=fp->_IO_read_end;return 0;}\n")) {
      define_read_buffer_empty = "#define read_buffer_empty(fp) ((fp)->_IO_read_ptr >= (fp)->_IO_read_end)";
    } else if (compilationOkay("#include<stdio.h>\nint main(int argc,char *argv[]){FILE*fp;fp->_cnt <= 0;return 0;}\n")) {
      define_read_buffer_empty = "#define read_buffer_empty(fp) ((fp)->_cnt <= 0)";
    } else if (compilationOkay("#include<stdio.h>\nint main(int argc,char *argv[]){FILE*fp;fp->__cnt <= 0;return 0;}\n")) {
      define_read_buffer_empty = "#define read_buffer_empty(fp) ((fp)->__cnt <= 0)";
    } else if (compilationOkay("#include<stdio.h>\nint main(int argc,char *argv[]){FILE*fp;fp->level <= 0;return 0;}\n")) {
      define_read_buffer_empty = "#define read_buffer_empty(fp) ((fp)->level <= 0)";
    } else if (compilationOkay("#include<stdio.h>\nint main(int argc,char *argv[]){FILE*fp;fp->_r <= 0;return 0;}\n")) {
      define_read_buffer_empty = "#define read_buffer_empty(fp) ((fp)->_r <= 0)";
    } else {
      define_read_buffer_empty = NULL;
    } /* if */
    if (define_read_buffer_empty != NULL) {
      strcpy(buffer, "#include<stdio.h>\n");
      strcat(buffer, define_read_buffer_empty);
      strcat(buffer, "\nint main(int argc,char *argv[])\n"
                     "{FILE*fp;fp=fopen(\"version.h\",\"r\");"
                     "if(fp==NULL||!read_buffer_empty(fp))puts(0);else{"
                     "getc(fp);printf(\"%d\\n\",read_buffer_empty(fp)?0:1);}return 0;}\n");
      if (!compilationOkay(buffer) || doTest() != 1) {
        define_read_buffer_empty = NULL;
      } /* if */
    } /* if */
    if (define_read_buffer_empty != NULL) {
      puts(define_read_buffer_empty);
    } /* if */
    cleanUpCompilation();
    return 0;
  } /* main */
