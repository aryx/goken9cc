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

.PHONY: test
test:
	cd tests/s/mini; mk; ./hello_linux_amd64.exe; ./hello_linux_386.exe

hellotest:
	echo TODO

build-docker: build-mkrc

build-gosrc:
	docker build -f Dockerfile.gosrc -t "goken9cc-gosrc" .
build-alpine:
	docker build -f Dockerfile.alpine -t "goken9cc-alpine" .
build-mkrc:
	docker build -f Dockerfile.mkrc -t "goken9cc-mkrc" .

###############################################################################
# Go tests
###############################################################################

gotest:
	cd src; ./run.bash

#TODO: hello_web but more complicated to test and hello_draw
#TODO: use cmp.out to ensure the output is right
hellogotest:
	cd tests; make; ./hello_go; ./hello_unicode; ./hello_goroutine

###############################################################################
# Developer targets
###############################################################################

# See https://github.com/aryx/codemap and https://github.com/aryx/fork-efuns
visual:
	codemap -screen_size 3 -efuns_client efuns_client -emacs_client /dev/null .
