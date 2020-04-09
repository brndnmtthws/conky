ARG IMAGE=registry.gitlab.com/brndnmtthws-oss/conky
FROM ${IMAGE}/builder/fedora-29-base:latest

RUN dnf -qy install \
    gcc \
    gcc-c++ \
    && dnf clean all
