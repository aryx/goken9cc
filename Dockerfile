# Build and test goken9cc on Ubuntu Linux (x86_64 and glibc)

FROM ubuntu:22.04

# Setup a basic C dev environment
RUN apt-get update # needed otherwise can't find any package
RUN apt-get install -y gcc libc6-dev bison make ed
# for some tests to pass that requires /etc/services or timezones
RUN apt-get install -y netbase tzdata

# Now let's build from source
WORKDIR /src
COPY . .

#TODO: switch to just configure; make; make install
# so we can disable some tests that don't work inside Docker like syslog
ENV IN_DOCKER=true
RUN cd src; ./all.bash

#TODO: switch to just make test
ENV PATH="$PATH:/src/bin"
RUN make hellogotest
