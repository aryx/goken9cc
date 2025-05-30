// Copyright 2009 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Parse input AST and prepare Prog structure.

package main

import (
	"fmt"
	"go/ast"
	"go/doc"
	"go/parser"
	"go/scanner"
	"os"
	"strings"
)

func parse(name string, flags uint) *ast.File {
	ast1, err := parser.ParseFile(name, nil, flags)
	if err != nil {
		if list, ok := err.(scanner.ErrorList); ok {
			// If err is a scanner.ErrorList, its String will print just
			// the first error and then (+n more errors).
			// Instead, turn it into a new Error that will return
			// details for all the errors.
			for _, e := range list {
				fmt.Fprintln(os.Stderr, e)
			}
			os.Exit(2)
		}
		fatal("parsing %s: %s", name, err)
	}
	return ast1
}

// ReadGo populates f with information learned from reading the
// Go source file with the given file name.  It gathers the C preamble
// attached to the import "C" comment, a list of references to C.xxx,
// a list of exported functions, and the actual AST, to be rewritten and
// printed.
func (f *File) ReadGo(name string) {
	// Two different parses: once with comments, once without.
	// The printer is not good enough at printing comments in the
	// right place when we start editing the AST behind its back,
	// so we use ast1 to look for the doc comments on import "C"
	// and on exported functions, and we use ast2 for translating
	// and reprinting.
	ast1 := parse(name, parser.ParseComments)
	ast2 := parse(name, 0)

	f.Package = ast1.Name.Name
	f.Name = make(map[string]*Name)

	// In ast1, find the import "C" line and get any extra C preamble.
	sawC := false
	for _, decl := range ast1.Decls {
		d, ok := decl.(*ast.GenDecl)
		if !ok {
			continue
		}
		for _, spec := range d.Specs {
			s, ok := spec.(*ast.ImportSpec)
			if !ok || string(s.Path.Value) != `"C"` {
				continue
			}
			sawC = true
			if s.Name != nil {
				error(s.Path.Pos(), `cannot rename import "C"`)
			}
			if s.Doc != nil {
				f.Preamble += doc.CommentText(s.Doc) + "\n"
			} else if len(d.Specs) == 1 && d.Doc != nil {
				f.Preamble += doc.CommentText(d.Doc) + "\n"
			}
		}
	}
	if !sawC {
		error(noPos, `cannot find import "C"`)
	}

	// In ast2, strip the import "C" line.
	w := 0
	for _, decl := range ast2.Decls {
		d, ok := decl.(*ast.GenDecl)
		if !ok {
			ast2.Decls[w] = decl
			w++
			continue
		}
		ws := 0
		for _, spec := range d.Specs {
			s, ok := spec.(*ast.ImportSpec)
			if !ok || string(s.Path.Value) != `"C"` {
				d.Specs[ws] = spec
				ws++
			}
		}
		if ws == 0 {
			continue
		}
		d.Specs = d.Specs[0:ws]
		ast2.Decls[w] = d
		w++
	}
	ast2.Decls = ast2.Decls[0:w]

	// Accumulate pointers to uses of C.x.
	if f.Ref == nil {
		f.Ref = make([]*Ref, 0, 8)
	}
	f.walk(ast2, "prog", (*File).saveRef)

	// Accumulate exported functions.
	// The comments are only on ast1 but we need to
	// save the function bodies from ast2.
	// The first walk fills in ExpFunc, and the
	// second walk changes the entries to
	// refer to ast2 instead.
	f.walk(ast1, "prog", (*File).saveExport)
	f.walk(ast2, "prog", (*File).saveExport2)

	f.AST = ast2
}

// Save references to C.xxx for later processing.
func (f *File) saveRef(x interface{}, context string) {
	n, ok := x.(*ast.Expr)
	if !ok {
		return
	}
	if sel, ok := (*n).(*ast.SelectorExpr); ok {
		// For now, assume that the only instance of capital C is
		// when used as the imported package identifier.
		// The parser should take care of scoping in the future,
		// so that we will be able to distinguish a "top-level C"
		// from a local C.
		if l, ok := sel.X.(*ast.Ident); ok && l.Name == "C" {
			i := len(f.Ref)
			if i >= cap(f.Ref) {
				new := make([]*Ref, 2*i)
				for j, v := range f.Ref {
					new[j] = v
				}
				f.Ref = new
			}
			if context == "as2" {
				context = "expr"
			}
			goname := sel.Sel.Name
			name := f.Name[goname]
			if name == nil {
				name = &Name{
					Go: goname,
				}
				f.Name[goname] = name
			}
			f.Ref = f.Ref[0 : i+1]
			f.Ref[i] = &Ref{
				Name:    name,
				Expr:    n,
				Context: context,
			}
			return
		}
	}
}

