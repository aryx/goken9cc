package main

import (
	"bytes"
	"flag"
	"go/parser"
	"go/printer"
	"go/ast"
	"log"
	"os"
)

var (
	srcFn   = flag.String("src", "", "source filename")
	getName = flag.String("name", "", "func/type name to output")
	html    = flag.Bool("html", true, "output HTML")
	showPkg = flag.Bool("pkg", false, "show package in output")
)

func main() {
	// handle input
	flag.Parse()
	if *srcFn == "" || *getName == "" {
		flag.Usage()
		os.Exit(2)
	}
	// load file
	file, err := parser.ParseFile(*srcFn, nil, 0)
	if err != nil {
		log.Exit(err)
	}
	// create printer
	p := &printer.Config{
		Mode:     0,
		Tabwidth: 8,
		Styler:   nil,
	}
	if *html {
		p.Mode = printer.GenHTML
	}
	// create filter
	filter := func(name string) bool {
		return name == *getName
	}
	// filter
	if !ast.FilterFile(file, filter) {
		os.Exit(1)
	}
	b := new(bytes.Buffer)
	p.Fprint(b, file)
	// drop package declaration
	if !*showPkg {
		for {
			c, err := b.ReadByte()
			if c == '\n' || err != nil {
				break
			}
		}
	}
	// drop leading newlines
	for {
		b, err := b.ReadByte()
		if err != nil {
			break
		}
		if b != '\n' {
			os.Stdout.Write([]byte{b})
			break
		}
	}
	// output
	b.WriteTo(os.Stdout)
}
