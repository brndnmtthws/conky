/*  This file is part of the Vc library. {{{
Copyright © 2014-2015 Matthias Kretz <kretz@kde.org>

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

// load ctors{{{1
/**
 * Construct a vector from loading its entries from the array at \p mem.
 *
 * \param mem A pointer to data. The pointer must not be aligned on a
 *            MemoryAlignment boundary unless you add the Vc::Aligned flag as a second
 *            argument.
 */
explicit Vc_INTRINSIC Vector(const EntryType *mem)
{
    load(mem);
}
/**
 * Construct a vector from loading its entries from the array at \p mem.
 *
 * \param mem A pointer to data. If \p flags contains the Vc::Aligned flag, the pointer
 *            must be aligned on a MemoryAlignment boundary.
 * \param flags A (combination of) flag object(s), such as Vc::Aligned, Vc::Streaming,
 *              Vc::Unaligned, and/or Vc::PrefetchDefault.
 */
template <typename Flags, typename = enable_if<Traits::is_load_store_flag<Flags>::value>>
explicit Vc_INTRINSIC Vector(const EntryType *mem, Flags flags)
{
    load(mem, flags);
}

template <typename U, typename Flags = DefaultLoadTag,
          typename = enable_if<
              (!std::is_integral<U>::value || !std::is_integral<EntryType>::value ||
               sizeof(EntryType) >= sizeof(U)) &&
              std::is_arithmetic<U>::value &&Traits::is_load_store_flag<Flags>::value>>
explicit Vc_INTRINSIC Vector(const U *x, Flags flags = Flags())
{
    load<U, Flags>(x, flags);
}

// load member functions{{{1
/**
 * Load the vector entries from \p mem, overwriting the previous values.
 *
 * \param mem
 * A pointer to data. The pointer must not be aligned on a MemoryAlignment boundary unless
 * you add the Vc::Aligned flag as a second argument.
 */
Vc_INTRINSIC void load(const EntryType *mem)
{
    load(mem, DefaultLoadTag());
}
/**
 * Load the vector entries from \p mem, overwriting the previous values.
 *
 * \param mem
 * A pointer to data. If \p flags contains the Vc::Aligned flag, the pointer must be
 * aligned on a MemoryAlignment boundary.
 * \param flags
 * A (combination of) flag object(s), such as Vc::Aligned, Vc::Streaming, Vc::Unaligned,
 * and/or Vc::PrefetchDefault.
 */
template <typename Flags>
Vc_INTRINSIC enable_if<Traits::is_load_store_flag<Flags>::value, void>
load(const EntryType *mem, Flags flags)
{
    load<EntryType, Flags>(mem, flags);
}
private:
template <typename U, typename Flags>
struct load_concept : public std::enable_if<
              (!std::is_integral<U>::value || !std::is_integral<EntryType>::value ||
               sizeof(EntryType) >= sizeof(U)) &&
              std::is_arithmetic<U>::value && Traits::is_load_store_flag<Flags>::value, void>
{};

public:
template <typename U, typename Flags = DefaultLoadTag>
Vc_INTRINSIC_L typename load_concept<U, Flags>::type load(const U *mem, Flags = Flags()) Vc_INTRINSIC_R;
//}}}1

// vim: foldmethod=marker
