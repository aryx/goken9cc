// Copyright 2009 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import (
	"bytes"
	"fmt"
	"go/ast"
	"go/printer"
	"go/token"
	"os"
	"strings"
)

// writeDefs creates output files to be compiled by 6g, 6c, and gcc.
// (The comments here say 6g and 6c but the code applies to the 8 and 5 tools too.)
func (p *Package) writeDefs() {
	// The path for the shared object is slash-free so that ELF loaders
	// will treat it as a relative path.  We rewrite slashes to underscores.
	sopath := "cgo_" + strings.Map(slashToUnderscore, p.PackagePath)
	soprefix := ""
	if os.Getenv("GOOS") == "darwin" {
		// OS X requires its own prefix for a relative path
		soprefix = "@rpath/"
	}

	fgo2 := creat("_cgo_gotypes.go")
	fc := creat("_cgo_defun.c")

	// Write second Go output: definitions of _C_xxx.
	// In a separate file so that the import of "unsafe" does not
	// pollute the original file.
	fmt.Fprintf(fgo2, "// Created by cgo - DO NOT EDIT\n")
	fmt.Fprintf(fgo2, "package %s\n\n", p.PackageName)
	fmt.Fprintf(fgo2, "import \"unsafe\"\n\n")
	fmt.Fprintf(fgo2, "import \"os\"\n\n")
	fmt.Fprintf(fgo2, "type _ unsafe.Pointer\n\n")
	fmt.Fprintf(fgo2, "func _Cerrno(dst *os.Error, x int) { *dst = os.Errno(x) }\n")

	for name, def := range p.Typedef {
		fmt.Fprintf(fgo2, "type %s ", name)
		printer.Fprint(fgo2, def)
		fmt.Fprintf(fgo2, "\n")
	}
	fmt.Fprintf(fgo2, "type _Ctype_void [0]byte\n")

	fmt.Fprintf(fc, cProlog, soprefix, soprefix, soprefix, soprefix, soprefix)

	for _, n := range p.Name {
		if n.Kind != "var" {
			continue
		}
		fmt.Fprintf(fc, "#pragma dynimport ·%s %s \"%s%s.so\"\n", n.Mangle, n.C, soprefix, sopath)
		fmt.Fprintf(fgo2, "var %s ", n.Mangle)
		printer.Fprint(fgo2, &ast.StarExpr{X: n.Type.Go})
		fmt.Fprintf(fgo2, "\n")
	}
	fmt.Fprintf(fc, "\n")

	for _, n := range p.Name {
		if n.Const != "" {
			fmt.Fprintf(fgo2, "const _Cconst_%s = %s\n", n.Go, n.Const)
		}
	}
	fmt.Fprintf(fgo2, "\n")

	for _, n := range p.Name {
		if n.FuncType != nil {
			p.writeDefsFunc(fc, fgo2, n, soprefix, sopath)
		}
	}

	p.writeExports(fgo2, fc)

	fgo2.Close()
	fc.Close()
}

// Construct a gcc struct matching the 6c argument frame.
// Assumes that in gcc, char is 1 byte, short 2 bytes, int 4 bytes, long long 8 bytes.
// These assumptions are checked by the gccProlog.
// Also assumes that 6c convention is to word-align the
// input and output parameters.
func (p *Package) structType(n *Name) (string, int64) {
	var buf bytes.Buffer
	fmt.Fprint(&buf, "struct {\n")
	off := int64(0)
	for i, t := range n.FuncType.Params {
		if off%t.Align != 0 {
			pad := t.Align - off%t.Align
			fmt.Fprintf(&buf, "\t\tchar __pad%d[%d];\n", off, pad)
			off += pad
		}
		fmt.Fprintf(&buf, "\t\t%s p%d;\n", t.C, i)
		off += t.Size
	}
	if off%p.PtrSize != 0 {
		pad := p.PtrSize - off%p.PtrSize
		fmt.Fprintf(&buf, "\t\tchar __pad%d[%d];\n", off, pad)
		off += pad
	}
	if t := n.FuncType.Result; t != nil {
		if off%t.Align != 0 {
			pad := t.Align - off%t.Align
			fmt.Fprintf(&buf, "\t\tchar __pad%d[%d];\n", off, pad)
			off += pad
		}
		qual := ""
		if t.C[len(t.C)-1] == '*' {
			qual = "const "
		}
		fmt.Fprintf(&buf, "\t\t%s%s r;\n", qual, t.C)
		off += t.Size
	}
	if off%p.PtrSize != 0 {
		pad := p.PtrSize - off%p.PtrSize
		fmt.Fprintf(&buf, "\t\tchar __pad%d[%d];\n", off, pad)
		off += pad
	}
	if n.AddError {
		fmt.Fprint(&buf, "\t\tvoid *e[2]; /* os.Error */\n")
		off += 2 * p.PtrSize
	}
	if off == 0 {
		fmt.Fprintf(&buf, "\t\tchar unused;\n") // avoid empty struct
		off++
	}
	fmt.Fprintf(&buf, "\t}\n")
	return buf.String(), off
}

