/*  This file is part of the Vc library. {{{
Copyright © 2009-2015 Matthias Kretz <kretz@kde.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the names of contributing organizations nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

}}}*/

#ifndef VC_GLOBAL_H_
#define VC_GLOBAL_H_

#include <cstdint>
#include "fwddecl.h"

#ifdef DOXYGEN

/**
 * \name Compiler Identification Macros
 * \ingroup Utilities
 */
//@{
/**
 * \ingroup Utilities
 * This macro is defined to a number identifying the ICC version if the current
 * translation unit is compiled with the Intel compiler.
 *
 * For any other compiler this macro is not defined.
 */
#define Vc_ICC __INTEL_COMPILER_BUILD_DATE
#undef Vc_ICC
/**
 * \ingroup Utilities
 * This macro is defined to a number identifying the Clang version if the current
 * translation unit is compiled with the Clang compiler.
 *
 * For any other compiler this macro is not defined.
 */
#define Vc_CLANG (__clang_major__ * 0x10000 + __clang_minor__ * 0x100 + __clang_patchlevel__)
#undef Vc_CLANG
/**
 * \ingroup Utilities
 * This macro is defined to a number identifying the Apple Clang version if the current
 * translation unit is compiled with the Apple Clang compiler.
 *
 * For any other compiler this macro is not defined.
 */
#define Vc_APPLECLANG (__clang_major__ * 0x10000 + __clang_minor__ * 0x100 + __clang_patchlevel__)
#undef Vc_APPLECLANG
/**
 * \ingroup Utilities
 * This macro is defined to a number identifying the GCC version if the current
 * translation unit is compiled with the GCC compiler.
 *
 * For any other compiler this macro is not defined.
 */
#define Vc_GCC (__GNUC__ * 0x10000 + __GNUC_MINOR__ * 0x100 + __GNUC_PATCHLEVEL__)
/**
 * \ingroup Utilities
 * This macro is defined to a number identifying the Microsoft Visual C++ version if
 * the current translation unit is compiled with the Visual C++ (MSVC) compiler.
 *
 * For any other compiler this macro is not defined.
 */
#define Vc_MSVC _MSC_FULL_VER
#undef Vc_MSVC
//@}

#else  // DOXYGEN

// Compiler defines
#ifdef __INTEL_COMPILER
#define Vc_ICC __INTEL_COMPILER_BUILD_DATE
#elif defined(__clang__) && defined(__apple_build_version__)
#define Vc_APPLECLANG (__clang_major__ * 0x10000 + __clang_minor__ * 0x100 + __clang_patchlevel__)
#elif defined(__clang__)
#define Vc_CLANG (__clang_major__ * 0x10000 + __clang_minor__ * 0x100 + __clang_patchlevel__)
#elif defined(__GNUC__)
#define Vc_GCC (__GNUC__ * 0x10000 + __GNUC_MINOR__ * 0x100 + __GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
#define Vc_MSVC _MSC_FULL_VER
#else
#define Vc_UNSUPPORTED_COMPILER 1
#endif

#if defined Vc_GCC && Vc_GCC >= 0x60000
#define Vc_RESET_DIAGNOSTICS _Pragma("GCC diagnostic pop")
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#else
#define Vc_RESET_DIAGNOSTICS
#endif

#if defined Vc_ICC
// 'warning #2922: template parameter "<unnamed>" cannot be used because it follows a
// parameter pack and cannot be deduced from the parameters of function template'
// This warning is stupid. The parameter is unnamed because I don't want to use it. I see
// no other workaround than to disable the warning. Sadly, it doesn't suffice to disable
// it for the Vc headers. It must also be disabled at the places Vc types are used.
#pragma warning disable 2922
#endif

#if __cplusplus < 201103 && (!defined Vc_MSVC || _MSC_VER < 1900)
# error "Vc requires support for C++11."
#elif __cplusplus >= 201402L
# define Vc_CXX14 1
# if __cplusplus > 201700L
#  define Vc_CXX17 1
# endif
#endif

#if defined(__GNUC__) && !defined(Vc_NO_INLINE_ASM)
#define Vc_GNU_ASM 1
#endif

#ifdef Vc_GCC
#  if Vc_GCC >= 0x70000 && defined __i386__
     // GCC 7 changed alignof(max_align_t) to 16. glibc 2.26 followed with malloc in 2.26.
     // 1. If GCC >= 7 and libc is not glibc max_align_t and malloc mismatch
     // 2. If GCC >= 7 and libc is glibc < 2.26 max_align_t and malloc mismatch
