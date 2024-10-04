// -*- C++ -*-
//===------------------------------ span ---------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
// Adapted for use with Vc:
// Copyright © 2018 Matthias Kretz <kretz@kde.org>
//===---------------------------------------------------------------------===//

#ifndef VC_COMMON_SPAN_H_
#define VC_COMMON_SPAN_H_

#include <array>        // for array
#include <cstddef>      // for ptrdiff_t
#include <cstddef>      // for std::byte
#include <iterator>     // for iterators
#include <type_traits>  // for remove_cv, etc
#include "subscript.h"  // for AdaptSubscriptOperator

namespace Vc_VERSIONED_NAMESPACE
{
#ifdef __cpp_inline_variables
inline
#endif
    constexpr ptrdiff_t dynamic_extent = -1;
namespace Common
{
template <typename T, ptrdiff_t Extent = dynamic_extent> class span;

template <typename T, ptrdiff_t Extent>
constexpr auto begin(const span<T, Extent>& s) noexcept -> decltype(s.begin())
{
    return s.begin();
}
template <typename T, ptrdiff_t Extent>
constexpr auto end(const span<T, Extent>& s) noexcept -> decltype(s.end())
{
    return s.end();
}

template <class T> struct _is_span_impl : public std::false_type {
};

template <class T, ptrdiff_t Extent>
struct _is_span_impl<span<T, Extent>> : public std::true_type {
};

template <class T>
struct _is_span : public _is_span_impl<typename std::remove_cv<T>::type> {
};

template <class T> struct _is_std_array_impl : public std::false_type {
};

template <class T, size_t Sz>
struct _is_std_array_impl<array<T, Sz>> : public std::true_type {
};

template <class T>
struct _is_std_array : public _is_std_array_impl<typename std::remove_cv<T>::type> {
};

template <class T, class ElementType, class = void>
struct _is_span_compatible_container : public std::false_type {
};

template <class... Ts> using _void_t = void;

template <class C> constexpr auto _std_data(C& c) -> decltype(c.data())
{
    return c.data();
}
template <class C> constexpr auto _std_data(const C& c) -> decltype(c.data())
{
    return c.data();
}
template <class T, std::size_t N> constexpr T* _std_data(T (&array)[N]) noexcept
{
    return array;
}
template <class E> constexpr const E* _std_data(std::initializer_list<E> il) noexcept
{
    return il.begin();
}

template <class C> constexpr auto _std_size(const C& c) -> decltype(c.size())
{
    return c.size();
}
template <class T, std::size_t N>
constexpr std::size_t _std_size(const T (&array)[N]) noexcept
{
    return N;
}

template <class T, class ElementType>
struct _is_span_compatible_container<
    T, ElementType,
    _void_t<
        // is not a specialization of span
        typename std::enable_if<!_is_span<T>::value, std::nullptr_t>::type,
        // is not a specialization of array
        typename std::enable_if<!_is_std_array<T>::value, std::nullptr_t>::type,
        // is_array_v<Container> is false,
        typename std::enable_if<!std::is_array<T>::value, std::nullptr_t>::type,
        // data(cont) and size(cont) are well formed
        decltype(data(std::declval<T>())), decltype(size(std::declval<T>())),
        // remove_pointer_t<decltype(data(cont))>(*)[] is convertible to ElementType(*)[]
        typename std::enable_if<
            std::is_convertible<typename std::remove_pointer<decltype(
                                    data(std::declval<T&>()))>::type (*)[],
                                ElementType (*)[]>::value,
            std::nullptr_t>::type>> : public std::true_type {
};

#if defined Vc_MSVC || (defined Vc_GCC && Vc_GCC < 0x50100) || defined Vc_ICC || !defined __cpp_constexpr || __cpp_constexpr < 201304
#define Vc_CONSTEXPR
#else
#define Vc_CONSTEXPR constexpr
#endif

template <typename T, ptrdiff_t Extent> class span
{
public:
    //  constants and types
    using element_type = T;
    using value_type = typename std::remove_cv<T>::type;
    using index_type = ptrdiff_t;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;  // not in standard
    using reference = T&;
    using const_reference = const T&;  // not in standard
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr index_type extent = Extent;
    static_assert(Extent >= 0, "Can't have a span with an extent < 0");

    // [span.cons], span constructors, copy, assignment, and destructor
    Vc_CONSTEXPR span() noexcept : data_{nullptr}
    {
        static_assert(Extent == 0,
                      "Can't default construct a statically sized span with size > 0");
    }

    Vc_CONSTEXPR span(const span&) noexcept = default;
    Vc_CONSTEXPR span& operator=(const span&) noexcept = default;

    Vc_CONSTEXPR span(pointer _ptr, index_type _count) : data_{_ptr}
    {
        (void)_count;
        Vc_ASSERT(((void)"size mismatch in span's constructor (ptr, len)", Extent == _count));
    }
    Vc_CONSTEXPR span(pointer _f, pointer _l) : data_{_f}
    {
        (void)_l;
        Vc_ASSERT(((void)"size mismatch in span's constructor (ptr, ptr)",
                   Extent == distance(_f, _l)));
    }

    Vc_CONSTEXPR span(element_type (&_arr)[Extent]) noexcept : data_{_arr} {}
    Vc_CONSTEXPR span(array<value_type, Extent>& _arr) noexcept : data_{_arr.data()} {}
    Vc_CONSTEXPR span(const array<value_type, Extent>& _arr) noexcept : data_{_arr.data()} {}

    template <class Container>
    inline Vc_CONSTEXPR span(
        Container& _c,
        typename std::enable_if<_is_span_compatible_container<Container, T>::value,
                                std::nullptr_t>::type = nullptr)
        : data_{_std_data(_c)}
    {
        Vc_ASSERT(("size mismatch in span's constructor (container))",
                   Extent == _std_size(_c)));
    }

    template <class Container>
    inline Vc_CONSTEXPR span(
        const Container& _c,
        typename std::enable_if<_is_span_compatible_container<const Container, T>::value,
                                std::nullptr_t>::type = nullptr)
        : data_{_std_data(_c)}
    {
        Vc_ASSERT(("size mismatch in span's constructor (const container)",
                   Extent == _std_size(_c)));
    }

    template <class OtherElementType>
    inline Vc_CONSTEXPR span(
        const span<OtherElementType, Extent>& _other,
        typename std::enable_if<
            std::is_convertible<OtherElementType (*)[], element_type (*)[]>::value,
            std::nullptr_t>::type = nullptr)
        : data_{_other.data()}
    {
    }

    template <class OtherElementType>
    inline Vc_CONSTEXPR span(
        const span<OtherElementType, dynamic_extent>& _other,
        typename std::enable_if<
            std::is_convertible<OtherElementType (*)[], element_type (*)[]>::value,
            std::nullptr_t>::type = nullptr) noexcept
        : data_{_other.data()}
    {
        Vc_ASSERT(("size mismatch in span's constructor (other span)",
                   Extent == _other.size()));
    }

    //  ~span() noexcept = default;

    template <ptrdiff_t Count>
    inline Vc_CONSTEXPR span<element_type, Count> first() const noexcept
    {
        static_assert(Count >= 0, "Count must be >= 0 in span::first()");
        static_assert(Count <= Extent, "Count out of range in span::first()");
        return {data(), Count};
    }

    template <ptrdiff_t Count>
    inline Vc_CONSTEXPR span<element_type, Count> last() const noexcept
    {
        static_assert(Count >= 0, "Count must be >= 0 in span::last()");
        static_assert(Count <= Extent, "Count out of range in span::last()");
        return {data() + size() - Count, Count};
    }

    Vc_CONSTEXPR span<element_type, dynamic_extent> first(index_type _count) const noexcept
    {
        Vc_ASSERT(("Count out of range in span::first(count)",
                   _count >= 0 && _count <= size()));
        return {data(), _count};
    }

    Vc_CONSTEXPR span<element_type, dynamic_extent> last(index_type _count) const noexcept
    {
        Vc_ASSERT(
            ("Count out of range in span::last(count)", _count >= 0 && _count <= size()));
        return {data() + size() - _count, _count};
    }

#ifndef Vc_MSVC
    // MSVC 190024215 fails with "error C2059: syntax error: '<end Parse>'" somewhere in
    // this file.  Unless someone needs this function on MSVC, I don't see a reason to
    // invest time into working around their bugs.
    template <ptrdiff_t Offset, ptrdiff_t Count = dynamic_extent>
    inline Vc_CONSTEXPR auto subspan() const noexcept
        -> span<element_type, Count != dynamic_extent ? Count : Extent - Offset>
    {
        Vc_ASSERT(
            ("Offset out of range in span::subspan()", Offset >= 0 && Offset <= size()));
        return {data() + Offset, Count == dynamic_extent ? size() - Offset : Count};
    }

    inline Vc_CONSTEXPR span<element_type, dynamic_extent> subspan(
        index_type offset, index_type count = dynamic_extent) const noexcept
    {
        Vc_ASSERT(("Offset out of range in span::subspan(offset, count)",
                   offset >= 0 && offset <= size()));
        Vc_ASSERT(("Count out of range in span::subspan(offset, count)",
                   (count >= 0 && count <= size()) || count == dynamic_extent));
        if (count == dynamic_extent) {
            return {data() + offset, size() - offset};
        }
        Vc_ASSERT(("count + offset out of range in span::subspan(offset, count)",
                   offset + count <= size()));
        return {data() + offset, count};
    }
#endif  // Vc_MSVC

    Vc_CONSTEXPR index_type size() const noexcept { return Extent; }
    Vc_CONSTEXPR index_type size_bytes() const noexcept
    {
        return Extent * sizeof(element_type);
    }
    Vc_CONSTEXPR bool empty() const noexcept { return Extent == 0; }

    Vc_CONSTEXPR reference operator[](index_type _idx) const noexcept
    {
        Vc_ASSERT(("span<T,N>[] index out of bounds", _idx >= 0 && _idx < size()));
        return data_[_idx];
    }

    Vc_CONSTEXPR reference operator()(index_type _idx) const noexcept
    {
        Vc_ASSERT(("span<T,N>() index out of bounds", _idx >= 0 && _idx < size()));
        return data_[_idx];
    }

    Vc_CONSTEXPR pointer data() const noexcept { return data_; }

    // [span.iter], span iterator support
    Vc_CONSTEXPR iterator begin() const noexcept { return iterator(data()); }
    Vc_CONSTEXPR iterator end() const noexcept { return iterator(data() + size()); }
    Vc_CONSTEXPR const_iterator cbegin() const noexcept { return const_iterator(data()); }
    Vc_CONSTEXPR const_iterator cend() const noexcept
    {
        return const_iterator(data() + size());
    }
    Vc_CONSTEXPR reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    Vc_CONSTEXPR reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
    Vc_CONSTEXPR const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }
    Vc_CONSTEXPR const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    Vc_CONSTEXPR void swap(span& _other) noexcept
    {
        pointer _p = data_;
        data_ = _other.data_;
        _other.data_ = _p;
    }

#ifdef __cpp_lib_byte
    span<const std::byte, Extent * sizeof(element_type)> _as_bytes() const noexcept
    {
        return {reinterpret_cast<const std::byte*>(data()), size_bytes()};
    }

