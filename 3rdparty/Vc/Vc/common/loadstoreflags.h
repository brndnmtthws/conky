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

#ifndef VC_COMMON_LOADSTOREFLAGS_H_
#define VC_COMMON_LOADSTOREFLAGS_H_

#include "../traits/type_traits.h"

namespace Vc_VERSIONED_NAMESPACE
{

/**
 * Hint for \ref Prefetch to select prefetches that mark the memory as exclusive.
 *
 * This hint may optimize the prefetch if the memory will subsequently be written to.
 */
struct Exclusive {};
/**
 * Hint for \ref Prefetch to select prefetches that mark the memory as shared.
 */
struct Shared {};

namespace LoadStoreFlags
{

struct StreamingFlag {};
struct UnalignedFlag {};
struct PrefetchFlagBase {};
// TODO: determine a good default for typical CPU use
template <size_t L1 = 16 * 64, size_t L2 = 128 * 64, typename ExclusiveOrShared_ = void>
struct PrefetchFlag : public PrefetchFlagBase {
    typedef ExclusiveOrShared_ ExclusiveOrShared;
    static constexpr size_t L1Stride = L1;
    static constexpr size_t L2Stride = L2;
    static constexpr bool IsExclusive = std::is_same<ExclusiveOrShared, Exclusive>::value;
    static constexpr bool IsShared = std::is_same<ExclusiveOrShared, Shared>::value;
};

template<typename Base, typename Default, typename... LoadStoreFlags> struct ExtractType
{
    typedef Default type;
};
template<typename Base, typename Default, typename T, typename... LoadStoreFlags> struct ExtractType<Base, Default, T, LoadStoreFlags...>
{
    typedef typename std::conditional<std::is_base_of<Base, T>::value, T, typename ExtractType<Base, Default, LoadStoreFlags...>::type>::type type;
};

// ICC warns about the constexpr members in LoadStoreFlags: member "LoadStoreFlags<Flags...>::IsAligned" was declared but never referenced
// who needs that warning, especially if it was referenced...
// The warning cannot be reenabled because it gets emitted whenever the LoadStoreFlags is instantiated
// somewhere, so it could be anywhere.
#ifdef Vc_ICC
#pragma warning(disable: 177)
#endif
/**\internal
 * Implementation of the load/store flags mechanism. This is internal API. Only some
 * concrete aliases are API-relevant types.
 */
template<typename... Flags> struct LoadStoreFlags
{
private:
    // ICC doesn't grok this line:
    //template<typename Test> using TestFlag = std::is_same<typename ExtractType<StreamingFlag, void, Flags...>::type, void>;
    typedef typename ExtractType<PrefetchFlagBase, PrefetchFlag<0, 0>, Flags...>::type Prefetch;

public:
    constexpr LoadStoreFlags() {}

    static constexpr bool IsStreaming = !std::is_same<typename ExtractType<StreamingFlag, void, Flags...>::type, void>::value;
    static constexpr bool IsUnaligned = !std::is_same<typename ExtractType<UnalignedFlag, void, Flags...>::type, void>::value;
    static constexpr bool IsAligned = !IsUnaligned;
    static constexpr bool IsPrefetch = !std::is_same<typename ExtractType<PrefetchFlagBase, void, Flags...>::type, void>::value;
    static constexpr bool IsExclusivePrefetch = Prefetch::IsExclusive;
    static constexpr bool IsSharedPrefetch = Prefetch::IsShared;
    static constexpr size_t L1Stride = Prefetch::L1Stride;
    static constexpr size_t L2Stride = Prefetch::L2Stride;

    typedef LoadStoreFlags<typename std::conditional<std::is_same<Flags, UnalignedFlag>::value, void, Flags>::type...> UnalignedRemoved;

    // The following EnableIf* convenience types cannot use enable_if because then no LoadStoreFlags type
    // could ever be instantiated. Instead these types are defined either as void* or void. The
    // function that does SFINAE then assigns "= nullptr" to this type. Thus, the ones with just
    // void result in substitution failure.
    typedef typename std::conditional<IsAligned   && !IsStreaming, void *, void>::type EnableIfAligned;
    typedef typename std::conditional<IsAligned   &&  IsStreaming, void *, void>::type EnableIfStreaming;
    typedef typename std::conditional<IsUnaligned && !IsStreaming, void *, void>::type EnableIfUnalignedNotStreaming;
    typedef typename std::conditional<IsUnaligned &&  IsStreaming, void *, void>::type EnableIfUnalignedAndStreaming;
    typedef typename std::conditional<IsUnaligned                , void *, void>::type EnableIfUnaligned;
    typedef typename std::conditional<!IsUnaligned               , void *, void>::type EnableIfNotUnaligned;
    typedef typename std::conditional<IsPrefetch                 , void *, void>::type EnableIfPrefetch;
    typedef typename std::conditional<!IsPrefetch                , void *, void>::type EnableIfNotPrefetch;
};

/**\internal
 * Specialization for no flags (i.e aligned, non-streaming, no prefetching)
 */
template<> struct LoadStoreFlags<>
{
    constexpr LoadStoreFlags() {}

    static constexpr bool IsStreaming = false;
    static constexpr bool IsUnaligned = false;
    static constexpr bool IsAligned = !IsUnaligned;
    static constexpr bool IsPrefetch = false;
    static constexpr bool IsExclusivePrefetch = false;
    static constexpr bool IsSharedPrefetch = false;
    static constexpr size_t L1Stride = 0;
    static constexpr size_t L2Stride = 0;
    typedef void* EnableIfAligned;
    typedef void* EnableIfNotUnaligned;
    typedef void* EnableIfNotPrefetch;
};

/**
 * Operator for concatenation of LoadStoreFlags.
 *
 * Example:
 * \code
 * float_v x(mem, Vc::Aligned | Vc::Streaming);
 * \endcode
 */
template<typename... LFlags, typename... RFlags>
constexpr LoadStoreFlags<LFlags..., RFlags...> operator|(LoadStoreFlags<LFlags...>, LoadStoreFlags<RFlags...>)
{
    return LoadStoreFlags<LFlags..., RFlags...>();
}

} // LoadStoreFlags namespace

using LoadStoreFlags::PrefetchFlag;

typedef LoadStoreFlags::LoadStoreFlags<> AlignedTag;
typedef LoadStoreFlags::LoadStoreFlags<LoadStoreFlags::StreamingFlag> StreamingTag;
typedef LoadStoreFlags::LoadStoreFlags<LoadStoreFlags::UnalignedFlag> UnalignedTag;

/// The default load tag type uses unaligned (non-streaming) loads.
typedef UnalignedTag DefaultLoadTag;
/// The default store tag type uses unaligned (non-streaming) stores.
typedef UnalignedTag DefaultStoreTag;

/**\addtogroup Utilities
 * @{
 */
/**
 * Use this object for a \p flags parameter to request aligned loads and stores.
 *
 * It specifies that a load/store can expect a memory address that is aligned on
 * the correct boundary. (i.e. \p MemoryAlignment)
 *
 * \warning
 * If you specify Aligned, but the memory address is not aligned the program
 * will most likely crash.
 */
constexpr AlignedTag Aligned;

/**
 * Use this object for a \p flags parameter to request unaligned loads and stores.
 *
 * It specifies that a load/store can \em not expect a memory address that is
 * aligned on the correct boundary. (i.e. alignment is less than
 * \p MemoryAlignment)
 *
 * \note
 * If you specify Unaligned, but the memory address is aligned the load/store
 * will execute slightly slower than necessary.
 */
constexpr UnalignedTag Unaligned;

/**
 * Use this object for a \p flags parameter to request streaming loads and stores.
 *
 * It specifies that the cache should be bypassed for the given load/store.
 * Whether this will actually be done depends on the target system's capabilities.
 *
 * Streaming stores can be interesting when the code calculates values that, after being
 * written to memory, will not be used for a long time or used by a different thread.
 *
 * \note
 * Expect that most target systems do not support unaligned streaming loads or stores.
 * Therefore, make sure that you also specify Aligned.
 */
constexpr StreamingTag Streaming;

/**
 * Use this object for a \p flags parameter to request default software prefetches to be
 * emitted.
 */
constexpr LoadStoreFlags::LoadStoreFlags<PrefetchFlag<>> PrefetchDefault;
///@}

/**
 * \tparam L1
 * \tparam L2
 * \tparam ExclusiveOrShared
 */
template <size_t L1 = PrefetchFlag<>::L1Stride,
          size_t L2 = PrefetchFlag<>::L2Stride,
          typename ExclusiveOrShared = PrefetchFlag<>::ExclusiveOrShared>
struct Prefetch : public LoadStoreFlags::LoadStoreFlags<PrefetchFlag<L1, L2, ExclusiveOrShared>>
{
};

namespace Traits
{
///\internal partial specialization for detecting LoadStoreFlags types
template <typename... Ts>
struct is_loadstoreflag_internal<LoadStoreFlags::LoadStoreFlags<Ts...>> : public std::true_type
{
};
///\internal partial specialization for detecting the derived Prefetch type as a
/// load/store flag.
template <size_t L1, size_t L2, typename ExclusiveOrShared>
struct is_loadstoreflag_internal<Prefetch<L1, L2, ExclusiveOrShared>> : public std::true_type
{
};
}  // namespace Traits
}  // namespace Vc

#endif // VC_COMMON_LOADSTOREFLAGS_H_
