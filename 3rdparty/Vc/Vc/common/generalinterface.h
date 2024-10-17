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

public:
    ///////////////////////////////////////////////////////////////////////////
    // init to zero
    Vc_INTRINSIC Vector() = default;

    ///////////////////////////////////////////////////////////////////////////
    // types

    ///////////////////////////////////////////////////////////////////////////
    // constants
    static constexpr std::size_t size() { return Size; }

    ///////////////////////////////////////////////////////////////////////////
    // constant Vectors
    explicit Vc_INTRINSIC_L Vector(VectorSpecialInitializerZero) Vc_INTRINSIC_R;
    explicit Vc_INTRINSIC_L Vector(VectorSpecialInitializerOne) Vc_INTRINSIC_R;
    explicit Vc_INTRINSIC_L Vector(VectorSpecialInitializerIndexesFromZero) Vc_INTRINSIC_R;
    static Vc_INTRINSIC Vc_CONST Vector Zero() { return Vector(Vc::Zero); }
    static Vc_INTRINSIC Vc_CONST Vector One() { return Vector(Vc::One); }
    static Vc_INTRINSIC Vc_CONST Vector IndexesFromZero()
    {
        return Vector(Vc::IndexesFromZero);
    }

    ///////////////////////////////////////////////////////////////////////////
    // generator ctor
    template <class G, int = 0,
              class = typename std::enable_if<std::is_convertible<
                  decltype(std::declval<G>()(size_t())), value_type>::value>::type>
    explicit Vector(G &&g) : Vector(generate(std::forward<G>(g)))
    {
    }

// vim: foldmethod=marker
