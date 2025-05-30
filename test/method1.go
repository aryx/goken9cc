// errchk $G $D/$F.go

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

type T struct { }
func (t *T) M(int, string)	// GCCGO_ERROR "previous"
func (t *T) M(int, float) { }   // ERROR "redeclared|redefinition"

func f(int, string)	// GCCGO_ERROR "previous"
func f(int, float) { }  // ERROR "redeclared|redefinition"

func g(a int, b string)  // GCCGO_ERROR "previous"
func g(a int, c string)  // ERROR "redeclared|redefinition"