func (p *Package) writeDefsFunc(fc, fgo2 *os.File, n *Name, soprefix, sopath string) {
	name := n.Go
	gtype := n.FuncType.Go
	if n.AddError {
		// Add "os.Error" to return type list.
		// Type list is known to be 0 or 1 element - it's a C function.
		err := &ast.Field{Type: ast.NewIdent("os.Error")}
		l := gtype.Results.List
		if len(l) == 0 {
			l = []*ast.Field{err}
		} else {
			l = []*ast.Field{l[0], err}
		}
		t := new(ast.FuncType)
		*t = *gtype
		t.Results = &ast.FieldList{List: l}
		gtype = t
	}

	// Go func declaration.
	d := &ast.FuncDecl{
		Name: ast.NewIdent(n.Mangle),
		Type: gtype,
	}
	printer.Fprint(fgo2, d)
	fmt.Fprintf(fgo2, "\n")

	if name == "CString" || name == "GoString" || name == "GoStringN" {
		// The builtins are already defined in the C prolog.
		return
	}

	var argSize int64
	_, argSize = p.structType(n)

	// C wrapper calls into gcc, passing a pointer to the argument frame.
	// Also emit #pragma to get a pointer to the gcc wrapper.
	fmt.Fprintf(fc, "#pragma dynimport _cgo%s _cgo%s \"%s%s.so\"\n", n.Mangle, n.Mangle, soprefix, sopath)
	fmt.Fprintf(fc, "void (*_cgo%s)(void*);\n", n.Mangle)
	fmt.Fprintf(fc, "\n")
	fmt.Fprintf(fc, "void\n")
	fmt.Fprintf(fc, "·%s(struct{uint8 x[%d];}p)\n", n.Mangle, argSize)
	fmt.Fprintf(fc, "{\n")
	fmt.Fprintf(fc, "\tcgocall(_cgo%s, &p);\n", n.Mangle)
	if n.AddError {
		// gcc leaves errno in first word of interface at end of p.
		// check whether it is zero; if so, turn interface into nil.
		// if not, turn interface into errno.
		// Go init function initializes ·_Cerrno with an os.Errno
		// for us to copy.
		fmt.Fprintln(fc, `	{
			int32 e;
			void **v;
			v = (void**)(&p+1) - 2;	/* v = final two void* of p */
			e = *(int32*)v;
			v[0] = (void*)0xdeadbeef;
			v[1] = (void*)0xdeadbeef;
			if(e == 0) {
				/* nil interface */
				v[0] = 0;
				v[1] = 0;
			} else {
				·_Cerrno(v, e);	/* fill in v as os.Error for errno e */
			}
		}`)
	}
	fmt.Fprintf(fc, "}\n")
	fmt.Fprintf(fc, "\n")
}