#    ifdef __GLIBC_PREREQ
#      if __GLIBC_PREREQ(2,26)
#        define Vc_HAVE_STD_MAX_ALIGN_T 1
#      endif
#    endif
#  elif Vc_GCC >= 0x40900
#    define Vc_HAVE_STD_MAX_ALIGN_T 1
#  else
#    define Vc_HAVE_MAX_ALIGN_T 1
#  endif
#elif !defined(Vc_CLANG) && !defined(Vc_ICC)
//   Clang/ICC don't provide max_align_t at all
//   TODO: Clang defines max_align_t since 3.5.0. Whether std::max_align_t is defined depends on the
//   standard library version.
#  define Vc_HAVE_STD_MAX_ALIGN_T 1
#endif

#if defined(Vc_GCC) || defined(Vc_CLANG) || defined Vc_APPLECLANG
#define Vc_USE_BUILTIN_VECTOR_TYPES 1
#endif

#ifdef Vc_MSVC
#  define Vc_CDECL __cdecl
#  define Vc_VDECL __vectorcall
#else
#  define Vc_CDECL
#  define Vc_VDECL
#endif

/* Define the following strings to a unique integer, which is the only type the preprocessor can
 * compare. This allows to use -DVc_IMPL=SSE3. The preprocessor will then consider Vc_IMPL and SSE3
 * to be equal. Of course, it is important to undefine the strings later on!
 */
#define Scalar 0x00100000
#define SSE    0x00200000
#define SSE2   0x00300000
#define SSE3   0x00400000
#define SSSE3  0x00500000
#define SSE4_1 0x00600000
#define SSE4_2 0x00700000
#define AVX    0x00800000
#define AVX2   0x00900000

#define XOP    0x00000001
#define FMA4   0x00000002
#define F16C   0x00000004
#define POPCNT 0x00000008
#define SSE4a  0x00000010
#define FMA    0x00000020
#define BMI2   0x00000040

#define IMPL_MASK 0xFFF00000
#define EXT_MASK  0x000FFFFF

#ifdef Vc_MSVC
# ifdef _M_IX86_FP
#  if _M_IX86_FP >= 1
#   ifndef __SSE__
#    define __SSE__ 1
#   endif
#  endif
#  if _M_IX86_FP >= 2
#   ifndef __SSE2__
#    define __SSE2__ 1
#   endif
#  endif
# elif defined(_M_AMD64)
// If the target is x86_64 then SSE2 is guaranteed
#  ifndef __SSE__
#   define __SSE__ 1
#  endif
#  ifndef __SSE2__
#   define __SSE2__ 1
#  endif
# endif
#endif

#if defined Vc_ICC && !defined __POPCNT__
# if defined __SSE4_2__ || defined __SSE4A__
#  define __POPCNT__ 1
# endif
#endif

#ifdef VC_IMPL
#error "You are using the old VC_IMPL macro. Since Vc 1.0 all Vc macros start with Vc_, i.e. a lower-case 'c'"
#endif

#ifndef Vc_IMPL

#  if defined(__AVX2__)
#    define Vc_IMPL_AVX2 1
#    define Vc_IMPL_AVX 1
#  elif defined(__AVX__)
#    define Vc_IMPL_AVX 1
#  else
#    if defined(__SSE4_2__)
#      define Vc_IMPL_SSE 1
#      define Vc_IMPL_SSE4_2 1
#    endif
#    if defined(__SSE4_1__)
#      define Vc_IMPL_SSE 1
#      define Vc_IMPL_SSE4_1 1
#    endif
#    if defined(__SSE3__)
#      define Vc_IMPL_SSE 1
#      define Vc_IMPL_SSE3 1
#    endif
#    if defined(__SSSE3__)
#      define Vc_IMPL_SSE 1
#      define Vc_IMPL_SSSE3 1
#    endif
#    if defined(__SSE2__)
#      define Vc_IMPL_SSE 1
#      define Vc_IMPL_SSE2 1
#    endif

#    if defined(Vc_IMPL_SSE)
       // nothing
