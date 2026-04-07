# Dockerfile.dev
FROM ubuntu:24.04

# avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV CUNIT_VERSION=2.2.3
ENV CUNIT_RELEASE_BASENAME=cunit-2.2-3

# install build tools, runtimes and misc deps
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      build-essential \
      curl \
      ca-certificates \
      git \
      python3 \
      python3-pip \
      nodejs \
      npm \
      automake autoconf libtool gfortran flex bison scons swig doxygen graphviz libgraphviz-dev python3-dev libgtk-3-dev python3-gi python3-numpy liblapack-dev libblas-dev libsundials-dev coinor-libipopt-dev libmumps-seq-dev libpcre3-dev python3-packaging \
    && rm -rf /var/lib/apt/lists/*

# install a current Node.js (v22 LTS)
RUN apt-get update \
 && apt-get install -y curl ca-certificates \
 && curl -fsSL https://deb.nodesource.com/setup_22.x | bash - \
 && apt-get install -y nodejs \
 && rm -rf /var/lib/apt/lists/*

# install Codex CLI
RUN npm install -g @openai/codex

# install GNU patch and link apply_patch
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      patch \
    && rm -rf /var/lib/apt/lists/*

RUN ln -s "$(which patch)" /usr/local/bin/apply_patch

WORKDIR /work

# build and install CUnit from the GitHub release tarball
RUN curl -L --retry 3 -o "/tmp/${CUNIT_RELEASE_BASENAME}.tar.gz" \
      "https://github.com/ascend4/cunit/releases/download/v${CUNIT_VERSION}/${CUNIT_RELEASE_BASENAME}.tar.gz" \
  && tar -xzf "/tmp/${CUNIT_RELEASE_BASENAME}.tar.gz" -C /tmp \
  && cd "/tmp/${CUNIT_RELEASE_BASENAME}" \
  && ./configure --prefix=/usr/local \
  && make -j"$(nproc)" \
  && make install \
  && rm -rf "/tmp/${CUNIT_RELEASE_BASENAME}" "/tmp/${CUNIT_RELEASE_BASENAME}.tar.gz"

# codex binary, apply_patch and CUnit are in relevant path vars
ENV PATH="/usr/local/bin:/opt/codex:${PATH}"
ENV LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH}"

USER ubuntu
RUN mkdir -p ~/.local
USER root

# by default drop in to a shell
ENTRYPOINT [ "bash" ]
