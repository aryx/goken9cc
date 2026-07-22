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
RUN apt-get install -y --no-install-recommends gcc libc6-dev

# Now let's build from source
WORKDIR /src
COPY . .

# Small shell script (not GNU autoconf) to detect arch and generate ./mkconfig
RUN ./configure

# The script below obviously builds 'mk' but also:
# - 'rc', which is called by 'mk'
# - 'ed', which is used by the mkenam scripts run during the build
RUN ./scripts/build-mk.sh
# copy ./ROOT/<arch>/bin/{mk,rc,ed} to ./bin/
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

ENV GOOS="linux"
ENV PATH="/src/ROOT/amd64/bin:/src/ROOT/arm64/bin:${PATH}"

###############################################################################
# Stage2: Just binaries (on amd64/arm64)
###############################################################################
# TODO: copy just /src/ROOT/*/bin/ and maybe /src/ROOT/etc? 

###############################################################################
# Test (on amd64/arm64)
###############################################################################

FROM build AS test

#old: Setup for 386/arm32 tests
# RUN dpkg --add-architecture i386 # so we can run also hello_linux_386.exe
# RUN dpkg --add-architecture armhf
# RUN apt-get update # needed otherwise can't find any package
# RUN apt-get install -y libc6:i386 libc6:armhf
# RUN apt install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf

# qemu-user provides the qemu-xxx per-arch emulator binaries used by
# scripts/qemu-runner. We deliberately do NOT install qemu-user-binfmt:
# it registers qemu as a kernel binfmt_misc interpreter so foreign ELF
# binaries can be run directly as ./foo, but that registration is
# host-global, invisible from inside the container, and unreliable
# across CI setups (see scripts/qemu-runner for the history, e.g. it
# used to make mips binaries run under qemu-mipsn32 instead of
# qemu-mips). Tests instead invoke the right qemu-xxx explicitly.
RUN apt-get install -y qemu-user

# Run tests
RUN mk test

###############################################################################
# Test on amd64/arm64 using the 386/arm plan9 toolchain (8c/5c)
###############################################################################

FROM build AS principia
#TODO? wanted --no-install-recommends but then git does not work so well
RUN apt-get install -y git
RUN git clone https://github.com/aryx/principia-softwarica /principia
WORKDIR /principia
#coupling: https://github.com/aryx/principia-softwarica/blob/master/Dockerfile
# 386
RUN cp mkconfig.pc mkconfig
RUN mk && mk install && mk kernel
# arm
RUN cp mkconfig.pi mkconfig
RUN mk && mk install && mk kernel