// If a function should be exported add it to ExpFunc.
func (f *File) saveExport(x interface{}, context string) {
	n, ok := x.(*ast.FuncDecl)
	if !ok {
		return
	}

	if n.Doc == nil {
		return
	}
	for _, c := range n.Doc.List {
		if string(c.Text[0:9]) != "//export " {
			continue
		}

		name := strings.TrimSpace(string(c.Text[9:]))
		if name == "" {
			error(c.Position, "export missing name")
		}

		if f.ExpFunc == nil {
			f.ExpFunc = make([]*ExpFunc, 0, 8)
		}
		i := len(f.ExpFunc)
		if i >= cap(f.ExpFunc) {
			new := make([]*ExpFunc, i, 2*i)
			copy(new, f.ExpFunc)
			f.ExpFunc = new
		}
		f.ExpFunc = f.ExpFunc[0 : i+1]
		f.ExpFunc[i] = &ExpFunc{
			Func:    n,
			ExpName: name,
		}
		break
	}
}

// Make f.ExpFunc[i] point at the Func from this AST instead of the other one.
func (f *File) saveExport2(x interface{}, context string) {
	n, ok := x.(*ast.FuncDecl)
	if !ok {
		return
	}

	for _, exp := range f.ExpFunc {
		if exp.Func.Name.Name == n.Name.Name {
			exp.Func = n
			break
		}
	}
}

