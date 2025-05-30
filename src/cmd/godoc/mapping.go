// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This file implements the Mapping data structure.

package main

import (
	"fmt"
	"io"
	"os"
	pathutil "path"
	"sort"
	"strings"
)


// A Mapping object maps relative paths (e.g. from URLs)
// to absolute paths (of the file system) and vice versa.
//
// A Mapping object consists of a list of individual mappings
// of the form: prefix -> path which are interpreted as follows:
// A relative path of the form prefix/tail is to be mapped to
// the absolute path/tail, if that absolute path exists in the file
// system. Given a Mapping object, a relative path is mapped to an
// absolute path by trying each of the individual mappings in order,
// until a valid mapping is found. For instance, for the mapping:
//
//	user   -> /home/user
//	public -> /home/user/public
//	public -> /home/build/public
//
// the relative paths below are mapped to absolute paths as follows:
//
//	user/foo                -> /home/user/foo
//	public/net/rpc/file1.go -> /home/user/public/net/rpc/file1.go
//
// If there is no /home/user/public/net/rpc/file2.go, the next public
// mapping entry is used to map the relative path to:
//
//	public/net/rpc/file2.go -> /home/build/public/net/rpc/file2.go
//
// (assuming that file exists).
//
// Each individual mapping also has a RWValue associated with it that
// may be used to store mapping-specific information. See the Iterate
// method. 
//
type Mapping struct {
	list     []mapping
	prefixes []string // lazily computed from list
}


type mapping struct {
	prefix, path string
	value        *RWValue
}


// Init initializes the Mapping from a list of ':'-separated
// paths. Empty paths are ignored; relative paths are assumed
// to be relative to the current working directory and converted
// to absolute paths. For each path of the form:
//
//	dirname/localname
//
// a mapping
//
//	localname -> path
//
// is added to the Mapping object, in the order of occurrence.
// For instance, the argument:
//
//	/home/user:/home/build/public
//
// leads to the following mapping:
//
//	user   -> /home/user
//	public -> /home/build/public
//
func (m *Mapping) Init(paths string) {
	pathlist := canonicalizePaths(strings.Split(paths, ":", -1), nil)
	list := make([]mapping, len(pathlist))

	// create mapping list
	for i, path := range pathlist {
		_, prefix := pathutil.Split(path)
		list[i] = mapping{prefix, path, new(RWValue)}
	}

	m.list = list
}


// IsEmpty returns true if there are no mappings specified.
func (m *Mapping) IsEmpty() bool { return len(m.list) == 0 }


// PrefixList returns a list of all prefixes, with duplicates removed.
// For instance, for the mapping:
//
//	user   -> /home/user
//	public -> /home/user/public
//	public -> /home/build/public
//
// the prefix list is:
//
//	user, public
//
func (m *Mapping) PrefixList() []string {
	// compute the list lazily
	if m.prefixes == nil {
		list := make([]string, len(m.list))

		// populate list
		for i, e := range m.list {
			list[i] = e.prefix
		}

		// sort the list and remove duplicate entries
		sort.SortStrings(list)
		i := 0
		prev := ""
		for _, path := range list {
			if path != prev {
				list[i] = path
				i++
				prev = path
			}
		}

		m.prefixes = list[0:i]
	}

	return m.prefixes
}


// Fprint prints the mapping.
func (m *Mapping) Fprint(w io.Writer) {
	for _, e := range m.list {
		fmt.Fprintf(w, "\t%s -> %s\n", e.prefix, e.path)
	}
}


func splitFirst(path string) (head, tail string) {
	i := strings.Index(path, "/")
	if i > 0 {
		// 0 < i < len(path)
		return path[0:i], path[i+1:]
	}
	return "", path
}


// ToAbsolute maps a relative path to an absolute path using the Mapping
// specified by the receiver. If the path cannot be mapped, the empty
// string is returned.
//
func (m *Mapping) ToAbsolute(path string) string {
	prefix, tail := splitFirst(path)
	for _, e := range m.list {
		switch {
		case e.prefix == prefix:
			// use tail
		case e.prefix == "":
			tail = path
		default:
			continue // no match
		}
		abspath := pathutil.Join(e.path, tail)
		if _, err := os.Stat(abspath); err == nil {
			return abspath
		}
	}

	return "" // no match
}


// ToRelative maps an absolute path to a relative path using the Mapping
// specified by the receiver. If the path cannot be mapped, the empty
// string is returned.
//
func (m *Mapping) ToRelative(path string) string {
	for _, e := range m.list {
		if strings.HasPrefix(path, e.path) {
			// /absolute/prefix/foo -> prefix/foo
			return pathutil.Join(e.prefix, path[len(e.path):]) // Join will remove a trailing '/'
		}
	}
	return "" // no match
}


// Iterate calls f for each path and RWValue in the mapping (in uspecified order)
// until f returns false.
//
func (m *Mapping) Iterate(f func(path string, value *RWValue) bool) {
	for _, e := range m.list {
		if !f(e.path, e.value) {
			return
		}
	}
}
