/*  This file is part of the Vc library. {{{
Copyright © 2016 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_ELEMENTREFERENCE_H_
#define VC_COMMON_ELEMENTREFERENCE_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
template <typename U, typename Accessor = U> class ElementReference
{
    friend U;
    friend Accessor;
    Vc_INTRINSIC ElementReference(U &o, int i) noexcept : index(i), obj(o) {}

    static constexpr bool get_noexcept =
        noexcept(Accessor::get(std::declval<U &>(), int()));
    template <typename T> static constexpr bool set_noexcept()
    {
        return noexcept(Accessor::set(std::declval<U &>(), int(), std::declval<T>()));
    }

public:
    using value_type = typename U::value_type;
    Vc_INTRINSIC ElementReference(const ElementReference &) = delete;

    /**
     * Move Constructor
     *
     * this is the only way to constructor an ElementReference in user code
     *
     * \note
     * Please be aware that this class models the concept of a reference
     * and as such it can have the same lifetime issue as a standard C++
     * reference.
     *
     * \note
     * C++ 17 support copy-elision, which in turn allows to
     * the ElementReference obtained via operator[] from a function
     * and avoid copying. C++11 and C++14 don't offer this, thus we add
     * the move constructor, to allow them to move the data and thus avoid
     * copying (which was prohibited by the deleted constructor above
     */
    Vc_INTRINSIC ElementReference(ElementReference &&) = default;

    Vc_INTRINSIC operator value_type() const noexcept(get_noexcept)
    {
        return Accessor::get(obj, index);
    }

    template <typename T>
        Vc_INTRINSIC ElementReference &operator=(T &&x) &&
        noexcept(noexcept(Accessor::set(std::declval<U &>(), int(), std::declval<T>())))
    {
        Accessor::set(obj, index, std::forward<T>(x));
        return *this;
    }

// TODO: improve with operator.()

#define Vc_OP_(op_)                                                                      \
    template <typename T, typename R = decltype(std::declval<const value_type &>()       \
                                                    op_ std::declval<T>())>              \
        Vc_INTRINSIC ElementReference &operator op_##=(T &&x) &&                         \
        noexcept(get_noexcept && noexcept(Accessor::set(std::declval<U &>(), int(),      \
                                                        std::declval<R &&>())))          \
    {                                                                                    \
        const value_type &lhs = Accessor::get(obj, index);                               \
        Accessor::set(obj, index, lhs op_ std::forward<T>(x));                           \
        return *this;                                                                    \
    }
    Vc_ALL_ARITHMETICS(Vc_OP_);
    Vc_ALL_SHIFTS(Vc_OP_);
    Vc_ALL_BINARY(Vc_OP_);
#undef Vc_OP_

    template <typename = void>
        Vc_INTRINSIC ElementReference &operator++() &&
        noexcept(noexcept(std::declval<value_type &>() =
                              Accessor::get(std::declval<U &>(), int())) &&
                 set_noexcept<decltype(++std::declval<value_type &>())>())
    {
        value_type x = Accessor::get(obj, index);
        Accessor::set(obj, index, ++x);
        return *this;
    }

    template <typename = void>
        Vc_INTRINSIC value_type operator++(int) &&
        noexcept(noexcept(std::declval<value_type &>() =
                              Accessor::get(std::declval<U &>(), int())) &&
                 set_noexcept<decltype(std::declval<value_type &>()++)>())
    {
        const value_type r = Accessor::get(obj, index);
        value_type x = r;
        Accessor::set(obj, index, ++x);
        return r;
    }

    template <typename = void>
        Vc_INTRINSIC ElementReference &operator--() &&
        noexcept(noexcept(std::declval<value_type &>() =
                              Accessor::get(std::declval<U &>(), int())) &&
                 set_noexcept<decltype(--std::declval<value_type &>())>())
    {
        value_type x = Accessor::get(obj, index);
        Accessor::set(obj, index, --x);
        return *this;
    }

    template <typename = void>
        Vc_INTRINSIC value_type operator--(int) &&
        noexcept(noexcept(std::declval<value_type &>() =
                              Accessor::get(std::declval<U &>(), int())) &&
                 set_noexcept<decltype(std::declval<value_type &>()--)>())
    {
        const value_type r = Accessor::get(obj, index);
        value_type x = r;
        Accessor::set(obj, index, --x);
        return r;
    }

    friend void swap(ElementReference &&a, ElementReference &&b) {
        value_type tmp(a);
        static_cast<ElementReference &&>(a) = static_cast<value_type>(b);
        static_cast<ElementReference &&>(b) = tmp;
    }

    friend void swap(value_type &a, ElementReference &&b) {
        value_type tmp(a);
        a = static_cast<value_type>(b);
        static_cast<ElementReference &&>(b) = tmp;
    }

    friend void swap(ElementReference &&a, value_type &b) {
        value_type tmp(a);
        static_cast<ElementReference &&>(a) = b;
        b = tmp;
    }

private:
    int index;
    U &obj;
};

}  // namespace Detail
}  // namespace Vc

#endif  // VC_COMMON_ELEMENTREFERENCE_H_

// vim: foldmethod=marker
