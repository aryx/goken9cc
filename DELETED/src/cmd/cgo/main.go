// Copyright 2009 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Cgo; see gmp.go for an overview.

// TODO(rsc):
//	Emit correct line number annotations.
//	Make 6g understand the annotations.

package main

import (
	"flag"
	"fmt"
	"go/ast"
	"go/token"
	"os"
	"reflect"
	"strings"
)

// A Package collects information about the package we're going to write.
type Package struct {
	PackageName string // name of package
	PackagePath string
	PtrSize     int64
	GccOptions  []string
	Written     map[string]bool
	Name        map[string]*Name    // accumulated Name from Files
	Typedef     map[string]ast.Expr // accumulated Typedef from Files
	ExpFunc     []*ExpFunc          // accumulated ExpFunc from Files
	Decl        []ast.Decl
}

// A File collects information about a single Go input file.
type File struct {
	AST      *ast.File           // parsed AST
	Package  string              // Package name
	Preamble string              // C preamble (doc comment on import "C")
	Ref      []*Ref              // all references to C.xxx in AST
	ExpFunc  []*ExpFunc          // exported functions for this file
	Name     map[string]*Name    // map from Go name to Name
	Typedef  map[string]ast.Expr // translations of all necessary types from C
}

// A Ref refers to an expression of the form C.xxx in the AST.
type Ref struct {
	Name    *Name
	Expr    *ast.Expr
	Context string // "type", "expr", "call", or "call2"
}

func (r *Ref) Pos() token.Position {
	return (*r.Expr).Pos()
}

// A Name collects information about C.xxx.
type Name struct {
	Go       string // name used in Go referring to package C
	Mangle   string // name used in generated Go
	C        string // name used in C
	Define   string // #define expansion
	Kind     string // "const", "type", "var", "func", "not-type"
	Type     *Type  // the type of xxx
	FuncType *FuncType
	AddError bool
	Const    string // constant definition
}

// A ExpFunc is an exported function, callable from C.
// Such functions are identified in the Go input file
// by doc comments containing the line //export ExpName
type ExpFunc struct {
	Func    *ast.FuncDecl
	ExpName string // name to use from C
}

// A Type collects information about a type in both the C and Go worlds.
type Type struct {
	Size       int64
	Align      int64
	C          string
	Go         ast.Expr
	EnumValues map[string]int64
}

// A FuncType collects information about a function type in both the C and Go worlds.
type FuncType struct {
	Params []*Type
	Result *Type
	Go     *ast.FuncType
}

func usage() {
	fmt.Fprint(os.Stderr, "usage: cgo [compiler options] file.go ...\n")
	os.Exit(2)
}

var ptrSizeMap = map[string]int64{
	"386":   4,
	"amd64": 8,
	"arm":   4,
}

func main() {
	flag.Usage = usage
	flag.Parse()
	args := flag.Args()
	if len(args) < 1 {
		usage()
	}

	// Find first arg that looks like a go file and assume everything before
	// that are options to pass to gcc.
	var i int
	for i = len(args); i > 0; i-- {
		if !strings.HasSuffix(args[i-1], ".go") {
			break
		}
	}
	if i == len(args) {
		usage()
	}
	gccOptions, goFiles := args[0:i], args[i:]

	arch := os.Getenv("GOARCH")
	if arch == "" {
		fatal("$GOARCH is not set")
	}
	ptrSize := ptrSizeMap[arch]
	if ptrSize == 0 {
		fatal("unknown $GOARCH %q", arch)
	}

	// Clear locale variables so gcc emits English errors [sic].
	os.Setenv("LANG", "en_US.UTF-8")
	os.Setenv("LC_ALL", "C")
	os.Setenv("LC_CTYPE", "C")

	p := &Package{
		PtrSize:    ptrSize,
		GccOptions: gccOptions,
		Written:    make(map[string]bool),
	}

	for _, input := range goFiles {
		f := new(File)
		// Reset f.Preamble so that we don't end up with conflicting headers / defines
		f.Preamble = ""
		f.ReadGo(input)
		p.Translate(f)
		for _, cref := range f.Ref {
			switch cref.Context {
			case "call", "call2":
				if cref.Name.Kind != "type" {
					break
				}
				*cref.Expr = cref.Name.Type.Go
			}
		}
		if nerrors > 0 {
			os.Exit(2)
		}
		pkg := f.Package
		if dir := os.Getenv("CGOPKGPATH"); dir != "" {
			pkg = dir + "/" + pkg
		}
		p.PackagePath = pkg
		p.writeOutput(f, input)

		p.Record(f)
	}

	p.writeDefs()
}

// Record what needs to be recorded about f.
func (p *Package) Record(f *File) {
	if p.PackageName == "" {
		p.PackageName = f.Package
	} else if p.PackageName != f.Package {
		error(noPos, "inconsistent package names: %s, %s", p.PackageName, f.Package)
	}

	if p.Typedef == nil {
		p.Typedef = f.Typedef
	} else {
		for k, v := range f.Typedef {
			if p.Typedef[k] == nil {
				p.Typedef[k] = v
			} else if !reflect.DeepEqual(p.Typedef[k], v) {
				error(noPos, "inconsistent definitions for C type %s", k)
			}
		}
	}

	if p.Name == nil {
		p.Name = f.Name
	} else {
		for k, v := range f.Name {
			if p.Name[k] == nil {
				p.Name[k] = v
			} else if !reflect.DeepEqual(p.Name[k], v) {
				error(noPos, "inconsistent definitions for C.%s", k)
			}
		}
	}

	if len(f.ExpFunc) > 0 {
		n := len(p.ExpFunc)
		ef := make([]*ExpFunc, n+len(f.ExpFunc))
		copy(ef, p.ExpFunc)
		copy(ef[n:], f.ExpFunc)
		p.ExpFunc = ef
	}

	if len(f.AST.Decls) > 0 {
		n := len(p.Decl)
		d := make([]ast.Decl, n+len(f.AST.Decls))
		copy(d, p.Decl)
		copy(d[n:], f.AST.Decls)
		p.Decl = d
	}
}
