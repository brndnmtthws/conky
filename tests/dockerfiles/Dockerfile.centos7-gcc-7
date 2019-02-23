ARG IMAGE=registry.gitlab.com/brndnmtthws-oss/conky
FROM ${IMAGE}/builder/centos7-base:latest

RUN yum -y -q install \
    devtoolset-7 \
    && yum clean all
