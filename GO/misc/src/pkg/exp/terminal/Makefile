# Copyright 2011 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

include ../../../Make.inc

TARG=exp/terminal
GOFILES=\
	terminal.go\

ifneq ($(GOOS),windows)
GOFILES+=\
	util.go
endif

include ../../../Make.pkg
