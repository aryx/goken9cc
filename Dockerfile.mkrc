# Build and test goken9cc on Ubuntu using mk/rc

FROM ubuntu:22.04

# Setup a basic C dev environment
RUN apt-get update # needed otherwise can't find any package
RUN apt-get install -y gcc libc6-dev bison ed

# Now let's build from source
WORKDIR /src
COPY . .

RUN ./scripts/build-mk.sh
RUN ./scripts/promote-mk.sh
# coupling: env.sh
ENV PATH="/src/bin:${PATH}"
ENV MKSHELL="/src/bin/rc"
ENV RCMAIN="/src/etc/rcmain.unix"

RUN ./configure
RUN mk
RUN mk install

# Setup for 386 tests
RUN dpkg --add-architecture i386 # so we can run also hello_linux_386.exe
RUN apt-get update # needed otherwise can't find any package
RUN apt-get install -y libc6:i386

ENV GOOS="linux"
ENV PATH="/src/ROOT/amd64/bin:${PATH}"
# Run tests
RUN mk test
