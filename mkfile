TOP=.
<$TOP/mkconfig
<$TOP/mkfiles/$objtype/mkfile

DIRS=\
  lib_core/lib9 lib_core/libbio \
  lib_strings/libregexp lib_strings/libstring lib_strings/libflate \
  generators/lex/liblex/ generators \
  mk rc \
  debuggers/libmach linkers/ar \
  src/libmach src/cmd/nm src/cmd/ar \
  compilers/cc src/cmd/cc compilers/pcc \
  assemblers/5ak linkers/5lk compilers/5ck   machines/5i \
  src/cmd/5l src/cmd/5a src/cmd/5c \
  linkers/5l9 assemblers/5a9 compilers/5c9 \
  linkers/7l assemblers/7a compilers/7c \
  assemblers/8ak linkers/8lk compilers/8ck \
  src/cmd/8l src/cmd/8a src/cmd/8c \
  src/cmd/6l src/cmd/6a src/cmd/6c \
  assemblers/va linkers/vl compilers/vc   machines/vi \
  assemblers/ia linkers/il compilers/ic \
  src/cmd/prof src/cmd/cov \
  debuggers/acid \
  utilities typesetting \

<$TOP/mkfiles/mkdirs

test:
	cd tests; mk test


#TODO: LPDIRS like in principia
sync:VQ:
	echo TODO
