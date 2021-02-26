ARG IMAGE=registry.gitlab.com/brndnmtthws-oss/conky
FROM ${IMAGE}/builder/ubuntu-base:latest

RUN wget -q https://apt.llvm.org/llvm-snapshot.gpg.key \
  && apt-key add llvm-snapshot.gpg.key \
  && add-apt-repository 'deb http://apt.llvm.org/focal/ llvm-toolchain-focal-9 main' \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
  clang-9 \
  lldb-9 \
  lld-9 \
  libc++-9-dev \
  libc++abi-9-dev \
  clang-tools-9 \
  clang-format-9 \
  clang-tidy-9 \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*
