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

.PHONY: test
test:
	echo TODO

hellotest:
	echo TODO

build-docker:
	docker build -t "goken9cc" .

###############################################################################
# Go tests
###############################################################################

gotest:
	cd src; ./run.bash

hellogotest:
	6g hello.go
	6l -o hello hello.6
	./hello

###############################################################################
# Developer targets
###############################################################################

# See https://github.com/aryx/codemap and https://github.com/aryx/fork-efuns
visual:
	codemap -screen_size 3 -efuns_client efuns_client -emacs_client /dev/null .
