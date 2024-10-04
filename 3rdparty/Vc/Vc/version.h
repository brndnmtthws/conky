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

#ifndef VC_VERSION_H_
#define VC_VERSION_H_

/**
 * \name Version Macros
 * \ingroup Utilities
 */
//@{
/**
 * \ingroup Utilities
 * Contains the version string of the %Vc headers. Same as Vc::versionString().
 */
#define Vc_VERSION_STRING "1.4.4"

/**
 * \ingroup Utilities
 * Contains the encoded version number of the %Vc headers. Same as Vc::versionNumber().
 */
#define Vc_VERSION_NUMBER 0x010408

/**
 * \ingroup Utilities
 *
 * Helper macro to compare against an encoded version number.
 * Example:
 * \code
 * #if Vc_VERSION_NUMBER >= Vc_VERSION_CHECK(1, 0, 0)
 * \endcode
 */
#define Vc_VERSION_CHECK(major, minor, patch) ((major << 16) | (minor << 8) | (patch << 1))
//@}

#define Vc_LIBRARY_ABI_VERSION 5

///\internal identify Vc 2.0
#define Vc_IS_VERSION_2 (Vc_VERSION_NUMBER >= Vc_VERSION_CHECK(1, 70, 0))
///\internal identify Vc 1.x
#define Vc_IS_VERSION_1 (Vc_VERSION_NUMBER < Vc_VERSION_CHECK(1, 70, 0))

namespace Vc_VERSIONED_NAMESPACE
{
/**
 * \ingroup Utilities
 * \headerfile version.h <Vc/version.h>
 *
 * \returns the version string of the %Vc headers.
 *
 * \note There exists a built-in check that ensures on application startup that the %Vc version of the
 * library (link time) and the headers (compile time) are equal. A mismatch between headers and
 * library could lead to errors that are very hard to debug.
 * \note If you need to disable the check (it costs a very small amount of application startup time)
 * you can define Vc_NO_VERSION_CHECK at compile time.
 */
inline const char *versionString() { return Vc_VERSION_STRING; }

/**
 * \ingroup Utilities
 * \headerfile version.h <Vc/version.h>
 *
 * \returns the version of the %Vc headers encoded in an integer.
 */
constexpr unsigned int versionNumber() { return Vc_VERSION_NUMBER; }
}

// @BEGIN SKIP GODBOLT@
#if !defined(Vc_NO_VERSION_CHECK) && !defined(Vc_COMPILE_LIB)
namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{
/**\internal
 * This function is implemented in the libVc library and checks whether the library is
 * compatible with the version information passed via the function parameters. If it is
 * incompatible the function prints a warning and aborts.
 */
void Vc_CDECL checkLibraryAbi(unsigned int compileTimeAbi, unsigned int versionNumber,
                              const char *versionString);
namespace
{
/**\internal
 * This constructor function is compiled into every translation unit and thus executed on
 * startup (before main) for as many TUs as are linked into the executable. It calls
 * Vc::Common::checkLibraryAbi to ensure the TU was compiled with Vc headers that are
 * compatible to the linked libVc.
 */
static struct runLibraryAbiCheck {
    runLibraryAbiCheck()
    {
        checkLibraryAbi(Vc_LIBRARY_ABI_VERSION, Vc_VERSION_NUMBER, Vc_VERSION_STRING);
    }
} _runLibraryAbiCheck;
}  // unnamed namespace
}  // namespace Common
}  // namespace Vc
#endif
// @END SKIP GODBOLT@

#endif // VC_VERSION_H_
