**Vc is now in maintenance mode and no longer actively developed.
However, we continue to review pull requests with bugfixes from the community.**

**You may be interested in switching to [std-simd](https://github.com/VcDevel/std-simd).**
GCC 11 includes an experimental version of `std::simd` as part of libstdc++, which also works with clang.
Features present in Vc 1.4 and not present in *std-simd* will eventually turn into Vc 2.0,which then depends on *std-simd*.

# Vc: portable, zero-overhead C++ types for explicitly data-parallel programming

Recent generations of CPUs, and GPUs in particular, require data-parallel codes
for full efficiency. Data parallelism requires that the same sequence of
operations is applied to different input data. CPUs and GPUs can thus reduce
the necessary hardware for instruction decoding and scheduling in favor of more
arithmetic and logic units, which execute the same instructions synchronously.
On CPU architectures this is implemented via SIMD registers and instructions.
A single SIMD register can store N values and a single SIMD instruction can
execute N operations on those values. On GPU architectures N threads run in
perfect sync, fed by a single instruction decoder/scheduler. Each thread has
local memory and a given index to calculate the offsets in memory for loads and
stores.

Current C++ compilers can do automatic transformation of scalar codes to SIMD
instructions (auto-vectorization). However, the compiler must reconstruct an
intrinsic property of the algorithm that was lost when the developer wrote a
purely scalar implementation in C++. Consequently, C++ compilers cannot
vectorize any given code to its most efficient data-parallel variant.
Especially larger data-parallel loops, spanning over multiple functions or even
translation units, will often not be transformed into efficient SIMD code.

The Vc library provides the missing link. Its types enable explicitly stating
data-parallel operations on multiple values. The parallelism is therefore added
via the type system. Competing approaches state the parallelism via new control
structures and consequently new semantics inside the body of these control
structures.

Vc is a free software library to ease explicit vectorization of C++ code. It
has an intuitive API and provides portability between different compilers and
compiler versions as well as portability between different vector instruction
sets. Thus an application written with Vc can be compiled for:

* AVX and AVX2
* SSE2 up to SSE4.2 or SSE4a
* Scalar
* ~~AVX-512 (Vc 2 development)~~
* ~~NEON (in development)~~
* ~~NVIDIA GPUs / CUDA (research)~~

After Intel dropped MIC support with ICC 18, Vc 1.4 also removed support for it.

## Examples

### Usage on Compiler Explorer

* [Simdize Example](https://godbolt.org/z/JVEM2j)
* [Total momentum and time stepping of `std::vector<Particle>`](https://godbolt.org/z/JNdkL9)
* [Matrix Example](https://godbolt.org/z/fFEkuX): This uses vertical
  vectorization which does not scale to different vector sizes. However, the
  example is instructive to compare it with similar solutions of other languages
  or libraries.
* [N-vortex solver](https://godbolt.org/z/4o1cg_) showing `simdize`d iteration
  over many `std::vector<float>`. Note how [important the `-march` flag is, compared
  to plain `-mavx2 -mfma`](https://godbolt.org/z/hKiOjr).

### Scalar Product

Let's start from the code for calculating a 3D scalar product using builtin floats:
```cpp
using Vec3D = std::array<float, 3>;
float scalar_product(Vec3D a, Vec3D b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
```
Using Vc, we can easily vectorize the code using the `float_v` type:
```cpp
using Vc::float_v
using Vec3D = std::array<float_v, 3>;
float_v scalar_product(Vec3D a, Vec3D b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
```
The above will scale to 1, 4, 8, 16, etc. scalar products calculated in parallel, depending
on the target hardware's capabilities.

For comparison, the same vectorization using Intel SSE intrinsics is more verbose and uses
prefix notation (i.e. function calls):
```cpp
using Vec3D = std::array<__m128, 3>;
__m128 scalar_product(Vec3D a, Vec3D b) {
  return _mm_add_ps(_mm_add_ps(_mm_mul_ps(a[0], b[0]), _mm_mul_ps(a[1], b[1])),
                    _mm_mul_ps(a[2], b[2]));
}
```
The above will neither scale to AVX, AVX-512, etc. nor is it portable to other SIMD ISAs.

## Build Requirements

cmake >= 3.0

C++11 Compiler:

* GCC >= 4.8.1
* clang >= 3.4
* ICC >= 18.0.5
* Visual Studio 2019 (64-bit target)


## Building and Installing Vc

* Clone Vc and initialize Vc's git submodules:

```sh
git clone https://github.com/VcDevel/Vc.git
cd Vc
git submodule update --init
```

* Create a build directory:

```sh
$ mkdir build
$ cd build
```

* Configure with cmake and add relevant options:

```sh
$ cmake ..
```

Optionally, specify an installation directory:

```sh
$ cmake -DCMAKE_INSTALL_PREFIX=/opt/Vc ..
```

Optionally, include building the unit tests:

```sh
$ cmake -DBUILD_TESTING=ON ..
```

On Windows, if you have multiple versions of Visual Studio installed, you can select one:

```sh
$ cmake -G "Visual Studio 16 2019" ..
```

See `cmake --help` for a list of possible generators.


* Build and install:

```sh
$ cmake --build . -j 16
$ cmake --install . # may require permissions
```

On Windows, you can also open `Vc.sln` in Visual Studio and build/install from the IDE.

## Documentation

The documentation is generated via [doxygen](http://doxygen.org). You can build
the documentation by running `doxygen` in the `doc` subdirectory.
Alternatively, you can find nightly builds of the documentation at:

* [1.4 branch](https://vcdevel.github.io/Vc-1.4/)
* [1.4.4 release](https://vcdevel.github.io/Vc-1.4.4/)
* [1.4.3 release](https://vcdevel.github.io/Vc-1.4.3/)
* [1.4.2 release](https://vcdevel.github.io/Vc-1.4.2/)
* [1.4.1 release](https://vcdevel.github.io/Vc-1.4.1/)
* [1.4.0 release](https://vcdevel.github.io/Vc-1.4.0/)
* [1.3 branch](https://vcdevel.github.io/Vc-1.3/)
* [1.3.0 release](https://vcdevel.github.io/Vc-1.3.0/)
* [1.2.0 release](https://vcdevel.github.io/Vc-1.2.0/)
* [1.1.0 release](https://vcdevel.github.io/Vc-1.1.0/)
* [0.7 branch](https://vcdevel.github.io/Vc-0.7/)

## Publications

* [M. Kretz, "Extending C++ for Explicit Data-Parallel Programming via SIMD
  Vector Types", Goethe University Frankfurt, Dissertation,
  2015.](http://publikationen.ub.uni-frankfurt.de/frontdoor/index/index/docId/38415)
* [M. Kretz and V. Lindenstruth, "Vc: A C++ library for explicit
  vectorization", Software: Practice and Experience,
  2011.](http://dx.doi.org/10.1002/spe.1149)
* [M. Kretz, "Efficient Use of Multi- and Many-Core Systems with Vectorization
  and Multithreading", University of Heidelberg,
  2009.](http://code.compeng.uni-frankfurt.de/attachments/13/Diplomarbeit.pdf)

[Work on integrating the functionality of Vc in the C++ standard library.](
https://github.com/VcDevel/Vc/wiki/ISO-Standardization-of-the-Vector-classes)

## License

Vc is released under the terms of the [3-clause BSD license](http://opensource.org/licenses/BSD-3-Clause).
