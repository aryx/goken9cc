// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package regexp

import (
	"fmt"
	"testing"
)

// For each pattern/text pair, what is the expected output of each function?
// We can derive the textual results from the indexed results, the non-submatch
// results from the submatched results, the single results from the 'all' results,
// and the byte results from the string results. Therefore the table includes
// only the FindAllStringSubmatchIndex result.
type FindTest struct {
	pat     string
	text    string
	matches [][]int
}

func (t FindTest) String() string {
	return fmt.Sprintf("pat: %#q text: %#q", t.pat, t.text)
}

var findTests = []FindTest{
	FindTest{``, ``, build(1, 0, 0)},
	FindTest{`^abcdefg`, "abcdefg", build(1, 0, 7)},
	FindTest{`a+`, "baaab", build(1, 1, 4)},
	FindTest{"abcd..", "abcdef", build(1, 0, 6)},
	FindTest{`a`, "a", build(1, 0, 1)},
	FindTest{`x`, "y", nil},
	FindTest{`b`, "abc", build(1, 1, 2)},
	FindTest{`.`, "a", build(1, 0, 1)},
	FindTest{`.*`, "abcdef", build(1, 0, 6)},
	FindTest{`^`, "abcde", build(1, 0, 0)},
	FindTest{`$`, "abcde", build(1, 5, 5)},
	FindTest{`^abcd$`, "abcd", build(1, 0, 4)},
	FindTest{`^bcd'`, "abcdef", nil},
	FindTest{`^abcd$`, "abcde", nil},
	FindTest{`a+`, "baaab", build(1, 1, 4)},
	FindTest{`a*`, "baaab", build(3, 0, 0, 1, 4, 5, 5)},
	FindTest{`[a-z]+`, "abcd", build(1, 0, 4)},
	FindTest{`[^a-z]+`, "ab1234cd", build(1, 2, 6)},
	FindTest{`[a\-\]z]+`, "az]-bcz", build(2, 0, 4, 6, 7)},
	FindTest{`[^\n]+`, "abcd\n", build(1, 0, 4)},
	FindTest{`[日本語]+`, "日本語日本語", build(1, 0, 18)},
	FindTest{`日本語+`, "日本語", build(1, 0, 9)},
	FindTest{`日本語+`, "日本語語語語", build(1, 0, 18)},
	FindTest{`()`, "", build(1, 0, 0, 0, 0)},
	FindTest{`(a)`, "a", build(1, 0, 1, 0, 1)},
	FindTest{`(.)(.)`, "日a", build(1, 0, 4, 0, 3, 3, 4)},
	FindTest{`(.*)`, "", build(1, 0, 0, 0, 0)},
	FindTest{`(.*)`, "abcd", build(1, 0, 4, 0, 4)},
	FindTest{`(..)(..)`, "abcd", build(1, 0, 4, 0, 2, 2, 4)},
	FindTest{`(([^xyz]*)(d))`, "abcd", build(1, 0, 4, 0, 4, 0, 3, 3, 4)},
	FindTest{`((a|b|c)*(d))`, "abcd", build(1, 0, 4, 0, 4, 2, 3, 3, 4)},
	FindTest{`(((a|b|c)*)(d))`, "abcd", build(1, 0, 4, 0, 4, 0, 3, 2, 3, 3, 4)},
	FindTest{`\a\b\f\n\r\t\v`, "\a\b\f\n\r\t\v", build(1, 0, 7)},
	FindTest{`[\a\b\f\n\r\t\v]+`, "\a\b\f\n\r\t\v", build(1, 0, 7)},

	FindTest{`a*(|(b))c*`, "aacc", build(1, 0, 4, 2, 2, -1, -1)},
	FindTest{`(.*).*`, "ab", build(1, 0, 2, 0, 2)},
	FindTest{`[.]`, ".", build(1, 0, 1)},
	FindTest{`/$`, "/abc/", build(1, 4, 5)},
	FindTest{`/$`, "/abc", nil},

	// multiple matches
	FindTest{`.`, "abc", build(3, 0, 1, 1, 2, 2, 3)},
	FindTest{`(.)`, "abc", build(3, 0, 1, 0, 1, 1, 2, 1, 2, 2, 3, 2, 3)},
	FindTest{`.(.)`, "abcd", build(2, 0, 2, 1, 2, 2, 4, 3, 4)},
	FindTest{`ab*`, "abbaab", build(3, 0, 3, 3, 4, 4, 6)},
	FindTest{`a(b*)`, "abbaab", build(3, 0, 3, 1, 3, 3, 4, 4, 4, 4, 6, 5, 6)},

	// fixed bugs
	FindTest{`ab$`, "cab", build(1, 1, 3)},
	FindTest{`axxb$`, "axxcb", nil},
	FindTest{`data`, "daXY data", build(1, 5, 9)},
	FindTest{`da(.)a$`, "daXY data", build(1, 5, 9, 7, 8)},

	// can backslash-escape any punctuation
	FindTest{`\!\"\#\$\%\&\'\(\)\*\+\,\-\.\/\:\;\<\=\>\?\@\[\\\]\^\_\{\|\}\~`,
		`!"#$%&'()*+,-./:;<=>?@[\]^_{|}~`, build(1, 0, 31)},
	FindTest{`[\!\"\#\$\%\&\'\(\)\*\+\,\-\.\/\:\;\<\=\>\?\@\[\\\]\^\_\{\|\}\~]+`,
		`!"#$%&'()*+,-./:;<=>?@[\]^_{|}~`, build(1, 0, 31)},
	FindTest{"\\`", "`", build(1, 0, 1)},
	FindTest{"[\\`]+", "`", build(1, 0, 1)},

	// long set of matches (longer than startSize)
	FindTest{
		".",
		"qwertyuiopasdfghjklzxcvbnm1234567890",
		build(36, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
			10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
			20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30,
			30, 31, 31, 32, 32, 33, 33, 34, 34, 35, 35, 36),
	},
}

