###############################################################################
# Overview
###############################################################################
# Build and test goken9cc on Ubuntu using mk/rc

###############################################################################
# Stage1: build
###############################################################################

FROM ubuntu:22.04 AS build

# Setup a basic C dev environment
RUN apt-get update # needed otherwise can't find any package
RUN apt-get install -y --no-install-recommends gcc libc6-dev bison

# Now let's build from source
WORKDIR /src
COPY . .

RUN ./configure

# This actually builds also 'rc', which is called by 'mk', and 'ed'
# which is used by the mkenam script run during the build
RUN ./scripts/build-mk.sh
RUN ./scripts/promote-mk.sh
# coupling: env.sh
ENV PATH="/src/bin:${PATH}"
ENV MKSHELL="/src/bin/rc"
#alt: PLAN9="/src" and use more #9/... paths in the code like in plan9port
ENV RCMAIN="/src/etc/rcmain.unix"
ENV YACCPAR="/src/etc/yaccpar"

RUN mk
RUN mk install

###############################################################################
# Stage2: Test
###############################################################################

FROM build AS test

# Setup for 386 tests
RUN dpkg --add-architecture i386 # so we can run also hello_linux_386.exe
RUN apt-get update # needed otherwise can't find any package
RUN apt-get install -y libc6:i386
ENV GOOS="linux"
ENV PATH="/src/ROOT/amd64/bin:${PATH}"
# Run tests
RUN mk test