#    else
#      define Vc_IMPL_Scalar 1
#    endif
#  endif
#  if !defined(Vc_IMPL_Scalar)
#    ifdef __FMA4__
#      define Vc_IMPL_FMA4 1
#    endif
#    ifdef __XOP__
#      define Vc_IMPL_XOP 1
#    endif
#    ifdef __F16C__
#      define Vc_IMPL_F16C 1
#    endif
#    ifdef __POPCNT__
#      define Vc_IMPL_POPCNT 1
#    endif
#    ifdef __SSE4A__
#      define Vc_IMPL_SSE4a 1
#    endif
#    ifdef __FMA__
#      define Vc_IMPL_FMA 1
#    endif
#    ifdef __BMI2__
#      define Vc_IMPL_BMI2 1
#    endif
#  endif

#else // Vc_IMPL

#  if (Vc_IMPL & IMPL_MASK) == AVX2 // AVX2 supersedes SSE
#    define Vc_IMPL_AVX2 1
#    define Vc_IMPL_AVX 1
#  elif (Vc_IMPL & IMPL_MASK) == AVX // AVX supersedes SSE
#    define Vc_IMPL_AVX 1
#  elif (Vc_IMPL & IMPL_MASK) == Scalar
#    define Vc_IMPL_Scalar 1
#  elif (Vc_IMPL & IMPL_MASK) == SSE4_2
#    define Vc_IMPL_SSE4_2 1
#    define Vc_IMPL_SSE4_1 1
#    define Vc_IMPL_SSSE3 1
#    define Vc_IMPL_SSE3 1
#    define Vc_IMPL_SSE2 1
#    define Vc_IMPL_SSE 1
#  elif (Vc_IMPL & IMPL_MASK) == SSE4_1
#    define Vc_IMPL_SSE4_1 1
#    define Vc_IMPL_SSSE3 1
#    define Vc_IMPL_SSE3 1
#    define Vc_IMPL_SSE2 1
#    define Vc_IMPL_SSE 1
#  elif (Vc_IMPL & IMPL_MASK) == SSSE3
#    define Vc_IMPL_SSSE3 1
#    define Vc_IMPL_SSE3 1
#    define Vc_IMPL_SSE2 1
#    define Vc_IMPL_SSE 1
#  elif (Vc_IMPL & IMPL_MASK) == SSE3
#    define Vc_IMPL_SSE3 1
#    define Vc_IMPL_SSE2 1
#    define Vc_IMPL_SSE 1
#  elif (Vc_IMPL & IMPL_MASK) == SSE2
#    define Vc_IMPL_SSE2 1
#    define Vc_IMPL_SSE 1
#  elif (Vc_IMPL & IMPL_MASK) == SSE
#    define Vc_IMPL_SSE 1
#    if defined(__SSE4_2__)
#      define Vc_IMPL_SSE4_2 1
#    endif
#    if defined(__SSE4_1__)
#      define Vc_IMPL_SSE4_1 1
#    endif
#    if defined(__SSE3__)
#      define Vc_IMPL_SSE3 1
#    endif
#    if defined(__SSSE3__)
#      define Vc_IMPL_SSSE3 1
#    endif
#    if defined(__SSE2__)
#      define Vc_IMPL_SSE2 1
#    endif
#  elif (Vc_IMPL & IMPL_MASK) == 0 && (Vc_IMPL & SSE4a)
     // this is for backward compatibility only where SSE4a was included in the main
     // line of available SIMD instruction sets
#    define Vc_IMPL_SSE3 1
#    define Vc_IMPL_SSE2 1
#    define Vc_IMPL_SSE 1
#  endif
#  if (Vc_IMPL & XOP)
#    define Vc_IMPL_XOP 1
#  endif
#  if (Vc_IMPL & FMA4)
#    define Vc_IMPL_FMA4 1
#  endif
#  if (Vc_IMPL & F16C)
#    define Vc_IMPL_F16C 1
#  endif
#  if (!defined(Vc_IMPL_Scalar) && defined(__POPCNT__)) || (Vc_IMPL & POPCNT)
#    define Vc_IMPL_POPCNT 1
#  endif
#  if (Vc_IMPL & SSE4a)
#    define Vc_IMPL_SSE4a 1
#  endif
#  if (Vc_IMPL & FMA)
#    define Vc_IMPL_FMA 1
#  endif
#  if (Vc_IMPL & BMI2)
#    define Vc_IMPL_BMI2 1
#  endif
#  undef Vc_IMPL

#endif // Vc_IMPL

