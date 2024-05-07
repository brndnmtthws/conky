/*  This file is part of the Vc library. {{{
Copyright © 2011-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_AVX_DEBUG_H_
#define VC_AVX_DEBUG_H_

#ifndef NDEBUG
#include "vector.h"
#include <iostream>
#include <iomanip>
#endif

namespace Vc_VERSIONED_NAMESPACE
{
namespace AVX
{
template <typename T, typename U> struct AddType {
    const U &d;
};
template <typename T, typename U> AddType<T, U> addType(const U &x) { return {x}; }

#ifdef NDEBUG
class DebugStream
{
    public:
        DebugStream(const char *, const char *, int) {}
        template<typename T> inline DebugStream &operator<<(const T &) { return *this; }
};
#else
class DebugStream
{
    private:
        template<typename T, typename V> static void printVector(V _x)
        {
            enum { Size = sizeof(V) / sizeof(T) };
            union { V v; T m[Size]; } x = { _x };
            std::cerr << '[' << std::setprecision(24) << x.m[0];
            for (int i = 1; i < Size; ++i) {
                std::cerr << ", " << std::setprecision(24) << x.m[i];
            }
            std::cerr << ']';
        }
    public:
        DebugStream(const char *func, const char *file, int line)
        {
            std::cerr << "\033[1;40;33mDEBUG: " << file << ':' << line << ' ' << func << ' ';
        }

        template<typename T> DebugStream &operator<<(const T &x) { std::cerr << x; return *this; }

        template <typename T, typename U> DebugStream &operator<<(AddType<T, U> &&x)
        {
            printVector<T, U>(x.d);
            return *this;
        }
        DebugStream &operator<<(__m128 x) {
            printVector<float, __m128>(x);
            return *this;
        }
        DebugStream &operator<<(__m256 x) {
            printVector<float, __m256>(x);
            return *this;
        }
        DebugStream &operator<<(__m128d x) {
            printVector<double, __m128d>(x);
            return *this;
        }
        DebugStream &operator<<(__m256d x) {
            printVector<double, __m256d>(x);
            return *this;
        }
        DebugStream &operator<<(__m128i x) {
            printVector<unsigned int, __m128i>(x);
            return *this;
        }
        DebugStream &operator<<(__m256i x) {
            printVector<unsigned int, __m256i>(x);
            return *this;
        }

        ~DebugStream()
        {
            std::cerr << "\033[0m" << std::endl;
        }
};
#endif

#ifdef Vc_DEBUG
#undef Vc_DEBUG
#endif
#ifdef Vc_MSVC
#define Vc_DEBUG Vc::AVX::DebugStream(__FUNCSIG__, __FILE__, __LINE__)
#else
#define Vc_DEBUG Vc::AVX::DebugStream(__PRETTY_FUNCTION__, __FILE__, __LINE__)
#endif

}  // namespace AVX
}  // namespace Vc

#endif // VC_AVX_DEBUG_H_
