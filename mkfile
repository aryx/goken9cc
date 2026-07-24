TOP=.
<$TOP/mkconfig
<$TOP/mkfiles/$objtype/mkfile

DIRS=\
  lib_boot/lib9 lib_core/libbio \
  lib_strings/libregexp lib_strings/libstring lib_strings/libflate \
  generators/lex/liblex/ generators \
  mk rc \
  lib_toolchain/libmach linkers/ar \
  src/libmach src/cmd/nm src/cmd/ar \
  compilers/cc compilers/cck src/cmd/cc compilers/pcc \
  assemblers/5ak linkers/5lk compilers/5ck   machines/5i \
  assemblers/5a linkers/5l compilers/5c \
  assemblers/7a linkers/7l compilers/7c \
  assemblers/8ak linkers/8lk compilers/8ck \
  assemblers/8a linkers/8l compilers/8c \
  assemblers/6a linkers/6l compilers/6c \
  assemblers/va linkers/vl compilers/vc   machines/vi \
  assemblers/ia linkers/il compilers/ic \
  assemblers/ea linkers/el \
  src/cmd/5l src/cmd/5a src/cmd/5c \
  src/cmd/8l src/cmd/8a src/cmd/8c \
  src/cmd/6l src/cmd/6a src/cmd/6c \
  src/cmd/prof src/cmd/cov \
  debuggers/acid \
  utilities typesetting \

<$TOP/mkfiles/mkdirs

test:
	cd tests; mk test

# macOS-native regression tests (no qemu). 'test_macos' auto-detects the
# host: Apple Silicon (arm64) runs the arm64 tests, Intel (x86_64) runs the
# amd64 ones. Use test_macos_arm64 / test_macos_amd64 to force one.
test_macos:V:
	if(~ `{uname -m} arm64)
		@{ cd tests; mk test_macos_arm64 }
	if not
		@{ cd tests; mk test_macos_amd64 }

test_macos_arm64:V:
	cd tests; mk test_macos_arm64

test_macos_amd64:V:
	cd tests; mk test_macos_amd64

# Windows-native regression tests (run directly, no wine/qemu, since the
# host running this target IS Windows). Mirrors test_macos above.
test_windows:V:
	cd tests; mk test_windows

#TODO: LPDIRS like in principia
sync:VQ:
	echo TODO
