//   Copyright Naoki Shibata and contributors 2010 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "funcproto.h"

/**
 * Generate the name for a structure which wraps a pair
 * of SIMD packets.
 *
 * In VSX intrinsics, SIMD data types are like "vector float".
 * This function replaces the space characters with '_'.
 * 
 * The returned string comes from malloc; the caller is
 * responsible for freeing it.
 */
static char* getPairTypeName(const char* typeName)
{
  size_t len = strlen(typeName);
  char* ret = malloc(len + 20);
  if (ret == NULL)
  {
      fprintf(stderr, "Could not allocate memory!\n");
      abort();
  }

  char* p = ret;

  strcpy(ret, "Sleef_");
  p += 6;

  for(size_t i = 0; i < len; ++i)
    *p++ = (typeName[i] != ' ') ? typeName[i] : '_';

  strcpy(p, "_2");

  return ret;
}

/**
 * Generate the C prototype, to standard output, for 
 * one of Sleef's functions.
 *
 * @param atrPrefix     A Sleef prefix such as "cinz_".
 * @param fpLetter      Either 'd' for "double" or 'f' for "float".
 * @param vectorWidth   Number of elements in the SIMD packet to
 *                      encode into the name of the C function.
 * @param fpType        What floating-point SIMD packet type is being used.
 * @param fpPairType    The name of the structure to encapsulate
 *                      a pair of floating-point SIMD packets.
 * @param intType       What integer SIMD packet type is being used.
 * @param intPairType   The name of the structure to encapsulate
 *                      a pair of integer SIMD packets.
 * @param isaName       Name of the instructure set architecture to
 *                      embed into the name of the C function.
 * @param callConv      Attribute to specify the calling convention.
 */
