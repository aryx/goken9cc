TOP=.
<$TOP/mkconfig
<$TOP/mkfiles/$objtype/mkfile

DIRS=\
  src/lib9 src/libbio src/libmach \
  lib_strings/libregexp lib_strings/libstring lib_strings/libflate \
  generators/lex/liblex/ generators \
  mk rc \
  src/cmd/nm src/cmd/ar \
  src/cmd/cc \
  assemblers/5a linkers/5l   machines/5i \
  src/cmd/5l src/cmd/5a src/cmd/5c \
  src/cmd/5l_ src/cmd/5a_ src/cmd/5c_ \
  src/cmd/7l src/cmd/7a src/cmd/7c \
  assemblers/8a   linkers/8l \
  src/cmd/8l src/cmd/8a src/cmd/8c \
  src/cmd/6l src/cmd/6a src/cmd/6c \
  assemblers/va linkers/vl compilers/vc   machines/vi \
  src/cmd/ia src/cmd/il src/cmd/ic \
  src/cmd/prof src/cmd/cov \
  debuggers/acid \
  utilities typesetting \

<$TOP/mkfiles/mkdirs

test:
	cd tests; mk test


#TODO: LPDIRS like in principia
sync:VQ:
	echo TODO
