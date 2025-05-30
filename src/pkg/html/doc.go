// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
The html package implements an HTML5-compliant tokenizer and parser.

Tokenization is done by creating a Tokenizer for an io.Reader r. It is the
caller's responsibility to ensure that r provides UTF-8 encoded HTML.

	z := html.NewTokenizer(r)

Given a Tokenizer z, the HTML is tokenized by repeatedly calling z.Next(),
which parses the next token and returns its type, or an error:

	for {
		tt := z.Next()
		if tt == html.Error {
			// ...
			return ...
		}
		// Process the current token.
	}

There are two APIs for retrieving the current token. The high-level API is to
call Token; the low-level API is to call Text or TagName / TagAttr. Both APIs
allow optionally calling Raw after Next but before Token, Text, TagName, or
TagAttr. In EBNF notation, the valid call sequence per token is:

	Next {Raw} [ Token | Text | TagName {TagAttr} ]

Token returns an independent data structure that completely describes a token.
Entities (such as "&lt;") are unescaped, tag names and attribute keys are
lower-cased, and attributes are collected into a []Attribute. For example:

	for {
		if z.Next() == html.Error {
			// Returning os.EOF indicates success.
			return z.Error()
		}
		emitToken(z.Token())
	}

The low-level API performs fewer allocations and copies, but the contents of
the []byte values returned by Text, TagName and TagAttr may change on the next
call to Next. For example, to extract an HTML page's anchor text:

	depth := 0
	for {
		tt := z.Next()
		switch tt {
		case Error:
			return z.Error()
		case Text:
			if depth > 0 {
				// emitBytes should copy the []byte it receives,
				// if it doesn't process it immediately.
				emitBytes(z.Text())
			}
		case StartTag, EndTag:
			tn, _ := z.TagName()
			if len(tn) == 1 && tn[0] == 'a' {
				if tt == StartTag {
					depth++
				} else {
					depth--
				}
			}
		}
	}

The relevant specifications include:
http://www.whatwg.org/specs/web-apps/current-work/multipage/syntax.html and
http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html
*/
package html

// The tokenization algorithm implemented by this package is not a line-by-line
// transliteration of the relatively verbose state-machine in the WHATWG
// specification. A more direct approach is used instead, where the program
// counter implies the state, such as whether it is tokenizing a tag or a text
// node. Specification compliance is verified by checking expected and actual
// outputs over a test suite rather than aiming for algorithmic fidelity.

// TODO(nigeltao): Implement a parser, not just a tokenizer.
// TODO(nigeltao): Does a DOM API belong in this package or a separate one?
// TODO(nigeltao): How does parsing interact with a JavaScript engine?