// walk walks the AST x, calling visit(f, x, context) for each node.
func (f *File) walk(x interface{}, context string, visit func(*File, interface{}, string)) {
	visit(f, x, context)
	switch n := x.(type) {
	case *ast.Expr:
		f.walk(*n, context, visit)

	// everything else just recurs
	default:
		error(noPos, "unexpected type %T in walk", x, visit)
		panic("unexpected type")

	case nil:

	// These are ordered and grouped to match ../../pkg/go/ast/ast.go
	case *ast.Field:
		f.walk(&n.Type, "type", visit)
	case *ast.FieldList:
		for _, field := range n.List {
			f.walk(field, context, visit)
		}
	case *ast.BadExpr:
	case *ast.Ident:
	case *ast.Ellipsis:
	case *ast.BasicLit:
	case *ast.FuncLit:
		f.walk(n.Type, "type", visit)
		f.walk(n.Body, "stmt", visit)
	case *ast.CompositeLit:
		f.walk(&n.Type, "type", visit)
		f.walk(n.Elts, "expr", visit)
	case *ast.ParenExpr:
		f.walk(&n.X, context, visit)
	case *ast.SelectorExpr:
		f.walk(&n.X, "selector", visit)
	case *ast.IndexExpr:
		f.walk(&n.X, "expr", visit)
		f.walk(&n.Index, "expr", visit)
	case *ast.SliceExpr:
		f.walk(&n.X, "expr", visit)
		f.walk(&n.Index, "expr", visit)
		if n.End != nil {
			f.walk(&n.End, "expr", visit)
		}
	case *ast.TypeAssertExpr:
		f.walk(&n.X, "expr", visit)
		f.walk(&n.Type, "type", visit)
	case *ast.CallExpr:
		if context == "as2" {
			f.walk(&n.Fun, "call2", visit)
		} else {
			f.walk(&n.Fun, "call", visit)
		}
		f.walk(n.Args, "expr", visit)
	case *ast.StarExpr:
		f.walk(&n.X, context, visit)
	case *ast.UnaryExpr:
		f.walk(&n.X, "expr", visit)
	case *ast.BinaryExpr:
		f.walk(&n.X, "expr", visit)
		f.walk(&n.Y, "expr", visit)
	case *ast.KeyValueExpr:
		f.walk(&n.Key, "expr", visit)
		f.walk(&n.Value, "expr", visit)

	case *ast.ArrayType:
		f.walk(&n.Len, "expr", visit)
		f.walk(&n.Elt, "type", visit)
	case *ast.StructType:
		f.walk(n.Fields, "field", visit)
	case *ast.FuncType:
		f.walk(n.Params, "field", visit)
		if n.Results != nil {
			f.walk(n.Results, "field", visit)
		}
	case *ast.InterfaceType:
		f.walk(n.Methods, "field", visit)
	case *ast.MapType:
		f.walk(&n.Key, "type", visit)
		f.walk(&n.Value, "type", visit)
	case *ast.ChanType:
		f.walk(&n.Value, "type", visit)

	case *ast.BadStmt:
	case *ast.DeclStmt:
		f.walk(n.Decl, "decl", visit)
	case *ast.EmptyStmt:
	case *ast.LabeledStmt:
		f.walk(n.Stmt, "stmt", visit)
	case *ast.ExprStmt:
		f.walk(&n.X, "expr", visit)
	case *ast.IncDecStmt:
		f.walk(&n.X, "expr", visit)
	case *ast.AssignStmt:
		f.walk(n.Lhs, "expr", visit)
		if len(n.Lhs) == 2 && len(n.Rhs) == 1 {
			f.walk(n.Rhs, "as2", visit)
		} else {
			f.walk(n.Rhs, "expr", visit)
		}
	case *ast.GoStmt:
		f.walk(n.Call, "expr", visit)
	case *ast.DeferStmt:
		f.walk(n.Call, "expr", visit)
	case *ast.ReturnStmt:
		f.walk(n.Results, "expr", visit)
	case *ast.BranchStmt:
	case *ast.BlockStmt:
		f.walk(n.List, "stmt", visit)
	case *ast.IfStmt:
		f.walk(n.Init, "stmt", visit)
		f.walk(&n.Cond, "expr", visit)
		f.walk(n.Body, "stmt", visit)
		f.walk(n.Else, "stmt", visit)
	case *ast.CaseClause:
		f.walk(n.Values, "expr", visit)
		f.walk(n.Body, "stmt", visit)
	case *ast.SwitchStmt:
		f.walk(n.Init, "stmt", visit)
		f.walk(&n.Tag, "expr", visit)
		f.walk(n.Body, "stmt", visit)
	case *ast.TypeCaseClause:
		f.walk(n.Types, "type", visit)
		f.walk(n.Body, "stmt", visit)
	case *ast.TypeSwitchStmt:
		f.walk(n.Init, "stmt", visit)
		f.walk(n.Assign, "stmt", visit)
		f.walk(n.Body, "stmt", visit)
	case *ast.CommClause:
		f.walk(n.Lhs, "expr", visit)
		f.walk(n.Rhs, "expr", visit)
		f.walk(n.Body, "stmt", visit)
	case *ast.SelectStmt:
		f.walk(n.Body, "stmt", visit)
	case *ast.ForStmt:
		f.walk(n.Init, "stmt", visit)
		f.walk(&n.Cond, "expr", visit)
		f.walk(n.Post, "stmt", visit)
		f.walk(n.Body, "stmt", visit)
	case *ast.RangeStmt:
		f.walk(&n.Key, "expr", visit)
		f.walk(&n.Value, "expr", visit)
		f.walk(&n.X, "expr", visit)
		f.walk(n.Body, "stmt", visit)

	case *ast.ImportSpec:
	case *ast.ValueSpec:
		f.walk(&n.Type, "type", visit)
		f.walk(n.Values, "expr", visit)
	case *ast.TypeSpec:
		f.walk(&n.Type, "type", visit)

	case *ast.BadDecl:
	case *ast.GenDecl:
		f.walk(n.Specs, "spec", visit)
	case *ast.FuncDecl:
		if n.Recv != nil {
			f.walk(n.Recv, "field", visit)
		}
		f.walk(n.Type, "type", visit)
		if n.Body != nil {
			f.walk(n.Body, "stmt", visit)
		}

	case *ast.File:
		f.walk(n.Decls, "decl", visit)

	case *ast.Package:
		for _, file := range n.Files {
			f.walk(file, "file", visit)
		}

	case []ast.Decl:
		for _, d := range n {
			f.walk(d, context, visit)
		}
	case []ast.Expr:
		for i := range n {
			f.walk(&n[i], context, visit)
		}
	case []ast.Stmt:
		for _, s := range n {
			f.walk(s, context, visit)
		}
	case []ast.Spec:
		for _, s := range n {
			f.walk(s, context, visit)
		}
	}
}
