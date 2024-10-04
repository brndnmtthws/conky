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

#ifndef VC_COMMON_MAKECONTAINER_H_
#define VC_COMMON_MAKECONTAINER_H_

#include "../vector.h"
#include <initializer_list>

namespace Vc_VERSIONED_NAMESPACE
{

    namespace
    {
        template<typename Container, typename T> struct make_container_helper
        {
            static constexpr Container help(std::initializer_list<T> list) { return { list }; }
        };

        template <typename T_, typename Abi, typename Alloc,
                  template <class, class> class Container>
        struct make_container_helper<Container<Vector<T_, Abi>, Alloc>,
                                     typename Vector<T_, Abi>::EntryType> {
            typedef Vector<T_, Abi> V;
            typedef typename V::EntryType T;
            typedef Container<V, Alloc> C;
            static inline C help(std::initializer_list<T> list) {
                const std::size_t size = (list.size() + (V::Size - 1)) / V::Size;
                C v(size);
                auto containerIt = v.begin();
                auto init = std::begin(list);
                const auto initEnd = std::end(list);
                for (std::size_t i = 0; i < size - 1; ++i) {
                    *containerIt++ = V(init, Vc::Unaligned);
                    init += V::Size;
                }
                Vc_ASSERT(all_of(*containerIt == V::Zero()));
                int j = 0;
                while (init != initEnd) {
                    (*containerIt)[j++] = *init++;
                }
                return v;
            }
        };

        template <typename T_, typename Abi, std::size_t N,
                  template <class, std::size_t> class Container>
        struct make_container_helper<Container<Vector<T_, Abi>, N>,
                                     typename Vector<T_, Abi>::EntryType> {
            typedef Vector<T_, Abi> V;
            typedef typename V::EntryType T;
            static constexpr std::size_t size = (N + (V::Size - 1)) / V::Size;
            typedef Container<
                V,
#if defined Vc_CLANG && Vc_CLANG < 0x30700 // TODO: when did Vc_APPLECLANG fix it?
                // clang before 3.7.0 has a bug when returning std::array<__m256x, 1>. So
                // increase it to std::array<__m256x, 2> and fill it with zeros. Better
                // than returning garbage.
                (size == 1 && std::is_same<Abi, VectorAbi::Avx>::value) ? 2 :
#endif
                                                                        size> C;
            static inline C help(std::initializer_list<T> list) {
                Vc_ASSERT(N == list.size())
                Vc_ASSERT(size == (list.size() + (V::Size - 1)) / V::Size)
                C v;
                auto containerIt = v.begin();
                auto init = std::begin(list);
                const auto initEnd = std::end(list);
                for (std::size_t i = 0; i < size - 1; ++i) {
                    *containerIt++ = V(init, Vc::Unaligned);
                    init += V::Size;
                }
                Vc_ASSERT(all_of(*containerIt == V::Zero()));
                int j = 0;
                while (init != initEnd) {
                    (*containerIt)[j++] = *init++;
                }
                return v;
            }
        };
    } // anonymous namespace

    /**
     * \ingroup Containers
     * \headerfile makeContainer.h <Vc/Utils>
     *
     * Construct a container of Vc vectors from a std::initializer_list of scalar entries.
     *
     * \tparam Container The container type to construct.
     * \tparam T The scalar type to use for the initializer_list.
     *
     * \param list An initializer list of arbitrary size. The type of the entries is important!
     * If you pass a list of integers you will get a container filled with Vc::int_v objects.
     * If, instead, you want to have a container of Vc::float_v objects, be sure the include a
     * period (.) and the 'f' postfix in the literals. Alternatively, you can pass the
     * type as second template argument to makeContainer.
     *
     * \return Returns a container of the requested class filled with the minimum number of SIMD
     * vectors to hold the values in the initializer list.
     * If the number of values in \p list does not match the number of values in the
     * returned container object, the remaining values in the returned object will be
     * zero-initialized.
     *
     * Example:
     * \code
     * auto data = Vc::makeContainer<std::vector<float_v>>({ 1.f, 2.f, 3.f, 4.f, 5.f });
     * // data.size() == 5 if float_v::Size == 1 (i.e. Vc_IMPL=Scalar)
     * // data.size() == 2 if float_v::Size == 4 (i.e. Vc_IMPL=SSE)
     * // data.size() == 1 if float_v::Size == 8 (i.e. Vc_IMPL=AVX)
     * \endcode
     */
    template<typename Container, typename T>
    constexpr auto makeContainer(std::initializer_list<T> list) -> decltype(make_container_helper<Container, T>::help(list))
    {
        return make_container_helper<Container, T>::help(list);
    }

    template<typename Container, typename T>
    constexpr auto make_container(std::initializer_list<T> list) -> decltype(makeContainer<Container, T>(list))
    {
        return makeContainer<Container, T>(list);
    }

}  // namespace Vc

#endif // VC_COMMON_MAKECONTAINER_H_