    span<std::byte, Extent * sizeof(element_type)> _as_writeable_bytes() const noexcept
    {
        return {reinterpret_cast<std::byte*>(data()), size_bytes()};
    }
#endif  // __cpp_lib_byte

private:
    pointer data_;
};

template <typename T> class span<T, dynamic_extent>
{
private:
public:
    //  constants and types
    using element_type = T;
    using value_type = typename std::remove_cv<T>::type;
    using index_type = ptrdiff_t;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;  // not in standard
    using reference = T&;
    using const_reference = const T&;  // not in standard
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr index_type extent = dynamic_extent;

    // [span.cons], span constructors, copy, assignment, and destructor
    Vc_CONSTEXPR span() noexcept : data_{nullptr}, size_{0} {}

    Vc_CONSTEXPR span(const span&) noexcept = default;
    Vc_CONSTEXPR span& operator=(const span&) noexcept = default;

    Vc_CONSTEXPR span(pointer _ptr, index_type _count) : data_{_ptr}, size_{_count} {}
    Vc_CONSTEXPR span(pointer _f, pointer _l) : data_{_f}, size_{distance(_f, _l)} {}

    template <size_t Sz>
    inline Vc_CONSTEXPR span(element_type (&_arr)[Sz]) noexcept : data_{_arr}, size_{Sz}
    {
    }

    template <size_t Sz>
    inline Vc_CONSTEXPR span(array<value_type, Sz>& _arr) noexcept
        : data_{_arr.data()}, size_{Sz}
    {
    }

    template <size_t Sz>
    inline Vc_CONSTEXPR span(const array<value_type, Sz>& _arr) noexcept
        : data_{_arr.data()}, size_{Sz}
    {
    }

    template <class Container>
    inline Vc_CONSTEXPR span(
        Container& _c,
        typename std::enable_if<_is_span_compatible_container<Container, T>::value,
                                std::nullptr_t>::type = nullptr)
        : data_{_std_data(_c)}, size_{index_type(_std_size(_c))}
    {
    }

    template <class Container>
    inline Vc_CONSTEXPR span(
        const Container& _c,
        typename std::enable_if<_is_span_compatible_container<const Container, T>::value,
                                std::nullptr_t>::type = nullptr)
        : data_{_std_data(_c)}, size_{index_type(_std_size(_c))}
    {
    }

    template <class OtherElementType, ptrdiff_t OtherExtent>
    inline Vc_CONSTEXPR span(
        const span<OtherElementType, OtherExtent>& _other,
        typename std::enable_if<
            std::is_convertible<OtherElementType (*)[], element_type (*)[]>::value,
            std::nullptr_t>::type = nullptr) noexcept
        : data_{_other.data()}, size_{_other.size()}
    {
    }

    //    ~span() noexcept = default;

    template <ptrdiff_t Count>
    inline Vc_CONSTEXPR span<element_type, Count> first() const noexcept
    {
        static_assert(Count >= 0, "");
        Vc_ASSERT(("Count out of range in span::first()", Count <= size()));
        return {data(), Count};
    }

    template <ptrdiff_t Count>
    inline Vc_CONSTEXPR span<element_type, Count> last() const noexcept
    {
        static_assert(Count >= 0, "");
        Vc_ASSERT(("Count out of range in span::last()", Count <= size()));
        return {data() + size() - Count, Count};
    }

    Vc_CONSTEXPR span<element_type, dynamic_extent> first(index_type _count) const noexcept
    {
        Vc_ASSERT(("Count out of range in span::first(count)",
                   _count >= 0 && _count <= size()));
        return {data(), _count};
    }

    Vc_CONSTEXPR span<element_type, dynamic_extent> last(index_type _count) const noexcept
    {
        Vc_ASSERT(
            ("Count out of range in span::last(count)", _count >= 0 && _count <= size()));
        return {data() + size() - _count, _count};
    }

    template <ptrdiff_t Offset, ptrdiff_t Count = dynamic_extent>
    inline Vc_CONSTEXPR span<T, dynamic_extent> subspan() const noexcept
    {
        Vc_ASSERT(
            ("Offset out of range in span::subspan()", Offset >= 0 && Offset <= size()));
        Vc_ASSERT(("Count out of range in span::subspan()",
                   Count == dynamic_extent || Offset + Count <= size()));
        return {data() + Offset, Count == dynamic_extent ? size() - Offset : Count};
    }

    Vc_CONSTEXPR span<element_type, dynamic_extent> inline subspan(
        index_type _offset, index_type _count = dynamic_extent) const noexcept
    {
        Vc_ASSERT(("Offset out of range in span::subspan(offset, count)",
                   _offset >= 0 && _offset <= size()));
        Vc_ASSERT(("count out of range in span::subspan(offset, count)",
                   (_count >= 0 && _count <= size()) || _count == dynamic_extent));
        if (_count == dynamic_extent)
            return {data() + _offset, size() - _offset};
        Vc_ASSERT(("Offset + count out of range in span::subspan(offset, count)",
                   _offset + _count <= size()));
        return {data() + _offset, _count};
    }

    Vc_CONSTEXPR index_type size() const noexcept { return size_; }
    Vc_CONSTEXPR index_type size_bytes() const noexcept
    {
        return size_ * sizeof(element_type);
    }
    Vc_CONSTEXPR bool empty() const noexcept { return size_ == 0; }

    Vc_CONSTEXPR reference operator[](index_type _idx) const noexcept
    {
        Vc_ASSERT(("span<T>[] index out of bounds", _idx >= 0 && _idx < size()));
        return data_[_idx];
    }

    Vc_CONSTEXPR reference operator()(index_type _idx) const noexcept
    {
        Vc_ASSERT(("span<T>() index out of bounds", _idx >= 0 && _idx < size()));
        return data_[_idx];
    }

    Vc_CONSTEXPR pointer data() const noexcept { return data_; }

    // [span.iter], span iterator support
    Vc_CONSTEXPR iterator begin() const noexcept { return iterator(data()); }
    Vc_CONSTEXPR iterator end() const noexcept { return iterator(data() + size()); }
    Vc_CONSTEXPR const_iterator cbegin() const noexcept { return const_iterator(data()); }
    Vc_CONSTEXPR const_iterator cend() const noexcept
    {
        return const_iterator(data() + size());
    }
    Vc_CONSTEXPR reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    Vc_CONSTEXPR reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
    Vc_CONSTEXPR const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }
    Vc_CONSTEXPR const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    Vc_CONSTEXPR void swap(span& _other) noexcept
    {
        pointer _p = data_;
        data_ = _other.data_;
        _other.data_ = _p;

        index_type _sz = size_;
        size_ = _other.size_;
        _other.size_ = _sz;
    }

#ifdef __cpp_lib_byte
// Disable _as_bytes() for older MSVC versions as it leads to a compilation error due to a compiler bug.
// When parsing the return type, MSVC will instantiate the primary template of span<> and static_assert().
#if _MSC_VER > 1928
    span<const std::byte, dynamic_extent> _as_bytes() const noexcept
    {
        return {reinterpret_cast<const std::byte*>(data()), size_bytes()};
    }