// writeOutput creates stubs for a specific source file to be compiled by 6g
// (The comments here say 6g and 6c but the code applies to the 8 and 5 tools too.)
func (p *Package) writeOutput(f *File, srcfile string) {
	base := srcfile
	if strings.HasSuffix(base, ".go") {
		base = base[0 : len(base)-3]
	}
	base = strings.Map(slashToUnderscore, base)
	fgo1 := creat(base + ".cgo1.go")
	fgcc := creat(base + ".cgo2.c")

	// Write Go output: Go input with rewrites of C.xxx to _C_xxx.
	fmt.Fprintf(fgo1, "// Created by cgo - DO NOT EDIT\n")
	fmt.Fprintf(fgo1, "//line %s:1\n", srcfile)
	printer.Fprint(fgo1, f.AST)

	// While we process the vars and funcs, also write 6c and gcc output.
	// Gcc output starts with the preamble.
	fmt.Fprintf(fgcc, "%s\n", f.Preamble)
	fmt.Fprintf(fgcc, "%s\n", gccProlog)

	for _, n := range f.Name {
		if n.FuncType != nil {
			p.writeOutputFunc(fgcc, n)
		}
	}

	fgo1.Close()
	fgcc.Close()
}

func (p *Package) writeOutputFunc(fgcc *os.File, n *Name) {
	name := n.Mangle
	if name == "_Cfunc_CString" || name == "_Cfunc_GoString" || name == "_Cfunc_GoStringN" || p.Written[name] {
		// The builtins are already defined in the C prolog, and we don't
		// want to duplicate function definitions we've already done.
		return
	}
	p.Written[name] = true

	ctype, _ := p.structType(n)

	// Gcc wrapper unpacks the C argument struct
	// and calls the actual C function.
	fmt.Fprintf(fgcc, "void\n")
	fmt.Fprintf(fgcc, "_cgo%s(void *v)\n", n.Mangle)
	fmt.Fprintf(fgcc, "{\n")
	if n.AddError {
		fmt.Fprintf(fgcc, "\tint e;\n") // assuming 32 bit (see comment above structType)
		fmt.Fprintf(fgcc, "\terrno = 0;\n")
	}
	fmt.Fprintf(fgcc, "\t%s *a = v;\n", ctype)
	fmt.Fprintf(fgcc, "\t")
	if n.FuncType.Result != nil {
		fmt.Fprintf(fgcc, "a->r = ")
	}
	fmt.Fprintf(fgcc, "%s(", n.C)
	for i := range n.FuncType.Params {
		if i > 0 {
			fmt.Fprintf(fgcc, ", ")
		}
		fmt.Fprintf(fgcc, "a->p%d", i)
	}
	fmt.Fprintf(fgcc, ");\n")
	if n.AddError {
		fmt.Fprintf(fgcc, "\t*(int*)(a->e) = errno;\n")
	}
	fmt.Fprintf(fgcc, "}\n")
	fmt.Fprintf(fgcc, "\n")
}

