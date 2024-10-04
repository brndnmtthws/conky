/*  This file is part of the Vc library. {{{
Copyright © 2013-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_MALLOC_H_
#define VC_COMMON_MALLOC_H_

#ifndef Vc_VECTOR_DECLARED_
#error "Incorrect inclusion order. This header must be included from Vc/vector.h only."
#endif

#if defined _WIN32 || defined _WIN64
#include <malloc.h>
#else
#include <cstdlib>
#endif

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{

template <size_t X> static constexpr size_t nextMultipleOf(size_t value)
{
    return (value % X) > 0 ? value + X - (value % X) : value;
}

template <std::size_t alignment> Vc_INTRINSIC void *aligned_malloc(std::size_t n)
{
#ifdef __MIC__
    return _mm_malloc(nextMultipleOf<alignment>(n), alignment);
#elif defined(_WIN32)
# ifdef __GNUC__
    return __mingw_aligned_malloc(nextMultipleOf<alignment>(n), alignment);
# else
    return _aligned_malloc(nextMultipleOf<alignment>(n), alignment);
# endif
#else
    void *ptr = nullptr;
    if (0 == posix_memalign(&ptr, alignment < sizeof(void *) ? sizeof(void *) : alignment,
                            nextMultipleOf<alignment>(n))) {
        return ptr;
    }
    return ptr;
#endif
}

template <Vc::MallocAlignment A> Vc_ALWAYS_INLINE void *malloc(size_t n)
{
    switch (A) {
    case Vc::AlignOnVector:
        return aligned_malloc<Vc::VectorAlignment>(n);
    case Vc::AlignOnCacheline:
        // TODO: hardcoding 64 is not such a great idea
        return aligned_malloc<64>(n);
    case Vc::AlignOnPage:
        // TODO: hardcoding 4096 is not such a great idea
        return aligned_malloc<4096>(n);
    }
    return nullptr;
}

Vc_ALWAYS_INLINE void free(void *p)
{
#ifdef __MIC__
    _mm_free(p);
#elif defined(_WIN32)
# ifdef __GNUC__
    return __mingw_aligned_free(p);
# else
    return _aligned_free(p);
# endif
#else
    std::free(p);
#endif
}
}  // namespace Common

/**
 * Allocates memory on the Heap with alignment and padding suitable for vectorized access.
 *
 * Memory that was allocated with this function must be released with Vc::free! Other methods might
 * work but are not portable.
 *
 * \param n Specifies the number of objects the allocated memory must be able to store.
 * \tparam T The type of the allocated memory. Note, that the constructor is not called.
 * \tparam A Determines the alignment of the memory. See \ref Vc::MallocAlignment.
 *
 * \return Pointer to memory of the requested type, or 0 on error. The allocated memory is padded at
 * the end to be a multiple of the requested alignment \p A. Thus if you request memory for 21
 * int objects, aligned via Vc::AlignOnCacheline, you can safely read a full cacheline until the
 * end of the array, without generating an out-of-bounds access. For a cacheline size of 64 Bytes
 * and an int size of 4 Bytes you would thus get an array of 128 Bytes to work with.
 *
 * \warning
 * \li The standard malloc function specifies the number of Bytes to allocate whereas this
 *     function specifies the number of values, thus differing in a factor of sizeof(T).
 * \li This function is mainly meant for use with builtin types. If you use a custom
 *     type with a sizeof that is not a multiple of 2 the results might not be what you expect.
 * \li The constructor of T is not called. You can make up for this:
 * \code
 * SomeType *array = new(Vc::malloc<SomeType, Vc::AlignOnCacheline>(N)) SomeType[N];
 * \endcode
 *
 * \see Vc::free
 *
 * \ingroup Utilities
 * \headerfile memory.h <Vc/Memory>
 */
template<typename T, Vc::MallocAlignment A>
Vc_ALWAYS_INLINE T *malloc(size_t n)
{
    return static_cast<T *>(Common::malloc<A>(n * sizeof(T)));
}

/**
 * Frees memory that was allocated with Vc::malloc.
 *
 * \param p The pointer to the memory to be freed.
 *
 * \tparam T The type of the allocated memory.
 *
 * \warning The destructor of T is not called. If needed, you can call the destructor before calling
 * free:
 * \code
 * for (int i = 0; i < N; ++i) {
 *   p[i].~T();
 * }
 * Vc::free(p);
 * \endcode
 *
 * \ingroup Utilities
 * \headerfile memory.h <Vc/Memory>
 *
 * \see Vc::malloc
 */
template<typename T>
Vc_ALWAYS_INLINE void free(T *p)
{
    Common::free(p);
}
}  // namespace Vc

#endif // VC_COMMON_MALLOC_H_
