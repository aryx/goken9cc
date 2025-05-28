###############################################################################
# Prelude
###############################################################################

###############################################################################
# Main targets
###############################################################################

all:
	cd src; ./make.bash

clean:
	cd src; ./clean.bash
	cd tests; make clean

.PHONY: test
test:
	echo TODO

hellotest:
	echo TODO

build-docker:
	docker build -t "goken9cc" .
build-alpine:
	docker build -f Dockerfile.alpine -t "goken9cc" .

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
