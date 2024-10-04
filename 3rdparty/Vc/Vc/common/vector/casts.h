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
    ///////////////////////////////////////////////////////////////////////////////////////////
    // casts - implemented in simd_cast_caller.tcc

    // implict conversion from compatible Vector<U>
    template <typename U>
    Vc_INTRINSIC_L Vector(
        U &&x,
        enable_if<NEON::is_vector<Traits::decay<U>>::value &&
                  !std::is_same<Vector, Traits::decay<U>>::value &&
                  Traits::is_implicit_cast_allowed<Traits::entry_type_of<U>, T>::value> =
            nullarg) Vc_INTRINSIC_R;

    // static_cast from other types
    template <typename U>
    Vc_INTRINSIC_L explicit Vector(U &&x,
                                   enable_if<Traits::is_simd_vector<U>::value &&
                                             !std::is_same<Vector, Traits::decay<U>>::value>>
                                   = nullarg) Vc_INTRINSIC_R;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // broadcast
    Vc_INTRINSIC Vector(EntryType a);
    template <typename U>
    Vc_INTRINSIC Vector(
        U a,
        typename std::enable_if<std::is_same<U, int>::value && !std::is_same<U, EntryType>::value,
                                void *>::type = nullptr)
        : Vector(static_cast<EntryType>(a))
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // explicitly forbid construction from initializer_list
    explicit Vector(std::initializer_list<EntryType>)
    {
        static_assert(std::is_same<EntryType, void>::value,
                      "A SIMD vector object cannot be initialized from an initializer list "
                      "because the number of entries in the vector is target-dependent.");
    }

// vim: foldmethod=marker
