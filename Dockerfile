# Build and test goken9cc on Ubuntu Linux.

FROM ubuntu:22.04
#alt: alpine:3.21

# Setup a basic C dev environment
RUN apt-get update # needed otherwise can't find any package
RUN apt-get install -y gcc libc6-dev make ed bison

# for some tests to pass that requires /etc/services or timezones
RUN apt-get install -y netbase tzdata

WORKDIR /src

# Now let's build from source
COPY . .

# so we can disable some tests that don't work inside Docker
# like syslog
ENV IN_DOCKER=true

#TODO: switch to just configure; make; make install
RUN cd src; ./all.bash

#TODO:
#ENV PATH="$PATH:/src/pkg/tool/linux_amd64"
#RUN make test
