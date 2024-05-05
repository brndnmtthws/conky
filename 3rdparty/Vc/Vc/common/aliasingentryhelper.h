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

#ifndef VC_COMMON_ALIASINGENTRYHELPER_H_
#define VC_COMMON_ALIASINGENTRYHELPER_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{

template<class StorageType> class AliasingEntryHelper
{
    private:
        typedef typename StorageType::EntryType T;
#ifdef Vc_ICC
        StorageType *const m_storage;
        const int m_index;
    public:
        Vc_ALWAYS_INLINE AliasingEntryHelper(StorageType *d, int index) : m_storage(d), m_index(index) {}
        Vc_ALWAYS_INLINE AliasingEntryHelper(const AliasingEntryHelper &) = default;
        Vc_ALWAYS_INLINE AliasingEntryHelper(AliasingEntryHelper &&) = default;
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator=(const AliasingEntryHelper &rhs) {
            m_storage->assign(m_index, rhs);
            return *this;
        }

        Vc_ALWAYS_INLINE AliasingEntryHelper &operator  =(T x) { m_storage->assign(m_index, x); return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator +=(T x) { m_storage->assign(m_index, m_storage->m(m_index) + x); return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator -=(T x) { m_storage->assign(m_index, m_storage->m(m_index) - x); return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator /=(T x) { m_storage->assign(m_index, m_storage->m(m_index) / x); return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator *=(T x) { m_storage->assign(m_index, m_storage->m(m_index) * x); return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator |=(T x) { m_storage->assign(m_index, m_storage->m(m_index) | x); return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator &=(T x) { m_storage->assign(m_index, m_storage->m(m_index) & x); return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator ^=(T x) { m_storage->assign(m_index, m_storage->m(m_index) ^ x); return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator %=(T x) { m_storage->assign(m_index, m_storage->m(m_index) % x); return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator<<=(T x) { m_storage->assign(m_index, m_storage->m(m_index)<< x); return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator>>=(T x) { m_storage->assign(m_index, m_storage->m(m_index)>> x); return *this; }
#define m_data m_storage->read(m_index)
#else
        typedef T A Vc_MAY_ALIAS;
        A &m_data;
    public:
        template<typename T2>
        Vc_ALWAYS_INLINE AliasingEntryHelper(T2 &d) : m_data(reinterpret_cast<A &>(d)) {}

        Vc_ALWAYS_INLINE AliasingEntryHelper(A &d) : m_data(d) {}
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator=(const AliasingEntryHelper &rhs) {
            m_data = rhs.m_data;
            return *this;
        }

        Vc_ALWAYS_INLINE AliasingEntryHelper &operator =(T x) { m_data  = x; return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator+=(T x) { m_data += x; return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator-=(T x) { m_data -= x; return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator/=(T x) { m_data /= x; return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator*=(T x) { m_data *= x; return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator|=(T x) { m_data |= x; return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator&=(T x) { m_data &= x; return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator^=(T x) { m_data ^= x; return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator%=(T x) { m_data %= x; return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator<<=(T x) { m_data <<= x; return *this; }
        Vc_ALWAYS_INLINE AliasingEntryHelper &operator>>=(T x) { m_data >>= x; return *this; }
#endif

        Vc_ALWAYS_INLINE Vc_PURE operator const T() const { return m_data; }

        Vc_ALWAYS_INLINE Vc_PURE bool operator==(T x) const { return static_cast<T>(m_data) == x; }
        Vc_ALWAYS_INLINE Vc_PURE bool operator!=(T x) const { return static_cast<T>(m_data) != x; }
        Vc_ALWAYS_INLINE Vc_PURE bool operator<=(T x) const { return static_cast<T>(m_data) <= x; }
        Vc_ALWAYS_INLINE Vc_PURE bool operator>=(T x) const { return static_cast<T>(m_data) >= x; }
        Vc_ALWAYS_INLINE Vc_PURE bool operator< (T x) const { return static_cast<T>(m_data) <  x; }
        Vc_ALWAYS_INLINE Vc_PURE bool operator> (T x) const { return static_cast<T>(m_data) >  x; }

        Vc_ALWAYS_INLINE Vc_PURE T operator-() const { return -static_cast<T>(m_data); }
        Vc_ALWAYS_INLINE Vc_PURE T operator~() const { return ~static_cast<T>(m_data); }
        Vc_ALWAYS_INLINE Vc_PURE T operator+(T x) const { return static_cast<T>(m_data) + x; }
        Vc_ALWAYS_INLINE Vc_PURE T operator-(T x) const { return static_cast<T>(m_data) - x; }
        Vc_ALWAYS_INLINE Vc_PURE T operator/(T x) const { return static_cast<T>(m_data) / x; }
        Vc_ALWAYS_INLINE Vc_PURE T operator*(T x) const { return static_cast<T>(m_data) * x; }
        Vc_ALWAYS_INLINE Vc_PURE T operator|(T x) const { return static_cast<T>(m_data) | x; }
        Vc_ALWAYS_INLINE Vc_PURE T operator&(T x) const { return static_cast<T>(m_data) & x; }
        Vc_ALWAYS_INLINE Vc_PURE T operator^(T x) const { return static_cast<T>(m_data) ^ x; }
        Vc_ALWAYS_INLINE Vc_PURE T operator%(T x) const { return static_cast<T>(m_data) % x; }
        //T operator<<(T x) const { return static_cast<T>(m_data) << x; }
        //T operator>>(T x) const { return static_cast<T>(m_data) >> x; }
#ifdef m_data
#undef m_data
#endif
};

}  // namespace Common
}  // namespace Vc

#endif // VC_COMMON_ALIASINGENTRYHELPER_H_
