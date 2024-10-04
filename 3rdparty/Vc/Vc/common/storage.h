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

#ifndef VC_COMMON_STORAGE_H_
#define VC_COMMON_STORAGE_H_

#include "aliasingentryhelper.h"
#include "types.h"
#include "maskbool.h"
#ifdef Vc_IMPL_AVX
#include "../avx/intrinsics.h"
#endif
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
template <typename V> inline V zero();
}  // namespace Detail
namespace Common
{
namespace Detail
{
#ifdef Vc_IMPL_AVX
template <typename ValueType, size_t Size> struct IntrinsicType {
    using type = typename std::conditional<
        std::is_integral<ValueType>::value,
        typename std::conditional<sizeof(ValueType) * Size == 16, __m128i, __m256i>::type,
        typename std::conditional<
            std::is_same<ValueType, double>::value,
            typename std::conditional<sizeof(ValueType) * Size == 16, __m128d,
                                      __m256d>::type,
            typename std::conditional<sizeof(ValueType) * Size == 16, __m128,
                                      __m256>::type>::type>::type;
};
#elif defined Vc_IMPL_SSE
template <typename ValueType, size_t Size> struct IntrinsicType {
    using type = typename std::conditional<
        std::is_integral<ValueType>::value, __m128i,
        typename std::conditional<std::is_same<ValueType, double>::value, __m128d,
                                  __m128>::type>::type;
};
#else
template <typename ValueType, size_t Size> struct IntrinsicType {
    static_assert(Size == 1,
                  "IntrinsicType without SIMD target support may only have Size = 1");
    using type = ValueType;
};
#endif
template <typename ValueType, size_t Size, size_t Bytes = sizeof(ValueType) * Size>
struct BuiltinType;
#ifdef Vc_USE_BUILTIN_VECTOR_TYPES
#define Vc_VECBUILTIN __attribute__((__vector_size__(16)))
template <size_t Size> struct BuiltinType<         double   , Size, 16> { typedef          double    type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         float    , Size, 16> { typedef          float     type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         long long, Size, 16> { typedef          long long type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<unsigned long long, Size, 16> { typedef unsigned long long type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         long     , Size, 16> { typedef          long      type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<unsigned long     , Size, 16> { typedef unsigned long      type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         int      , Size, 16> { typedef          int       type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<unsigned int      , Size, 16> { typedef unsigned int       type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         short    , Size, 16> { typedef          short     type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<unsigned short    , Size, 16> { typedef unsigned short     type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         char     , Size, 16> { typedef          char      type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<unsigned char     , Size, 16> { typedef unsigned char      type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<  signed char     , Size, 16> { typedef   signed char      type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         bool     , Size, 16> { typedef unsigned char      type Vc_VECBUILTIN; };
#undef Vc_VECBUILTIN
#define Vc_VECBUILTIN __attribute__((__vector_size__(32)))
template <size_t Size> struct BuiltinType<         double   , Size, 32> { typedef          double    type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         float    , Size, 32> { typedef          float     type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         long long, Size, 32> { typedef          long long type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<unsigned long long, Size, 32> { typedef unsigned long long type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         long     , Size, 32> { typedef          long      type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<unsigned long     , Size, 32> { typedef unsigned long      type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         int      , Size, 32> { typedef          int       type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<unsigned int      , Size, 32> { typedef unsigned int       type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         short    , Size, 32> { typedef          short     type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<unsigned short    , Size, 32> { typedef unsigned short     type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         char     , Size, 32> { typedef          char      type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<unsigned char     , Size, 32> { typedef unsigned char      type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<  signed char     , Size, 32> { typedef   signed char      type Vc_VECBUILTIN; };
template <size_t Size> struct BuiltinType<         bool     , Size, 32> { typedef unsigned char      type Vc_VECBUILTIN; };
#undef Vc_VECBUILTIN
#endif
}  // namespace Detail

template <typename ValueType, size_t Size>
using IntrinsicType = typename Detail::IntrinsicType<ValueType, Size>::type;

template <typename ValueType, size_t Size>
using BuiltinType = typename Detail::BuiltinType<ValueType, Size>::type;

namespace AliasStrategy
{
struct Union {};
struct MayAlias {};
struct VectorBuiltin {};
struct UnionMembers {};
}  // namespace AliasStrategy

using DefaultStrategy =
#if defined Vc_USE_BUILTIN_VECTOR_TYPES
    AliasStrategy::VectorBuiltin;
#elif defined Vc_MSVC
    AliasStrategy::UnionMembers;
#elif defined Vc_ICC
    AliasStrategy::Union;
#elif defined __GNUC__
    AliasStrategy::MayAlias;
#else
    AliasStrategy::Union;
#endif

template <typename ValueType, size_t Size, typename Strategy = DefaultStrategy>
class Storage;

// GCC 6 forbids `EntryType m[]` altogether
template <typename ValueType, size_t Size>
class Storage<ValueType, Size, AliasStrategy::Union>
{
    static_assert(std::is_fundamental<ValueType>::value &&
                      std::is_arithmetic<ValueType>::value,
                  "Only works for fundamental arithmetic types.");

public:
    using VectorType = IntrinsicType<ValueType, Size>;
    using EntryType = ValueType;

    union Alias {
        Vc_INTRINSIC Alias(VectorType vv) : v(vv) {}
        VectorType v;
        EntryType m[Size];
    };

    Vc_INTRINSIC Storage() : data(Vc::Detail::zero<VectorType>()) {}
    Vc_INTRINSIC Storage(const VectorType &x) : data(x) { assertCorrectAlignment(&data); }
    template <typename U>
    Vc_INTRINSIC explicit Storage(const U &x,
                                  enable_if<sizeof(U) == sizeof(VectorType)> = nullarg)
        : data(reinterpret_cast<VectorType>(x))
    {
        assertCorrectAlignment(&data);
    }

    Vc_INTRINSIC Storage(const Storage &) = default;
    Vc_INTRINSIC Storage &operator=(const Storage &) = default;

    Vc_INTRINSIC operator const VectorType &() const { return data; }
    Vc_INTRINSIC Vc_PURE VectorType &v() { return data; }
    Vc_INTRINSIC Vc_PURE const VectorType &v() const { return data; }
    Vc_INTRINSIC Vc_PURE EntryType m(size_t i) const { return Alias(data).m[i]; }
    Vc_INTRINSIC void set(size_t i, EntryType x)
    {
        Alias a(data);
        a.m[i] = x;
        data = a.v;
    }

private:
    VectorType data;
};

template <typename ValueType, size_t Size>
class Storage<ValueType, Size, AliasStrategy::MayAlias>
{
    static_assert(std::is_fundamental<ValueType>::value &&
                      std::is_arithmetic<ValueType>::value,
                  "Only works for fundamental arithmetic types.");

public:
    using VectorType = IntrinsicType<ValueType, Size>;
    using EntryType = ValueType;

    Vc_INTRINSIC Storage() : data() { assertCorrectAlignment(&data); }
    Vc_INTRINSIC Storage(const VectorType &x) : data(x)
    {
        assertCorrectAlignment(&data);
    }
    template <typename U>
    Vc_INTRINSIC explicit Storage(const U &x,
                                  enable_if<sizeof(U) == sizeof(VectorType)> = nullarg)
        : data(reinterpret_cast<const VectorType &>(x))
    {
        assertCorrectAlignment(&data);
    }
    Vc_INTRINSIC Storage &operator=(const VectorType &x)
    {
        data = x;
        return *this;
    }

    Vc_INTRINSIC Storage(const Storage &) = default;
    Vc_INTRINSIC Storage &operator=(const Storage &) = default;

    Vc_INTRINSIC operator const VectorType &() const { return v(); }
    Vc_INTRINSIC Vc_PURE VectorType &v() { return data; }
    Vc_INTRINSIC Vc_PURE const VectorType &v() const { return data; }

    Vc_INTRINSIC Vc_PURE EntryType m(size_t i) const
    {
        return aliasing_cast<EntryType>(&data)[i];
    }
    Vc_INTRINSIC void set(size_t i, EntryType x)
    {
        aliasing_cast<EntryType>(&data)[i] = x;
    }

private:
    VectorType data;
};

template <typename ValueType, size_t Size>
class Storage<ValueType, Size, AliasStrategy::VectorBuiltin>
{
    static_assert(std::is_fundamental<ValueType>::value &&
                      std::is_arithmetic<ValueType>::value,
                  "Only works for fundamental arithmetic types.");

    using Builtin = BuiltinType<ValueType, Size>;

public:
    using VectorType =
#ifdef Vc_TEMPLATES_DROP_ATTRIBUTES
        MayAlias<IntrinsicType<ValueType, Size>>;
#else
        IntrinsicType<ValueType, Size>;
#endif
    using EntryType = ValueType;

    Vc_INTRINSIC Storage() : data() { assertCorrectAlignment(&data); }
    Vc_INTRINSIC Storage(const Storage &) = default;
    Vc_INTRINSIC Storage &operator=(const Storage &) = default;

    Vc_INTRINSIC Storage(const VectorType &x)
        : data(aliasing_cast<Builtin>(x))
    {
        assertCorrectAlignment(&data);
    }
    template <typename U>
    Vc_INTRINSIC explicit Storage(const U &x,
                                  enable_if<sizeof(U) == sizeof(VectorType)> = nullarg)
        : data(aliasing_cast<Builtin>(x))
    {
        assertCorrectAlignment(&data);
    }
    Vc_INTRINSIC Storage &operator=(const VectorType &x)
    {
        data = aliasing_cast<Builtin>(x);
        return *this;
    }

    Vc_INTRINSIC operator const VectorType &() const { return v(); }
    Vc_INTRINSIC Vc_PURE VectorType &v() { return reinterpret_cast<VectorType &>(data); }
    Vc_INTRINSIC Vc_PURE const VectorType &v() const { return reinterpret_cast<const VectorType &>(data); }

    Vc_INTRINSIC Vc_PURE EntryType m(size_t i) const { return data[i]; }
    Vc_INTRINSIC void set(size_t i, EntryType x) { data[i] = x; }

    Vc_INTRINSIC Builtin &builtin() { return data; }
    Vc_INTRINSIC const Builtin &builtin() const { return data; }

private:
    Builtin data;
};

template <typename ValueType, size_t Size>
class Storage<ValueType, Size, AliasStrategy::UnionMembers>
{
    static_assert(std::is_fundamental<ValueType>::value &&
                      std::is_arithmetic<ValueType>::value,
                  "Only works for fundamental arithmetic types.");

public:
    using VectorType = IntrinsicType<ValueType, Size>;
    using EntryType = ValueType;

    Vc_INTRINSIC Storage() : data() { assertCorrectAlignment(&data); }
    Vc_INTRINSIC Storage(const VectorType &x) : data(x)
    {
        assertCorrectAlignment(&data);
    }
    template <typename U>
    Vc_INTRINSIC explicit Storage(const U &x,
                                  enable_if<sizeof(U) == sizeof(VectorType)> = nullarg)
        : data(reinterpret_cast<const VectorType &>(x))
    {
        assertCorrectAlignment(&data);
    }
    Vc_INTRINSIC Storage &operator=(const VectorType &x)
    {
        data = x;
        return *this;
    }

    Vc_INTRINSIC Storage(const Storage &) = default;
    Vc_INTRINSIC Storage &operator=(const Storage &) = default;

    Vc_INTRINSIC Vc_PURE VectorType &v() { return data; }
    Vc_INTRINSIC Vc_PURE const VectorType &v() const { return data; }

    Vc_INTRINSIC_L Vc_PURE_L EntryType m(size_t i) const Vc_INTRINSIC_R Vc_PURE_R;
    Vc_INTRINSIC void set(size_t i, EntryType x) { ref(i) = x; }

private:
    Vc_INTRINSIC_L Vc_PURE_L EntryType &ref(size_t i) Vc_INTRINSIC_R Vc_PURE_R;
    VectorType data;
};

#ifdef Vc_MSVC
template <> Vc_INTRINSIC Vc_PURE          double Storage<         double, 2, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m128d_f64[i]; }
template <> Vc_INTRINSIC Vc_PURE          float  Storage<         float , 4, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m128_f32[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed int    Storage<  signed int   , 4, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m128i_i32[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed short  Storage<  signed short , 8, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m128i_i16[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed char   Storage<  signed char  ,16, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m128i_i8[i]; }
template <> Vc_INTRINSIC Vc_PURE unsigned int    Storage<unsigned int   , 4, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m128i_u32[i]; }
template <> Vc_INTRINSIC Vc_PURE unsigned short  Storage<unsigned short , 8, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m128i_u16[i]; }
template <> Vc_INTRINSIC Vc_PURE unsigned char   Storage<unsigned char  ,16, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m128i_u8[i]; }

template <> Vc_INTRINSIC Vc_PURE          double &Storage<         double, 2, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m128d_f64[i]; }
template <> Vc_INTRINSIC Vc_PURE          float  &Storage<         float , 4, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m128_f32[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed int    &Storage<  signed int   , 4, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m128i_i32[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed short  &Storage<  signed short , 8, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m128i_i16[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed char   &Storage<  signed char  ,16, AliasStrategy::UnionMembers>::ref(size_t i) { return reinterpret_cast<signed char &>(data.m128i_i8[i]); }
template <> Vc_INTRINSIC Vc_PURE unsigned int    &Storage<unsigned int   , 4, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m128i_u32[i]; }
template <> Vc_INTRINSIC Vc_PURE unsigned short  &Storage<unsigned short , 8, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m128i_u16[i]; }
template <> Vc_INTRINSIC Vc_PURE unsigned char   &Storage<unsigned char  ,16, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m128i_u8[i]; }

#ifdef Vc_IMPL_AVX
template <> Vc_INTRINSIC Vc_PURE          double Storage<         double, 4, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m256d_f64[i]; }
template <> Vc_INTRINSIC Vc_PURE          float  Storage<         float , 8, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m256_f32[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed int    Storage<  signed int   , 8, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m256i_i32[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed short  Storage<  signed short ,16, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m256i_i16[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed char   Storage<  signed char  ,32, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m256i_i8[i]; }
template <> Vc_INTRINSIC Vc_PURE unsigned int    Storage<unsigned int   , 8, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m256i_u32[i]; }
template <> Vc_INTRINSIC Vc_PURE unsigned short  Storage<unsigned short ,16, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m256i_u16[i]; }
template <> Vc_INTRINSIC Vc_PURE unsigned char   Storage<unsigned char  ,32, AliasStrategy::UnionMembers>::m(size_t i) const { return data.m256i_u8[i]; }

template <> Vc_INTRINSIC Vc_PURE          double &Storage<         double, 4, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m256d_f64[i]; }
template <> Vc_INTRINSIC Vc_PURE          float  &Storage<         float , 8, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m256_f32[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed int    &Storage<  signed int   , 8, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m256i_i32[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed short  &Storage<  signed short ,16, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m256i_i16[i]; }
template <> Vc_INTRINSIC Vc_PURE   signed char   &Storage<  signed char  ,32, AliasStrategy::UnionMembers>::ref(size_t i) { return reinterpret_cast<signed char &>(data.m256i_i8[i]); }
template <> Vc_INTRINSIC Vc_PURE unsigned int    &Storage<unsigned int   , 8, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m256i_u32[i]; }
template <> Vc_INTRINSIC Vc_PURE unsigned short  &Storage<unsigned short ,16, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m256i_u16[i]; }
template <> Vc_INTRINSIC Vc_PURE unsigned char   &Storage<unsigned char  ,32, AliasStrategy::UnionMembers>::ref(size_t i) { return data.m256i_u8[i]; }
#endif
#endif  // Vc_MSVC

template <typename VectorType, typename EntryType>
using VectorMemoryUnion = Storage<EntryType, sizeof(VectorType) / sizeof(EntryType)>;

}  // namespace Common
}  // namespace Vc

#endif // VC_COMMON_STORAGE_H_
