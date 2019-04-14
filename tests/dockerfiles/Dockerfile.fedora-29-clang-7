ARG IMAGE=registry.gitlab.com/brndnmtthws-oss/conky
FROM ${IMAGE}/builder/fedora-29-base:latest

RUN dnf -y -q install \
    llvm \
    clang \
    libcxx-devel \
    libcxxabi-devel \
    && curl -Ls https://rpm.nodesource.com/setup_11.x -o npm.sh \
    && bash npm.sh \
    && rm npm.sh \
    && dnf -y -q install npm \
    && dnf clean all \
    && npm install -g lcov-summary \
    && npm cache clean --force