// If AVX is enabled in the compiler it will use VEX coding for the SIMD instructions.
#ifdef __AVX__
#  define Vc_USE_VEX_CODING 1
#endif

#ifdef Vc_IMPL_AVX
// if we have AVX then we also have all SSE intrinsics
#    define Vc_IMPL_SSE4_2 1
#    define Vc_IMPL_SSE4_1 1
#    define Vc_IMPL_SSSE3 1
#    define Vc_IMPL_SSE3 1
#    define Vc_IMPL_SSE2 1
#    define Vc_IMPL_SSE 1
#endif

#if defined(Vc_CLANG) && Vc_CLANG >= 0x30600 && Vc_CLANG < 0x30700
#    if defined(Vc_IMPL_AVX)
#        warning "clang 3.6.x miscompiles AVX code, frequently losing 50% of the data. Vc will fall back to SSE4 instead."
#        undef Vc_IMPL_AVX
#        if defined(Vc_IMPL_AVX2)
#            undef Vc_IMPL_AVX2
#        endif
#    endif
#endif

# if !defined(Vc_IMPL_Scalar) && !defined(Vc_IMPL_SSE) && !defined(Vc_IMPL_AVX)
#  error "No suitable Vc implementation was selected! Probably Vc_IMPL was set to an invalid value."
# elif defined(Vc_IMPL_SSE) && !defined(Vc_IMPL_SSE2)
#  error "SSE requested but no SSE2 support. Vc needs at least SSE2!"
# endif

#undef Scalar
#undef SSE
#undef SSE2
#undef SSE3
#undef SSSE3
#undef SSE4_1
#undef SSE4_2
#undef AVX
#undef AVX2

#undef XOP
#undef FMA4
#undef F16C
#undef POPCNT
#undef SSE4a
#undef FMA
#undef BMI2

#undef IMPL_MASK
#undef EXT_MASK

#if defined Vc_IMPL_AVX2
#define Vc_DEFAULT_IMPL_AVX2
#elif defined Vc_IMPL_AVX
#define Vc_DEFAULT_IMPL_AVX
#elif defined Vc_IMPL_SSE
#define Vc_DEFAULT_IMPL_SSE
#elif defined Vc_IMPL_Scalar
#define Vc_DEFAULT_IMPL_Scalar
#else
#error "Preprocessor logic broken. Please report a bug."
#endif

#endif // DOXYGEN

