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
	cd src; ./run.bash


build-docker:
	docker build -t "goken9cc" .

###############################################################################
# Developer targets
###############################################################################

# See https://github.com/aryx/codemap and https://github.com/aryx/fork-efuns
visual:
	codemap -screen_size 3 -efuns_client efuns_client -emacs_client /dev/null .
