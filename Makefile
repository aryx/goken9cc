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
	mk test

hellotest:
	echo TODO

build-docker: build-mkrc

build-gosrc:
	docker build -f Dockerfile.golang -t "goken9cc-gosrc" .
build-alpine:
	docker build -f Dockerfile.alpine -t "goken9cc-alpine" .
build-mkrc:
	docker build -f Dockerfile -t "goken9cc-mkrc" .

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
# Developer targets
###############################################################################

# See https://github.com/aryx/codemap and https://github.com/aryx/fork-efuns
visual:
	codemap -screen_size 3 -efuns_client efuns_client -emacs_client /dev/null .
