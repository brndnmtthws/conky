ARG IMAGE=registry.gitlab.com/brndnmtthws-oss/conky
FROM ${IMAGE}/builder/ubuntu-base:latest

RUN wget -q https://apt.llvm.org/llvm-snapshot.gpg.key \
  && apt-key add llvm-snapshot.gpg.key \
  && add-apt-repository ppa:ubuntu-toolchain-r/test \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
  g++-6 \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*