    span<std::byte, dynamic_extent> _as_writeable_bytes() const noexcept
    {
        return {reinterpret_cast<std::byte*>(data()), size_bytes()};
    }
#endif
#endif  // __cpp_lib_byte

private:
    pointer data_;
    index_type size_;
};

template <class T1, ptrdiff_t Extent1, class T2, ptrdiff_t Extent2>
Vc_CONSTEXPR bool operator==(const span<T1, Extent1>& lhs, const span<T2, Extent2>& rhs)
{
    return equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <class T1, ptrdiff_t Extent1, class T2, ptrdiff_t Extent2>
Vc_CONSTEXPR bool operator!=(const span<T1, Extent1>& lhs, const span<T2, Extent2>& rhs)
{
    return !(rhs == lhs);
}

template <class T1, ptrdiff_t Extent1, class T2, ptrdiff_t Extent2>
Vc_CONSTEXPR bool operator<(const span<T1, Extent1>& lhs, const span<T2, Extent2>& rhs)
{
    return lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <class T1, ptrdiff_t Extent1, class T2, ptrdiff_t Extent2>
Vc_CONSTEXPR bool operator<=(const span<T1, Extent1>& lhs, const span<T2, Extent2>& rhs)
{
    return !(rhs < lhs);
}

template <class T1, ptrdiff_t Extent1, class T2, ptrdiff_t Extent2>
Vc_CONSTEXPR bool operator>(const span<T1, Extent1>& lhs, const span<T2, Extent2>& rhs)
{
    return rhs < lhs;
}

template <class T1, ptrdiff_t Extent1, class T2, ptrdiff_t Extent2>
Vc_CONSTEXPR bool operator>=(const span<T1, Extent1>& lhs, const span<T2, Extent2>& rhs)
{
    return !(lhs < rhs);
}

//  as_bytes & as_writeable_bytes
template <class T, ptrdiff_t Extent>
auto as_bytes(span<T, Extent> _s) noexcept -> decltype(_s._as_bytes())
{
    return _s._as_bytes();
}

template <class T, ptrdiff_t Extent>
auto as_writeable_bytes(span<T, Extent> _s) noexcept ->
    typename std::enable_if<!std::is_const<T>::value,
                            decltype(_s._as_writeable_bytes())>::type
{
    return _s._as_writeable_bytes();
}

template <class T, ptrdiff_t Extent>
Vc_CONSTEXPR void swap(span<T, Extent>& lhs, span<T, Extent>& rhs) noexcept
{
    lhs.swap(rhs);
}

#undef Vc_CONSTEXPR

//  Deduction guides
#ifdef __cpp_deduction_guides
template <class T, size_t Sz> span(T (&)[Sz])->span<T, Sz>;

template <class T, size_t Sz> span(array<T, Sz>&)->span<T, Sz>;

template <class T, size_t Sz> span(const array<T, Sz>&)->span<const T, Sz>;

template <class Container> span(Container&)->span<typename Container::value_type>;

template <class Container>
span(const Container&)->span<const typename Container::value_type>;
#endif  // __cpp_deduction_guides

}  // namespace Common

/**
 * \ingroup Containers
 * \headerfile span.h <Vc/span>
 *
 * An adapted `std::span` with additional subscript operators supporting gather and scatter operations.
 *
 * The [std::span](https://en.cppreference.com/w/cpp/container/span) documentation applies.
 *
 * Example:
 * \code
 * struct Point {
 *   float x, y;
 * };
 * Point data[100];
 * // initialize values in data
 *
 * Vc::span<Point, 100> view(data);
 * float_v::IndexType indexes = ...;  // values between 0-99
 * float_v x = view[indexes][&Point::x];
 * float_v y = view[indexes][&Point::y];
 * \endcode
 */
template <typename T, ptrdiff_t Extent = dynamic_extent>
using span = Common::AdaptSubscriptOperator<Common::span<T, Extent>>;

namespace Traits
{
template <typename T, ptrdiff_t Extent>
struct has_contiguous_storage_impl<Vc::span<T, Extent>> : public std::true_type {
};
template <typename T, ptrdiff_t Extent>
struct has_contiguous_storage_impl<Vc::Common::span<T, Extent>> : public std::true_type {
};
}  // namespace Traits

}  // namespace Vc_VERSIONED_NAMESPACE

#endif  // VC_COMMON_SPAN_H_