// build is a helper to construct a [][]int by extracting n sequences from x.
// This represents n matches with len(x)/n submatches each.
func build(n int, x ...int) [][]int {
	ret := make([][]int, n)
	runLength := len(x) / n
	j := 0
	for i := range ret {
		ret[i] = make([]int, runLength)
		copy(ret[i], x[j:])
		j += runLength
		if j > len(x) {
			panic("invalid build entry")
		}
	}
	return ret
}

// First the simple cases.

func TestFind(t *testing.T) {
	for _, test := range findTests {
		result := MustCompile(test.pat).Find([]byte(test.text))
		switch {
		case len(test.matches) == 0 && len(result) == 0:
			// ok
		case test.matches == nil && result != nil:
			t.Errorf("expected no match; got one: %s", test)
		case test.matches != nil && result == nil:
			t.Errorf("expected match; got none: %s", test)
		case test.matches != nil && result != nil:
			expect := test.text[test.matches[0][0]:test.matches[0][1]]
			if expect != string(result) {
				t.Errorf("expected %q got %q: %s", expect, result, test)
			}
		}
	}
}

func TestFindString(t *testing.T) {
	for _, test := range findTests {
		result := MustCompile(test.pat).FindString(test.text)
		switch {
		case len(test.matches) == 0 && len(result) == 0:
			// ok
		case test.matches == nil && result != "":
			t.Errorf("expected no match; got one: %s", test)
		case test.matches != nil && result == "":
			// Tricky because an empty result has two meanings: no match or empty match.
			if test.matches[0][0] != test.matches[0][1] {
				t.Errorf("expected match; got none: %s", test)
			}
		case test.matches != nil && result != "":
			expect := test.text[test.matches[0][0]:test.matches[0][1]]
			if expect != result {
				t.Errorf("expected %q got %q: %s", expect, result, test)
			}
		}
	}
}

func testFindIndex(test *FindTest, result []int, t *testing.T) {
	switch {
	case len(test.matches) == 0 && len(result) == 0:
		// ok
	case test.matches == nil && result != nil:
		t.Errorf("expected no match; got one: %s", test)
	case test.matches != nil && result == nil:
		t.Errorf("expected match; got none: %s", test)
	case test.matches != nil && result != nil:
		expect := test.matches[0]
		if expect[0] != result[0] || expect[1] != result[1] {
			t.Errorf("expected %v got %v: %s", expect, result, test)
		}
	}
}

func TestFindIndex(t *testing.T) {
	for _, test := range findTests {
		testFindIndex(&test, MustCompile(test.pat).FindIndex([]byte(test.text)), t)
	}
}

func TestFindStringIndex(t *testing.T) {
	for _, test := range findTests {
		testFindIndex(&test, MustCompile(test.pat).FindStringIndex(test.text), t)
	}
}

// Now come the simple All cases.

