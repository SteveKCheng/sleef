#ifndef SLEEF_EXPORT_C99_H
#define SLEEF_EXPORT_C99_H

// The EXPORT_LIBM_FUNCTION macro re-exports one of Sleef's own
// functions as a replacement for a C99 math function from
// the standard C library.  This macro is activated if the
// "EXPORT_LIBM" feature is turned on in Sleef's CMake build 
// system; otherwise it expands to nothing.
//
// On Windows, when compiling Sleef into a DLL, a function 
// can simply be re-exported with its name from C99.
//
// For ELF shared objects compiled with GCC or Clang, in principle
// we could do the same thing.  But we are careful to export the
// C99 name as a versioned symbol, with our custom tag "SLEEF_3".
// Otherwise, if the same process loads both Sleef (as libsleeflibm.so)
// and the standard math library (libm.so), there may be confusion
// as to which implementation of a C99 math function to use. 
//
// Note that, on Unix, we do not interpose the standard C library,
// by tricks similar to LD_PRELOAD.  It is risky.  Instead, 
// client applications or shared objects have to opt in to using
// Sleef's implementation of the C99 math functions.  This can
// be done by linking in libsleeflibm.so first before libm.so
// when compiling the application or shared object.
//
// On Windows, of course, interposition is not possible.  Client
// code must link into the import library sleeflibm.lib explicitly.
//
// We would have preferred to export the C99 aliases in a separate
// .c source file, but unfortunately GCC does not allow that.  
// An alias to an existing function must be in the same translation 
// unit that the function is defined in.  Thus we include this 
// header in the relevant translation units, sleefdp.c and sleefsp.c,
// and invoke this macro for each of the C99 functions to export.
//
// The c99_fn argument to this macro is the name of the C99 function.
// The sleef_fn argument is Sleef's original name for the function.
//

#if !defined(DORENAME) || !defined(EXPORT_LIBM)

  #define EXPORT_LIBM_FUNCTION(c99_fn,sleef_fn)

#elif defined(_MSC_VER) && !defined(_M_IX86)

  #define EXPORT_LIBM_FUNCTION(c99_fn,sleef_fn) __pragma(comment(linker, "/export:" #c99_fn "=" #sleef_fn ))

#elif defined(_MSC_VER) && defined(_M_IX86)

  #define EXPORT_LIBM_FUNCTION(c99_fn,sleef_fn) __pragma(comment(linker, "/export:_" #c99_fn "=_" #sleef_fn ))

#elif defined(__GNUC__) && !defined(_WIN32)

  #define EXPORT_LIBM_FUNCTION(c99_fn,sleef_fn) \
    void c99_fn ## _override () __attribute__((alias(#sleef_fn))); \
    __asm__(".symver " # c99_fn "_override," #c99_fn "@@@SLEEF_3");

#else

  #error "EXPORT_LIBM feature is not supported in your current platform. "

#endif

#endif // header guard