namespace Vc_VERSIONED_NAMESPACE
{

typedef   signed char        int8_t;
typedef unsigned char       uint8_t;
typedef   signed short      int16_t;
typedef unsigned short     uint16_t;
typedef   signed int        int32_t;
typedef unsigned int       uint32_t;
typedef   signed long long  int64_t;
typedef unsigned long long uint64_t;

/**
 * \ingroup Utilities
 *
 * Enum that specifies the alignment and padding restrictions to use for memory allocation with
 * Vc::malloc.
 */
enum MallocAlignment {
    /**
     * Align on boundary of vector sizes (e.g. 16 Bytes on SSE platforms) and pad to allow
     * vector access to the end. Thus the allocated memory contains a multiple of
     * VectorAlignment bytes.
     */
    AlignOnVector,
    /**
     * Align on boundary of cache line sizes (e.g. 64 Bytes on x86) and pad to allow
     * full cache line access to the end. Thus the allocated memory contains a multiple of
     * 64 bytes.
     */
    AlignOnCacheline,
    /**
     * Align on boundary of page sizes (e.g. 4096 Bytes on x86) and pad to allow
     * full page access to the end. Thus the allocated memory contains a multiple of
     * 4096 bytes.
     */
    AlignOnPage
};

/**
 * \ingroup Utilities
 *
 * Enum to identify a certain SIMD instruction set.
 *
 * You can use \ref CurrentImplementation for the currently active implementation.
 *
 * \see ExtraInstructions
 */
enum Implementation : std::uint_least32_t { // TODO: make enum class
    /// uses only fundamental types
    ScalarImpl,
    /// x86 SSE + SSE2
    SSE2Impl,
    /// x86 SSE + SSE2 + SSE3
    SSE3Impl,
    /// x86 SSE + SSE2 + SSE3 + SSSE3
    SSSE3Impl,
    /// x86 SSE + SSE2 + SSE3 + SSSE3 + SSE4.1
    SSE41Impl,
    /// x86 SSE + SSE2 + SSE3 + SSSE3 + SSE4.1 + SSE4.2
    SSE42Impl,
    /// x86 AVX
    AVXImpl,
    /// x86 AVX + AVX2
    AVX2Impl,
    /// Intel Xeon Phi
    MICImpl,
    ImplementationMask = 0xfff
};

/**
 * \ingroup Utilities
 *
 * The list of available instructions is not easily described by a linear list of instruction sets.
 * On x86 the following instruction sets always include their predecessors:
 * SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2
 *
 * But there are additional instructions that are not necessarily required by this list. These are
 * covered in this enum.
 */
enum ExtraInstructions : std::uint_least32_t { // TODO: make enum class
    //! Support for float16 conversions in hardware
    Float16cInstructions  = 0x01000,
    //! Support for FMA4 instructions
    Fma4Instructions      = 0x02000,
    //! Support for XOP instructions
    XopInstructions       = 0x04000,
    //! Support for the population count instruction
    PopcntInstructions    = 0x08000,
    //! Support for SSE4a instructions
    Sse4aInstructions     = 0x10000,
    //! Support for FMA instructions (3 operand variant)
    FmaInstructions       = 0x20000,
    //! Support for ternary instruction coding (VEX)
    VexInstructions       = 0x40000,
    //! Support for BMI2 instructions
    Bmi2Instructions      = 0x80000,
    // PclmulqdqInstructions,
    // AesInstructions,
    // RdrandInstructions
    ExtraInstructionsMask = 0xfffff000u
};

/**
 * \ingroup Utilities
 * This class identifies the specific implementation %Vc uses in the current translation
 * unit in terms of a type.
 *
 * Most importantantly, the type \ref CurrentImplementation instantiates the class
 * template with the bitmask identifying the current implementation. The contents of the
 * bitmask can be queried with the static member functions of the class.
 */
template <unsigned int Features> struct ImplementationT {
    /// Returns the currently used Vc::Implementation.
    static constexpr Implementation current()
    {
        return static_cast<Implementation>(Features & ImplementationMask);
    }
    /// Returns whether \p impl is the current Vc::Implementation.
    static constexpr bool is(Implementation impl)
    {
        return static_cast<unsigned int>(impl) == current();
    }
    /**
     * Returns whether the current Vc::Implementation implements at least \p low and at
     * most \p high.
     */
    static constexpr bool is_between(Implementation low, Implementation high)
    {
        return static_cast<unsigned int>(low) <= current() &&
               static_cast<unsigned int>(high) >= current();
    }
    /**
     * Returns whether the current code would run on a CPU providing \p extraInstructions.
     */
    static constexpr bool runs_on(unsigned int extraInstructions)
    {
        return (extraInstructions & Features & ExtraInstructionsMask) ==
               (Features & ExtraInstructionsMask);
    }
};
/**
 * \ingroup Utilities
 * Identifies the %Vc implementation used in the current translation unit.
 *
 * \see ImplementationT
 */
using CurrentImplementation = ImplementationT<
#ifdef Vc_IMPL_Scalar
    ScalarImpl
#elif defined(Vc_IMPL_AVX2)
    AVX2Impl
#elif defined(Vc_IMPL_AVX)
    AVXImpl
#elif defined(Vc_IMPL_SSE4_2)
    SSE42Impl
#elif defined(Vc_IMPL_SSE4_1)
    SSE41Impl
#elif defined(Vc_IMPL_SSSE3)
    SSSE3Impl
#elif defined(Vc_IMPL_SSE3)
    SSE3Impl
#elif defined(Vc_IMPL_SSE2)
    SSE2Impl
#endif
#ifdef Vc_IMPL_SSE4a
    + Vc::Sse4aInstructions
#ifdef Vc_IMPL_XOP
    + Vc::XopInstructions
#ifdef Vc_IMPL_FMA4
    + Vc::Fma4Instructions
#endif
#endif
#endif
#ifdef Vc_IMPL_POPCNT
    + Vc::PopcntInstructions
#endif
#ifdef Vc_IMPL_FMA
    + Vc::FmaInstructions
#endif
#ifdef Vc_IMPL_BMI2
    + Vc::Bmi2Instructions
#endif
#ifdef Vc_USE_VEX_CODING
    + Vc::VexInstructions
#endif
    >;

}  // namespace Vc

#include "version.h"

#endif // VC_GLOBAL_H_

// vim: foldmethod=marker