// Write out the various stubs we need to support functions exported
// from Go so that they are callable from C.
func (p *Package) writeExports(fgo2, fc *os.File) {
	if len(p.ExpFunc) == 0 {
		return
	}

	fgcc := creat("_cgo_export.c")
	fgcch := creat("_cgo_export.h")

	fmt.Fprintf(fgcch, "/* Created by cgo - DO NOT EDIT. */\n")
	fmt.Fprintf(fgcch, "%s\n", gccExportHeaderProlog)

	fmt.Fprintf(fgcc, "/* Created by cgo - DO NOT EDIT. */\n")
	fmt.Fprintf(fgcc, "#include \"_cgo_export.h\"\n")

	for _, exp := range p.ExpFunc {
		fn := exp.Func

		// Construct a gcc struct matching the 6c argument and
		// result frame.
		ctype := "struct {\n"
		off := int64(0)
		npad := 0
		if fn.Recv != nil {
			t := p.cgoType(fn.Recv.List[0].Type)
			ctype += fmt.Sprintf("\t\t%s recv;\n", t.C)
			off += t.Size
		}
		fntype := fn.Type
		forFieldList(fntype.Params,
			func(i int, atype ast.Expr) {
				t := p.cgoType(atype)
				if off%t.Align != 0 {
					pad := t.Align - off%t.Align
					ctype += fmt.Sprintf("\t\tchar __pad%d[%d];\n", npad, pad)
					off += pad
					npad++
				}
				ctype += fmt.Sprintf("\t\t%s p%d;\n", t.C, i)
				off += t.Size
			})
		if off%p.PtrSize != 0 {
			pad := p.PtrSize - off%p.PtrSize
			ctype += fmt.Sprintf("\t\tchar __pad%d[%d];\n", npad, pad)
			off += pad
			npad++
		}
		forFieldList(fntype.Results,
			func(i int, atype ast.Expr) {
				t := p.cgoType(atype)
				if off%t.Align != 0 {
					pad := t.Align - off%t.Align
					ctype += fmt.Sprintf("\t\tchar __pad%d[%d]\n", npad, pad)
					off += pad
					npad++
				}
				ctype += fmt.Sprintf("\t\t%s r%d;\n", t.C, i)
				off += t.Size
			})
		if off%p.PtrSize != 0 {
			pad := p.PtrSize - off%p.PtrSize
			ctype += fmt.Sprintf("\t\tchar __pad%d[%d];\n", npad, pad)
			off += pad
			npad++
		}
		if ctype == "struct {\n" {
			ctype += "\t\tchar unused;\n" // avoid empty struct
			off++
		}
		ctype += "\t}"

		// Get the return type of the wrapper function
		// compiled by gcc.
		gccResult := ""
		if fntype.Results == nil || len(fntype.Results.List) == 0 {
			gccResult = "void"
		} else if len(fntype.Results.List) == 1 && len(fntype.Results.List[0].Names) <= 1 {
			gccResult = p.cgoType(fntype.Results.List[0].Type).C
		} else {
			fmt.Fprintf(fgcch, "\n/* Return type for %s */\n", exp.ExpName)
			fmt.Fprintf(fgcch, "struct %s_return {\n", exp.ExpName)
			forFieldList(fntype.Results,
				func(i int, atype ast.Expr) {
					fmt.Fprintf(fgcch, "\t%s r%d;\n", p.cgoType(atype).C, i)
				})
			fmt.Fprintf(fgcch, "};\n")
			gccResult = "struct " + exp.ExpName + "_return"
		}

		// Build the wrapper function compiled by gcc.
		s := fmt.Sprintf("%s %s(", gccResult, exp.ExpName)
		if fn.Recv != nil {
			s += p.cgoType(fn.Recv.List[0].Type).C
			s += " recv"
		}
		forFieldList(fntype.Params,
			func(i int, atype ast.Expr) {
				if i > 0 || fn.Recv != nil {
					s += ", "
				}
				s += fmt.Sprintf("%s p%d", p.cgoType(atype).C, i)
			})
		s += ")"
		fmt.Fprintf(fgcch, "\nextern %s;\n", s)

		fmt.Fprintf(fgcc, "extern _cgoexp_%s(void *, int);\n", exp.ExpName)
		fmt.Fprintf(fgcc, "\n%s\n", s)
		fmt.Fprintf(fgcc, "{\n")
		fmt.Fprintf(fgcc, "\t%s a;\n", ctype)
		if gccResult != "void" && (len(fntype.Results.List) > 1 || len(fntype.Results.List[0].Names) > 1) {
			fmt.Fprintf(fgcc, "\t%s r;\n", gccResult)
		}
		if fn.Recv != nil {
			fmt.Fprintf(fgcc, "\ta.recv = recv;\n")
		}
		forFieldList(fntype.Params,
			func(i int, atype ast.Expr) {
				fmt.Fprintf(fgcc, "\ta.p%d = p%d;\n", i, i)
			})
		fmt.Fprintf(fgcc, "\tcrosscall2(_cgoexp_%s, &a, (int) sizeof a);\n", exp.ExpName)
		if gccResult != "void" {
			if len(fntype.Results.List) == 1 && len(fntype.Results.List[0].Names) <= 1 {
				fmt.Fprintf(fgcc, "\treturn a.r0;\n")
			} else {
				forFieldList(fntype.Results,
					func(i int, atype ast.Expr) {
						fmt.Fprintf(fgcc, "\tr.r%d = a.r%d;\n", i, i)
					})
				fmt.Fprintf(fgcc, "\treturn r;\n")
			}
		}
		fmt.Fprintf(fgcc, "}\n")

		// Build the wrapper function compiled by 6c/8c
		goname := exp.Func.Name.Name
		if fn.Recv != nil {
			goname = "_cgoexpwrap_" + fn.Recv.List[0].Names[0].Name + "_" + goname
		}
		fmt.Fprintf(fc, "#pragma dynexport _cgoexp_%s _cgoexp_%s\n", exp.ExpName, exp.ExpName)
		fmt.Fprintf(fc, "extern void ·%s();\n", goname)
		fmt.Fprintf(fc, "\nvoid\n")
		fmt.Fprintf(fc, "_cgoexp_%s(void *a, int32 n)\n", exp.ExpName)
		fmt.Fprintf(fc, "{\n")
		fmt.Fprintf(fc, "\tcgocallback(·%s, a, n);\n", goname)
		fmt.Fprintf(fc, "}\n")

		// Calling a function with a receiver from C requires
		// a Go wrapper function.
		if fn.Recv != nil {
			fmt.Fprintf(fgo2, "func %s(recv ", goname)
			printer.Fprint(fgo2, fn.Recv.List[0].Type)
			forFieldList(fntype.Params,
				func(i int, atype ast.Expr) {
					fmt.Fprintf(fgo2, ", p%d", i)
					printer.Fprint(fgo2, atype)
				})
			fmt.Fprintf(fgo2, ")")
			if gccResult != "void" {
				fmt.Fprint(fgo2, " (")
				forFieldList(fntype.Results,
					func(i int, atype ast.Expr) {
						if i > 0 {
							fmt.Fprint(fgo2, ", ")
						}
						printer.Fprint(fgo2, atype)
					})
				fmt.Fprint(fgo2, ")")
			}
			fmt.Fprint(fgo2, " {\n")
			fmt.Fprint(fgo2, "\t")
			if gccResult != "void" {
				fmt.Fprint(fgo2, "return ")
			}
			fmt.Fprintf(fgo2, "recv.%s(", exp.Func.Name)
			forFieldList(fntype.Params,
				func(i int, atype ast.Expr) {
					if i > 0 {
						fmt.Fprint(fgo2, ", ")
					}
					fmt.Fprintf(fgo2, "p%d", i)
				})
			fmt.Fprint(fgo2, ")\n")
			fmt.Fprint(fgo2, "}\n")
		}
	}
}

