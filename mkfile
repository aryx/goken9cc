TOP=.
<$TOP/mkconfig
<$TOP/mkfiles/$objtype/mkfile

DIRS=\
  src/lib9 src/libbio src/libmach \
  lib_strings/libregexp lib_strings/libstring \
  src/cmd/mk src/cmd/rc \
  src/cmd/nm src/cmd/ar \
  src/cmd/cc \
  src/cmd/6l src/cmd/6a src/cmd/6c \
  src/cmd/8l src/cmd/8a src/cmd/8c \
  src/cmd/5l src/cmd/5a src/cmd/5c src/cmd/5i \
  src/cmd/va src/cmd/vl src/cmd/vc \
  src/cmd/prof src/cmd/cov \
  utilities

<$TOP/mkfiles/mkdirs

test:
	cd tests; mk test