static void outputFunctionDeclaration(funcSpec f, 
                                      const char* atrPrefix,
                                      const char fpLetter,
                                      const char* vectorWidth,
                                      const char* fpType, 
                                      const char* fpPairType,
                                      const char* intType,
                                      const char* intPairType,
                                      const char* isaName,
                                      const char* callConv)
{
  // No single-precision versions for these functions
  if (fpLetter == 'f' && (f.funcType == 3 || f.funcType == 4))
    return;

  const char* returnType = NULL;
  switch (f.funcType)
  {
    case 0: returnType = fpType; break;
    case 1: returnType = fpType; break;
    case 2: returnType = fpPairType; break;
    case 3: returnType = fpType; break;
    case 4: returnType = intType; break;
    case 5: returnType = fpType; break;
    case 6: returnType = fpPairType; break;
    case 7: returnType = "int"; break;
    case 8: returnType = "void*"; break;
  }

  const char* argument1Type = NULL;
  const char* argument2Type = NULL;
  const char* argument3Type = NULL;
  switch (f.funcType)
  {
    case 0: argument1Type = fpType; break;
    case 1: argument1Type = argument2Type = fpType; break;
    case 2: argument1Type = fpType; break;
    case 3: argument1Type = fpType; argument2Type = intType; break;
    case 4: argument1Type = fpType; break;
    case 5: argument1Type = argument2Type = argument3Type = fpType; break;
    case 6: argument1Type = fpType; break;
    case 7: argument1Type = "int"; break;
    case 8: argument1Type = "int"; break;
  }

  printf("IMPORT CONST %s Sleef_%s%s%c%s", 
       returnType, 
       atrPrefix != NULL ? atrPrefix : "",
       f.name, 
       fpLetter, 
       vectorWidth);

  if (f.ulp >= 0)
    printf("_u%02d", f.ulp);
  else if (isaName[0] != '\0')
    printf("_");
    
  printf("%s(", isaName);
  if (argument1Type != NULL) printf("%s", argument1Type);
  if (argument2Type != NULL) printf(", %s", argument2Type);
  if (argument3Type != NULL) printf(", %s", argument3Type);
  printf(")");

  // The two cases below should not use vector calling convention.
  // They do not have vector type as argument or return value.
  // Also, the corresponding definition (`getPtr` and `getInt`) in `sleefsimd*.c`
  // are not defined with `VECTOR_CC`.
  switch (f.funcType)
  {
    case 7:
    case 8: break;
    default:
      printf("%s", callConv);
  }

  printf(";\n");
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "Generate a header for renaming functions\n");
    fprintf(stderr, "Usage : %s <atr prefix> <DP width> <SP width> [<isa>]\n", argv[0]);
    fprintf(stderr, "\n");

    fprintf(stderr, "Generate a part of header for library functions\n");
    fprintf(stderr, "Usage : %s <atr prefix> <DP width> <SP width> <vdouble type> <vfloat type> <vint type> <vint2 type> <Macro to enable> [<isa>]\n", argv[0]);
    fprintf(stderr, "\n");

    exit(-1);
  }

  static const char *ulpSuffixStr[] = { "", "_u1", "_u05", "_u35", "_u15", "_u3500" };
  
  if (argc == 4 || argc == 5) {
    char *atrPrefix = strcmp(argv[1], "-") == 0 ? "" : argv[1];
    char *wdp = argv[2];
    char *wsp = argv[3];
    char *isaname = argc == 4 ? "" : argv[4];
    char *isaub = argc == 5 ? "_" : "";

    //

    printf("#ifndef DETERMINISTIC\n\n");

    for(int i=0;funcList[i].name != NULL;i++) {
      if (funcList[i].ulp >= 0) {
	printf("#define x%s%s Sleef_%s%sd%s_u%02d%s\n",
	       funcList[i].name, ulpSuffixStr[funcList[i].ulpSuffix],
	       "", funcList[i].name, wdp,
	       funcList[i].ulp, isaname);
	printf("#define y%s%s Sleef_%s%sd%s_u%02d%s\n",
	       funcList[i].name, ulpSuffixStr[funcList[i].ulpSuffix],
	       atrPrefix, funcList[i].name, wdp,
	       funcList[i].ulp, isaname);
      } else {
	printf("#define x%s Sleef_%s%sd%s%s%s\n",
	       funcList[i].name,
	       "", funcList[i].name, wdp, isaub, isaname);
	printf("#define y%s Sleef_%s%sd%s%s%s\n",
	       funcList[i].name,
	       atrPrefix, funcList[i].name, wdp, isaub, isaname);
      }
    }

    printf("\n");
  
    for(int i=0;funcList[i].name != NULL;i++) {
      if (funcList[i].ulp >= 0) {
	printf("#define x%sf%s Sleef_%s%sf%s_u%02d%s\n",
	       funcList[i].name, ulpSuffixStr[funcList[i].ulpSuffix],
	       "", funcList[i].name, wsp,
	       funcList[i].ulp, isaname);
	printf("#define y%sf%s Sleef_%s%sf%s_u%02d%s\n",
	       funcList[i].name, ulpSuffixStr[funcList[i].ulpSuffix],
	       atrPrefix, funcList[i].name, wsp,
	       funcList[i].ulp, isaname);
      } else {
	printf("#define x%sf Sleef_%s%sf%s%s%s\n",
	       funcList[i].name,
	       "", funcList[i].name, wsp, isaub, isaname);
	printf("#define y%sf Sleef_%s%sf%s%s%s\n",
	       funcList[i].name,
	       atrPrefix, funcList[i].name, wsp, isaub, isaname);
      }
    }

    //

    printf("\n#else //#ifndef DETERMINISTIC\n\n");

    for(int i=0;funcList[i].name != NULL;i++) {
      if (funcList[i].ulp >= 0) {
	printf("#define x%s%s Sleef_%s%sd%s_u%02d%s\n",
	       funcList[i].name, ulpSuffixStr[funcList[i].ulpSuffix],
	       atrPrefix, funcList[i].name, wdp,
	       funcList[i].ulp, isaname);
      } else {
	printf("#define x%s Sleef_%s%sd%s%s%s\n",
	       funcList[i].name,
	       atrPrefix, funcList[i].name, wdp, isaub, isaname);
      }
    }

    printf("\n");
  
    for(int i=0;funcList[i].name != NULL;i++) {
      if (funcList[i].ulp >= 0) {
	printf("#define x%sf%s Sleef_%s%sf%s_u%02d%s\n",
	       funcList[i].name, ulpSuffixStr[funcList[i].ulpSuffix],
	       atrPrefix, funcList[i].name, wsp,
	       funcList[i].ulp, isaname);
      } else {
	printf("#define x%sf Sleef_%s%sf%s%s%s\n",
	       funcList[i].name,
	       atrPrefix, funcList[i].name, wsp, isaub, isaname);
      }
    }

    printf("\n#endif // #ifndef DETERMINISTIC\n");
  }
  else {
    char *atrPrefix = strcmp(argv[1], "-") == 0 ? "" : argv[1];
    char *wdp = argv[2];
    char *wsp = argv[3];
    char *vdoublename = argv[4], *vdouble2Name = getPairTypeName(vdoublename);
    char *vfloatname = argv[5], *vfloat2Name = getPairTypeName(vfloatname);
    char *vintname = argv[6], *vint2Name = getPairTypeName(vintname);
    char *vint2name = argv[7];
    char *architecture = argv[8];
    char *isaname = argc == 10 ? argv[9] : "";
    char *isaub = argc == 10 ? "_" : "";

    if (strcmp(isaname, "sve") == 0)
      wdp = wsp = "x";

    char * vectorcc = "";
    #ifdef ENABLE_AAVPCS
    if (strcmp(isaname, "advsimd") == 0)
      vectorcc =" __attribute__((aarch64_vector_pcs))";
    #endif

    printf("#ifdef %s\n", architecture);

    if (strcmp(vdoublename, "-") != 0) {
      printf("\n");
      printf("#ifndef %s_DEFINED\n", vdouble2Name);
      if (strcmp(architecture, "__ARM_FEATURE_SVE") == 0) {
	    printf("typedef svfloat64x2_t %s;\n", vdouble2Name);
      } else {
	    printf("typedef struct {\n");
        printf("  %s x, y;\n", vdoublename);
        printf("} %s;\n", vdouble2Name);
      }
      printf("#define %s_DEFINED\n", vdouble2Name);
      printf("#endif\n");
      printf("\n");

      for(int i=0;funcList[i].name != NULL;i++) {
		outputFunctionDeclaration(funcList[i], NULL,
								  'd', wdp, 
								  vdoublename, vdouble2Name,
								  vintname, vint2Name,
								  isaname, vectorcc);
		outputFunctionDeclaration(funcList[i], atrPrefix,
								  'd', wdp,
								  vdoublename, vdouble2Name,
							      vintname, vint2Name,
								  isaname, vectorcc);
	  }

	  printf("\n");
	  printf("#ifndef %s_DEFINED\n", vfloat2Name);
      if (strcmp(architecture, "__ARM_FEATURE_SVE") == 0) {
        printf("typedef svfloat32x2_t %s;\n", vfloat2Name);
      } else {
        printf("typedef struct {\n");
        printf("  %s x, y;\n", vfloatname);
        printf("} %s;\n", vfloat2Name);
      }
      printf("#define %s_DEFINED\n", vfloat2Name);
      printf("#endif\n");
      printf("\n");

      for(int i=0;funcList[i].name != NULL;i++) {
		outputFunctionDeclaration(funcList[i], NULL,
								  'f', wsp,
								  vfloatname, vfloat2Name,
								  vintname, vint2Name,
								  isaname, vectorcc);
		outputFunctionDeclaration(funcList[i], atrPrefix,
								  'f', wsp,
								  vfloatname, vfloat2Name,
								  vintname, vint2Name,
								  isaname, vectorcc);
	  }
	}

    printf("#endif\n");

    free(vdouble2Name);
    free(vfloat2Name);
    free(vint2Name);
  }

  exit(0);
}
  
