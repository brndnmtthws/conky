ARG IMAGE=registry.gitlab.com/brndnmtthws-oss/conky
FROM ${IMAGE}/builder/centos7-base:latest

RUN yum -y -q install \
    llvm-toolset-7-clang \
    llvm-toolset-7-llvm \
    llvm-toolset-7-clang-tools-extra \
    && yum clean all
