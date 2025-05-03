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

RUN npm install -g @openai/codex

RUN apt-get update

RUN apt-get install -y --no-install-recommends subversion

WORKDIR /work

RUN svn co https://svn.code.sf.net/p/cunit/code/branches/mingw64 cunit \
  && cd cunit \
  && ./configure --prefix=/usr/local \
  && make -j8 \
  && make install

# make sure python and node modules you add are easily available
ENV PATH="/usr/local/bin:/opt/codex:${PATH}"

# by default drop in to a shell
ENTRYPOINT [ "bash" ]

