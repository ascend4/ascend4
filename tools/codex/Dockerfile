# Dockerfile.dev
FROM ubuntu:24.04

# avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

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

# install subversion, GNU patch and link apply_patch
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      subversion \
      patch \
    && rm -rf /var/lib/apt/lists/*

RUN ln -s "$(which patch)" /usr/local/bin/apply_patch

WORKDIR /work

# build and install CUnit from Subversion
RUN svn co https://svn.code.sf.net/p/cunit/code/branches/mingw64 cunit \
  && cd cunit \
  && ./configure --prefix=/usr/local \
  && make -j8 \
  && make install

# make sure python and node modules you add are easily available
ENV PATH="/usr/local/bin:/opt/codex:${PATH}"

USER ubuntu
RUN mkdir -p ~/.local
USER root

# by default drop in to a shell
ENTRYPOINT [ "bash" ]