// Call a function for each entry in an ast.FieldList, passing the
// index into the list and the type.
func forFieldList(fl *ast.FieldList, fn func(int, ast.Expr)) {
	if fl == nil {
		return
	}
	i := 0
	for _, r := range fl.List {
		if r.Names == nil {
			fn(i, r.Type)
			i++
		} else {
			for _ = range r.Names {
				fn(i, r.Type)
				i++
			}
		}
	}
}

// Map predeclared Go types to Type.
var goTypes = map[string]*Type{
	"int":        &Type{Size: 4, Align: 4, C: "int"},
	"uint":       &Type{Size: 4, Align: 4, C: "uint"},
	"int8":       &Type{Size: 1, Align: 1, C: "schar"},
	"uint8":      &Type{Size: 1, Align: 1, C: "uchar"},
	"int16":      &Type{Size: 2, Align: 2, C: "short"},
	"uint16":     &Type{Size: 2, Align: 2, C: "ushort"},
	"int32":      &Type{Size: 4, Align: 4, C: "int"},
	"uint32":     &Type{Size: 4, Align: 4, C: "uint"},
	"int64":      &Type{Size: 8, Align: 8, C: "int64"},
	"uint64":     &Type{Size: 8, Align: 8, C: "uint64"},
	"float":      &Type{Size: 4, Align: 4, C: "float"},
	"float32":    &Type{Size: 4, Align: 4, C: "float"},
	"float64":    &Type{Size: 8, Align: 8, C: "double"},
	"complex":    &Type{Size: 8, Align: 8, C: "__complex float"},
	"complex64":  &Type{Size: 8, Align: 8, C: "__complex float"},
	"complex128": &Type{Size: 16, Align: 16, C: "__complex double"},
}

