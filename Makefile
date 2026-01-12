###############################################################################
# Prelude
###############################################################################

###############################################################################
# Main targets
###############################################################################

all:
	mk
install:
	mk install
clean:
	mk clean

test:
	mk test

hellotest:
	echo TODO

# works for both amd64 and arm64
build-docker:
	docker build --tag "padator/goken:"`uname -m` -f Dockerfile --build-arg NPROC=`nproc` --target build .
build-docker-test: 
	docker build -f Dockerfile --build-arg NPROC=`nproc` --tag goken9cc-test --target test .
build-docker-principia: 
	docker build -f Dockerfile --build-arg NPROC=`nproc` --tag goken9cc-principia --target principia .

# need 'docker login -u padator' first with credentials of
# https://hub.docker.com/r/padator/ stored in ~/.docker/config.json
# See ocaml-light/Makefile for more info
push-docker:
	docker push "padator/goken:"`uname -m`
	docker buildx imagetools create --tag padator/goken:latest padator/goken:x86_64 padator/goken:aarch64


# Golang regression testing
build-gosrc:
	docker build -f Dockerfile.golang .
build-alpine:
	docker build -f Dockerfile.alpine .

###############################################################################
# Go tests
###############################################################################

gotest:
	cd src; ./run.bash

#TODO: hello_web.exe but more complicated to test and hello_draw.exe too
#TODO: we should use cmp.out to ensure the output is correct
hellogotest:
	cd tests; make; ./hello_go.exe; ./hello_unicode.exe; ./hello_goroutine.exe

###############################################################################
# Pad's targets
###############################################################################

# See https://github.com/aryx/codemap and https://github.com/aryx/fork-efuns
visual:
	codemap -screen_size 3 -efuns_client efuns_client -emacs_client /dev/null .
