/*  This file is part of the Vc library. {{{
Copyright © 2012-2015 Matthias Kretz <kretz@kde.org>

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

-------------------------------------------------------------------

The exp implementation is derived from Cephes, which carries the
following Copyright notice:

Cephes Math Library Release 2.2:  June, 1992
Copyright 1984, 1987, 1989 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140

}}}*/

#ifdef Vc_COMMON_MATH_H_INTERNAL

constexpr float log2_e = 1.44269504088896341f;

// These constants are adjusted to account for single-precision floating point.
// The original are for double precision:
// 
// constexpr float MAXLOGF = 88.72283905206835f;
// constexpr float MINLOGF = -103.278929903431851103f; /* log(2^-149) */

constexpr float MAXLOGF = 88.722831726074219f; /* log(2^127.99998474121094f) */
constexpr float MINLOGF = -88.029685974121094f; /* log(2^-126.99999237060547f) */
constexpr float MAXNUMF = 3.4028234663852885981170418348451692544e38f;

template <typename Abi, typename = enable_if<std::is_same<Abi, VectorAbi::Sse>::value ||
                                             std::is_same<Abi, VectorAbi::Avx>::value>>
inline Vector<float, detail::not_fixed_size_abi<Abi>> exp(Vector<float, Abi> x)
{
    using V = Vector<float, Abi>;
    typedef typename V::Mask M;
    typedef Detail::Const<float, Abi> C;

        const M overflow  = x > MAXLOGF;
        const M underflow = x < MINLOGF;

        // log₂(eˣ) = x * log₂(e) * log₂(2)
        //          = log₂(2^(x * log₂(e)))
        // => eˣ = 2^(x * log₂(e))
        // => n  = ⌊x * log₂(e) + ½⌋
        // => y  = x - n * ln(2)       | recall that: ln(2) * log₂(e) == 1
        // <=> eˣ = 2ⁿ * eʸ
        V z = floor(C::log2_e() * x + 0.5f);
        const auto n = static_cast<Vc::SimdArray<int, V::Size>>(z);
        x -= z * C::ln2_large();
        x -= z * C::ln2_small();

        /* Theoretical peak relative error in [-0.5, +0.5] is 4.2e-9. */
        z = ((((( 1.9875691500E-4f  * x
                + 1.3981999507E-3f) * x
                + 8.3334519073E-3f) * x
                + 4.1665795894E-2f) * x
                + 1.6666665459E-1f) * x
                + 5.0000001201E-1f) * (x * x)
                + x
                + 1.0f;

        x = ldexp(z, n); // == z * 2ⁿ

        x(overflow) = std::numeric_limits<typename V::EntryType>::infinity();
        x.setZero(underflow);

        return x;
    }

#endif // Vc_COMMON_MATH_H_INTERNAL