// Map an ast type to a Type.
func (p *Package) cgoType(e ast.Expr) *Type {
	switch t := e.(type) {
	case *ast.StarExpr:
		x := p.cgoType(t.X)
		return &Type{Size: p.PtrSize, Align: p.PtrSize, C: x.C + "*"}
	case *ast.ArrayType:
		if t.Len == nil {
			return &Type{Size: p.PtrSize + 8, Align: p.PtrSize, C: "GoSlice"}
		}
	case *ast.StructType:
		// TODO
	case *ast.FuncType:
		return &Type{Size: p.PtrSize, Align: p.PtrSize, C: "void*"}
	case *ast.InterfaceType:
		return &Type{Size: 3 * p.PtrSize, Align: p.PtrSize, C: "GoInterface"}
	case *ast.MapType:
		return &Type{Size: p.PtrSize, Align: p.PtrSize, C: "GoMap"}
	case *ast.ChanType:
		return &Type{Size: p.PtrSize, Align: p.PtrSize, C: "GoChan"}
	case *ast.Ident:
		// Look up the type in the top level declarations.
		// TODO: Handle types defined within a function.
		for _, d := range p.Decl {
			gd, ok := d.(*ast.GenDecl)
			if !ok || gd.Tok != token.TYPE {
				continue
			}
			for _, spec := range gd.Specs {
				ts, ok := spec.(*ast.TypeSpec)
				if !ok {
					continue
				}
				if ts.Name.Name == t.Name {
					return p.cgoType(ts.Type)
				}
			}
		}
		for name, def := range p.Typedef {
			if name == t.Name {
				return p.cgoType(def)
			}
		}
		if t.Name == "uintptr" {
			return &Type{Size: p.PtrSize, Align: p.PtrSize, C: "uintptr"}
		}
		if t.Name == "string" {
			return &Type{Size: p.PtrSize + 4, Align: p.PtrSize, C: "GoString"}
		}
		if r, ok := goTypes[t.Name]; ok {
			if r.Align > p.PtrSize {
				r.Align = p.PtrSize
			}
			return r
		}
	}
	error(e.Pos(), "unrecognized Go type %v", e)
	return &Type{Size: 4, Align: 4, C: "int"}
}

const gccProlog = `
// Usual nonsense: if x and y are not equal, the type will be invalid
// (have a negative array count) and an inscrutable error will come
// out of the compiler and hopefully mention "name".
#define __cgo_compile_assert_eq(x, y, name) typedef char name[(x-y)*(x-y)*-2+1];

// Check at compile time that the sizes we use match our expectations.
#define __cgo_size_assert(t, n) __cgo_compile_assert_eq(sizeof(t), n, _cgo_sizeof_##t##_is_not_##n)

__cgo_size_assert(char, 1)
__cgo_size_assert(short, 2)
__cgo_size_assert(int, 4)
typedef long long __cgo_long_long;
__cgo_size_assert(__cgo_long_long, 8)
__cgo_size_assert(float, 4)
__cgo_size_assert(double, 8)

#include <errno.h>
#include <string.h>
`

const builtinProlog = `
typedef struct { char *p; int n; } _GoString_;
_GoString_ GoString(char *p);
_GoString_ GoStringN(char *p, int l);
char *CString(_GoString_);
`

const cProlog = `
#include "runtime.h"
#include "cgocall.h"

#pragma dynimport initcgo initcgo "%slibcgo.so"
#pragma dynimport libcgo_thread_start libcgo_thread_start "%slibcgo.so"
#pragma dynimport libcgo_set_scheduler libcgo_set_scheduler "%slibcgo.so"
#pragma dynimport _cgo_malloc _cgo_malloc "%slibcgo.so"
#pragma dynimport _cgo_free _cgo_free "%slibcgo.so"

void ·_Cerrno(void*, int32);

void
·_Cfunc_GoString(int8 *p, String s)
{
	s = gostring((byte*)p);
	FLUSH(&s);
}

void
·_Cfunc_GoStringN(int8 *p, int32 l, String s)
{
	s = gostringn((byte*)p, l);
	FLUSH(&s);
}

void
·_Cfunc_CString(String s, int8 *p)
{
	p = cmalloc(s.len+1);
	mcpy((byte*)p, s.str, s.len);
	p[s.len] = 0;
	FLUSH(&p);
}
`

const gccExportHeaderProlog = `
typedef unsigned int uint;
typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef long long int64;
typedef unsigned long long uint64;
typedef __SIZE_TYPE__ uintptr;

typedef struct { char *p; int n; } GoString;
typedef void *GoMap;
typedef void *GoChan;
typedef struct { void *t; void *v; } GoInterface;
`
