###############################################################################
# Overview
###############################################################################
# Build and test goken9cc on Ubuntu using gcc/binutils (and mk/rc)

###############################################################################
# Stage1: build (on amd64/arm64)
###############################################################################

FROM ubuntu:24.04 AS build

# Setup a basic C dev environment
RUN apt-get update # needed otherwise can't find any package
RUN apt-get install -y --no-install-recommends gcc libc6-dev byacc

# Now let's build from source
WORKDIR /src
COPY . .

RUN ./configure

# The build-mk.sh script below actually builds also:
# - 'rc', which is called by 'mk'
# - 'ed', which is used by the mkenam script run during the build
RUN ./scripts/build-mk.sh
RUN ./scripts/promote-mk.sh

# coupling: env.sh
ENV PATH="/src/bin:${PATH}"
ENV MKSHELL="/src/bin/rc"
# mk can leverage multiple processors when NPROC > 1
ARG NPROC
ENV NPROC=${NPROC}
# just to cleanup the artifacts of build-mk.sh (which already
# does some cleaning but better safe than sorry).
RUN mk nuke

# Let's build goken (using mk/rc built in the previous step)
RUN mk
RUN mk install

###############################################################################
# Stage2: Test (on amd64/arm64)
###############################################################################

FROM build AS test

#old: Setup for 386/arm32 tests
# RUN dpkg --add-architecture i386 # so we can run also hello_linux_386.exe
# RUN dpkg --add-architecture armhf
# RUN apt-get update # needed otherwise can't find any package
# RUN apt-get install -y libc6:i386 libc6:armhf
# RUN apt install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf

# qemu-user-binfmt allows to execute binaries directly without
# prepending qemu-xxx before (but does not always work unfortunately).
RUN apt-get install -y qemu-user qemu-user-binfmt

ENV GOOS="linux"
ENV PATH="/src/ROOT/amd64/bin:/src/ROOT/arm64/bin:${PATH}"

# Run tests
RUN mk test
