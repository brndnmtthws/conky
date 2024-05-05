/*  This file is part of the Vc library. {{{
Copyright © 2010-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_SUPPORT_H_
#define VC_COMMON_SUPPORT_H_

#ifndef VC_GLOBAL_H_
#error "Vc/global.h must be included first!"
#endif

#if defined __x86_64__ || defined __amd64__ || defined __amd64 || defined __x86_64 ||    \
    defined _M_AMD64 || defined __i386__
#include <Vc/cpuid.h>
#endif

#if defined(Vc_GCC) && Vc_GCC >= 0x40400 && defined __SSE__
#define Vc_TARGET_NO_SIMD __attribute__((target("no-sse2,no-avx")))
#else
#define Vc_TARGET_NO_SIMD
#endif

#include "common/macros.h"
namespace Vc_VERSIONED_NAMESPACE
{

/**
 * \name Micro-Architecture Feature Tests
 */
//@{
/**
 * \ingroup Utilities
 * \headerfile support.h <Vc/support.h>
 * Determines the extra instructions supported by the current CPU.
 *
 * \return A combination of flags from Vc::ExtraInstructions that the current CPU supports.
 */
Vc_TARGET_NO_SIMD
unsigned int extraInstructionsSupported();

/**
 * \ingroup Utilities
 * \headerfile support.h <Vc/support.h>
 *
 * Tests whether the given implementation is supported by the system the code is executing on.
 *
 * \return \c true if the OS and hardware support execution of instructions defined by \p impl.
 * \return \c false otherwise
 *
 * \param impl The SIMD target to test for.
 */
Vc_TARGET_NO_SIMD bool Vc_VDECL isImplementationSupported(Vc::Implementation impl);

/**
 * \internal
 * \ingroup Utilities
 * \headerfile support.h <Vc/support.h>
 *
 * Tests whether the given implementation is supported by the system the code is executing on.
 *
 * \code
 * if (!isImplementationSupported<Vc::CurrentImplementation>()) {
 *   std::cerr << "This code was compiled with features that this system does not support.\n";
 *   return EXIT_FAILURE;
 * }
 * \endcode
 *
 * \return \c true if the OS and hardware support execution of instructions defined by \p impl.
 * \return \c false otherwise
 *
 * \tparam Impl The SIMD target to test for.
 */
template<typename Impl>
Vc_TARGET_NO_SIMD
static inline bool isImplementationSupported()
{
    return isImplementationSupported(Impl::current()) &&
           Impl::runs_on(extraInstructionsSupported());
}

/**
 * \ingroup Utilities
 * \headerfile support.h <Vc/support.h>
 *
 * Determines the best supported implementation for the current system.
 *
 * \return The enum value for the best implementation.
 */
Vc_TARGET_NO_SIMD
Vc::Implementation bestImplementationSupported();

#ifndef Vc_COMPILE_LIB
/**
 * \ingroup Utilities
 * \headerfile support.h <Vc/support.h>
 *
 * Tests that the CPU and Operating System support the vector unit which was compiled for. This
 * function should be called before any other Vc functionality is used. It checks whether the program
 * will work. If this function returns \c false then the program should exit with a useful error
 * message before the OS has to kill it because of an invalid instruction exception.
 *
 * If the program continues and makes use of any vector features not supported by
 * hard- or software then the program will crash.
 *
 * Example:
 * \code
 * int main()
 * {
 *   if (!Vc::currentImplementationSupported()) {
 *     std::cerr << "CPU or OS requirements not met for the compiled in vector unit!\n";
 *     exit -1;
 *   }
 *   ...
 * }
 * \endcode
 *
 * \return \c true if the OS and hardware support execution of the currently selected SIMD
 *                 instructions.
 * \return \c false otherwise
 */
Vc_TARGET_NO_SIMD
#ifndef DOXYGEN
static
#endif
inline bool currentImplementationSupported()
{
    return isImplementationSupported<Vc::CurrentImplementation>();
}
#endif // Vc_COMPILE_LIB
//@}

}

#undef Vc_TARGET_NO_SIMD

#endif // VC_COMMON_SUPPORT_H_