func TestFindAll(t *testing.T) {
	for _, test := range findTests {
		result := MustCompile(test.pat).FindAll([]byte(test.text), -1)
		switch {
		case test.matches == nil && result == nil:
			// ok
		case test.matches == nil && result != nil:
			t.Errorf("expected no match; got one: %s", test)
		case test.matches != nil && result == nil:
			t.Errorf("expected match; got none: %s", test)
		case test.matches != nil && result != nil:
			if len(test.matches) != len(result) {
				t.Errorf("expected %d matches; got %d: %s", len(test.matches), len(result), test)
				continue
			}
			for k, e := range test.matches {
				expect := test.text[e[0]:e[1]]
				if expect != string(result[k]) {
					t.Errorf("match %d: expected %q got %q: %s", k, expect, result[k], test)
				}
			}
		}
	}
}

func TestFindAllString(t *testing.T) {
	for _, test := range findTests {
		result := MustCompile(test.pat).FindAllString(test.text, -1)
		switch {
		case test.matches == nil && result == nil:
			// ok
		case test.matches == nil && result != nil:
			t.Errorf("expected no match; got one: %s", test)
		case test.matches != nil && result == nil:
			t.Errorf("expected match; got none: %s", test)
		case test.matches != nil && result != nil:
			if len(test.matches) != len(result) {
				t.Errorf("expected %d matches; got %d: %s", len(test.matches), len(result), test)
				continue
			}
			for k, e := range test.matches {
				expect := test.text[e[0]:e[1]]
				if expect != result[k] {
					t.Errorf("expected %q got %q: %s", expect, result, test)
				}
			}
		}
	}
}

func testFindAllIndex(test *FindTest, result [][]int, t *testing.T) {
	switch {
	case test.matches == nil && result == nil:
		// ok
	case test.matches == nil && result != nil:
		t.Errorf("expected no match; got one: %s", test)
	case test.matches != nil && result == nil:
		t.Errorf("expected match; got none: %s", test)
	case test.matches != nil && result != nil:
		if len(test.matches) != len(result) {
			t.Errorf("expected %d matches; got %d: %s", len(test.matches), len(result), test)
			return
		}
		for k, e := range test.matches {
			if e[0] != result[k][0] || e[1] != result[k][1] {
				t.Errorf("match %d: expected %v got %v: %s", k, e, result[k], test)
			}
		}
	}
}

func TestFindAllIndex(t *testing.T) {
	for _, test := range findTests {
		testFindAllIndex(&test, MustCompile(test.pat).FindAllIndex([]byte(test.text), -1), t)
	}
}

func TestFindAllStringIndex(t *testing.T) {
	for _, test := range findTests {
		testFindAllIndex(&test, MustCompile(test.pat).FindAllStringIndex(test.text, -1), t)
	}
}

// Now come the Submatch cases.

func testSubmatchBytes(test *FindTest, n int, submatches []int, result [][]byte, t *testing.T) {
	if len(submatches) != len(result)*2 {
		t.Errorf("match %d: expected %d submatches; got %d: %s", n, len(submatches)/2, len(result), test)
		return
	}
	for k := 0; k < len(submatches); k += 2 {
		if submatches[k] == -1 {
			if result[k/2] != nil {
				t.Errorf("match %d: expected nil got %q: %s", n, result, test)
			}
			continue
		}
		expect := test.text[submatches[k]:submatches[k+1]]
		if expect != string(result[k/2]) {
			t.Errorf("match %d: expected %q got %q: %s", n, expect, result, test)
			return
		}
	}
}

func TestFindSubmatch(t *testing.T) {
	for _, test := range findTests {
		result := MustCompile(test.pat).FindSubmatch([]byte(test.text))
		switch {
		case test.matches == nil && result == nil:
			// ok
		case test.matches == nil && result != nil:
			t.Errorf("expected no match; got one: %s", test)
		case test.matches != nil && result == nil:
			t.Errorf("expected match; got none: %s", test)
		case test.matches != nil && result != nil:
			testSubmatchBytes(&test, 0, test.matches[0], result, t)
		}
	}
}

func testSubmatchString(test *FindTest, n int, submatches []int, result []string, t *testing.T) {
	if len(submatches) != len(result)*2 {
		t.Errorf("match %d: expected %d submatches; got %d: %s", n, len(submatches)/2, len(result), test)
		return
	}
	for k := 0; k < len(submatches); k += 2 {
		if submatches[k] == -1 {
			if result[k/2] != "" {
				t.Errorf("match %d: expected nil got %q: %s", n, result, test)
			}
			continue
		}
		expect := test.text[submatches[k]:submatches[k+1]]
		if expect != result[k/2] {
			t.Errorf("match %d: expected %q got %q: %s", n, expect, result, test)
			return
		}
	}
}

