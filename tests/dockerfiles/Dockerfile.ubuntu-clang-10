ARG IMAGE=registry.gitlab.com/brndnmtthws-oss/conky
FROM ${IMAGE}/builder/ubuntu-base:latest

RUN wget -q https://apt.llvm.org/llvm-snapshot.gpg.key \
  && apt-key add llvm-snapshot.gpg.key \
  && add-apt-repository 'deb http://apt.llvm.org/focal/ llvm-toolchain-focal-10 main' \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
  clang-10 \
  lldb-10 \
  lld-10 \
  libc++-10-dev \
  libc++abi-10-dev \
  clang-tools-10 \
  clang-format-10 \
  clang-tidy-10 \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*
