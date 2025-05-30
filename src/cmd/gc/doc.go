// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*

Gc is the generic label for the family of Go compilers
that function as part of the (modified) Plan 9 tool chain.  The C compiler
documentation at

	http://plan9.bell-labs.com/sys/doc/comp.pdf     (Tools overview)
	http://plan9.bell-labs.com/sys/doc/compiler.pdf (C compiler architecture)

gives the overall design of the tool chain.  Aside from a few adapted pieces,
such as the optimizer, the Go compilers are wholly new programs.

The compiler reads in a set of Go files, typically suffixed ".go".  They
must all be part of one package.  The output is a single intermediate file
representing the "binary assembly" of the compiled package, ready as input
for the linker (6l, etc.).

The generated files contain type information about the symbols exported by
the package and about types used by symbols imported by the package from
other packages. It is therefore not necessary when compiling client C of
package P to read the files of P's dependencies, only the compiled output
of P.

Usage: 6g [flags] *.go  (or 8g or 5g)

Flags:
	-o file
		output file, default 6.out for 6g, etc.
	-e
		normally the compiler quits after 10 errors; -e prints all errors
	-L
		show entire file path when printing line numbers in errors
	-I dir1 -I dir2
		add dir1 and dir2 to the list of paths to check for imported packages
	-N
		disable optimization
	-S
		write assembly language text to standard output
	-V
		print the compiler version

There are also a number of debugging flags; run the command with no arguments
to get a usage message.

*/
package documentation