func TestFindStringSubmatch(t *testing.T) {
	for _, test := range findTests {
		result := MustCompile(test.pat).FindStringSubmatch(test.text)
		switch {
		case test.matches == nil && result == nil:
			// ok
		case test.matches == nil && result != nil:
			t.Errorf("expected no match; got one: %s", test)
		case test.matches != nil && result == nil:
			t.Errorf("expected match; got none: %s", test)
		case test.matches != nil && result != nil:
			testSubmatchString(&test, 0, test.matches[0], result, t)
		}
	}
}

func testSubmatchIndices(test *FindTest, n int, expect, result []int, t *testing.T) {
	if len(expect) != len(result) {
		t.Errorf("match %d: expected %d matches; got %d: %s", n, len(expect)/2, len(result)/2, test)
		return
	}
	for k, e := range expect {
		if e != result[k] {
			t.Errorf("match %d: submatch error: expected %v got %v: %s", n, expect, result, test)
		}
	}
}

func testFindSubmatchIndex(test *FindTest, result []int, t *testing.T) {
	switch {
	case test.matches == nil && result == nil:
		// ok
	case test.matches == nil && result != nil:
		t.Errorf("expected no match; got one: %s", test)
	case test.matches != nil && result == nil:
		t.Errorf("expected match; got none: %s", test)
	case test.matches != nil && result != nil:
		testSubmatchIndices(test, 0, test.matches[0], result, t)
	}
}

func TestFindSubmatchIndex(t *testing.T) {
	for _, test := range findTests {
		testFindSubmatchIndex(&test, MustCompile(test.pat).FindSubmatchIndex([]byte(test.text)), t)
	}
}

func TestFindStringSubmatchndex(t *testing.T) {
	for _, test := range findTests {
		testFindSubmatchIndex(&test, MustCompile(test.pat).FindStringSubmatchIndex(test.text), t)
	}
}

// Now come the monster AllSubmatch cases.

func TestFindAllSubmatch(t *testing.T) {
	for _, test := range findTests {
		result := MustCompile(test.pat).FindAllSubmatch([]byte(test.text), -1)
		switch {
		case test.matches == nil && result == nil:
			// ok
		case test.matches == nil && result != nil:
			t.Errorf("expected no match; got one: %s", test)
		case test.matches != nil && result == nil:
			t.Errorf("expected match; got none: %s", test)
		case len(test.matches) != len(result):
			t.Errorf("expected %d matches; got %d: %s", len(test.matches), len(result), test)
		case test.matches != nil && result != nil:
			for k, match := range test.matches {
				testSubmatchBytes(&test, k, match, result[k], t)
			}
		}
	}
}

func TestFindAllStringSubmatch(t *testing.T) {
	for _, test := range findTests {
		result := MustCompile(test.pat).FindAllStringSubmatch(test.text, -1)
		switch {
		case test.matches == nil && result == nil:
			// ok
		case test.matches == nil && result != nil:
			t.Errorf("expected no match; got one: %s", test)
		case test.matches != nil && result == nil:
			t.Errorf("expected match; got none: %s", test)
		case len(test.matches) != len(result):
			t.Errorf("expected %d matches; got %d: %s", len(test.matches), len(result), test)
		case test.matches != nil && result != nil:
			for k, match := range test.matches {
				testSubmatchString(&test, k, match, result[k], t)
			}
		}
	}
}

func testFindAllSubmatchIndex(test *FindTest, result [][]int, t *testing.T) {
	switch {
	case test.matches == nil && result == nil:
		// ok
	case test.matches == nil && result != nil:
		t.Errorf("expected no match; got one: %s", test)
	case test.matches != nil && result == nil:
		t.Errorf("expected match; got none: %s", test)
	case len(test.matches) != len(result):
		t.Errorf("expected %d matches; got %d: %s", len(test.matches), len(result), test)
	case test.matches != nil && result != nil:
		for k, match := range test.matches {
			testSubmatchIndices(test, k, match, result[k], t)
		}
	}
}

func TestFindAllSubmatchIndex(t *testing.T) {
	for _, test := range findTests {
		testFindAllSubmatchIndex(&test, MustCompile(test.pat).FindAllSubmatchIndex([]byte(test.text), -1), t)
	}
}

func TestFindAllStringSubmatchndex(t *testing.T) {
	for _, test := range findTests {
		testFindAllSubmatchIndex(&test, MustCompile(test.pat).FindAllStringSubmatchIndex(test.text, -1), t)
	}
}
