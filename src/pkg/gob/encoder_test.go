// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package gob

import (
	"bytes"
	"io"
	"os"
	"reflect"
	"strings"
	"testing"
)

type ET2 struct {
	x string
}

type ET1 struct {
	a    int
	et2  *ET2
	next *ET1
}

// Like ET1 but with a different name for a field
type ET3 struct {
	a             int
	et2           *ET2
	differentNext *ET1
}

// Like ET1 but with a different type for a field
type ET4 struct {
	a    int
	et2  float
	next int
}

func TestEncoderDecoder(t *testing.T) {
	b := new(bytes.Buffer)
	enc := NewEncoder(b)
	et1 := new(ET1)
	et1.a = 7
	et1.et2 = new(ET2)
	enc.Encode(et1)
	if enc.state.err != nil {
		t.Error("encoder fail:", enc.state.err)
	}
	dec := NewDecoder(b)
	newEt1 := new(ET1)
	dec.Decode(newEt1)
	if dec.state.err != nil {
		t.Fatal("error decoding ET1:", dec.state.err)
	}

	if !reflect.DeepEqual(et1, newEt1) {
		t.Fatalf("invalid data for et1: expected %+v; got %+v", *et1, *newEt1)
	}
	if b.Len() != 0 {
		t.Error("not at eof;", b.Len(), "bytes left")
	}

	enc.Encode(et1)
	newEt1 = new(ET1)
	dec.Decode(newEt1)
	if dec.state.err != nil {
		t.Fatal("round 2: error decoding ET1:", dec.state.err)
	}
	if !reflect.DeepEqual(et1, newEt1) {
		t.Fatalf("round 2: invalid data for et1: expected %+v; got %+v", *et1, *newEt1)
	}
	if b.Len() != 0 {
		t.Error("round 2: not at eof;", b.Len(), "bytes left")
	}

	// Now test with a running encoder/decoder pair that we recognize a type mismatch.
	enc.Encode(et1)
	if enc.state.err != nil {
		t.Error("round 3: encoder fail:", enc.state.err)
	}
	newEt2 := new(ET2)
	dec.Decode(newEt2)
	if dec.state.err == nil {
		t.Fatal("round 3: expected `bad type' error decoding ET2")
	}
}

// Run one value through the encoder/decoder, but use the wrong type.
// Input is always an ET1; we compare it to whatever is under 'e'.
func badTypeCheck(e interface{}, shouldFail bool, msg string, t *testing.T) {
	b := new(bytes.Buffer)
	enc := NewEncoder(b)
	et1 := new(ET1)
	et1.a = 7
	et1.et2 = new(ET2)
	enc.Encode(et1)
	if enc.state.err != nil {
		t.Error("encoder fail:", enc.state.err)
	}
	dec := NewDecoder(b)
	dec.Decode(e)
	if shouldFail && (dec.state.err == nil) {
		t.Error("expected error for", msg)
	}
	if !shouldFail && (dec.state.err != nil) {
		t.Error("unexpected error for", msg, dec.state.err)
	}
}

// Test that we recognize a bad type the first time.
func TestWrongTypeDecoder(t *testing.T) {
	badTypeCheck(new(ET2), true, "no fields in common", t)
	badTypeCheck(new(ET3), false, "different name of field", t)
	badTypeCheck(new(ET4), true, "different type of field", t)
}

func corruptDataCheck(s string, err os.Error, t *testing.T) {
	b := bytes.NewBufferString(s)
	dec := NewDecoder(b)
	dec.Decode(new(ET2))
	if dec.state.err != err {
		t.Error("expected error", err, "got", dec.state.err)
	}
}

// Check that we survive bad data.
func TestBadData(t *testing.T) {
	corruptDataCheck("", os.EOF, t)
	corruptDataCheck("\x7Fhi", io.ErrUnexpectedEOF, t)
	corruptDataCheck("\x03now is the time for all good men", errBadType, t)
}

// Types not supported by the Encoder.
var unsupportedValues = []interface{}{
	make(chan int),
	func(a int) bool { return true },
	new(interface{}),
}

func TestUnsupported(t *testing.T) {
	var b bytes.Buffer
	enc := NewEncoder(&b)
	for _, v := range unsupportedValues {
		err := enc.Encode(v)
		if err == nil {
			t.Errorf("expected error for %T; got none", v)
		}
	}
}

func encAndDec(in, out interface{}) os.Error {
	b := new(bytes.Buffer)
	enc := NewEncoder(b)
	enc.Encode(in)
	if enc.state.err != nil {
		return enc.state.err
	}
	dec := NewDecoder(b)
	dec.Decode(out)
	if dec.state.err != nil {
		return dec.state.err
	}
	return nil
}

func TestTypeToPtrType(t *testing.T) {
	// Encode a T, decode a *T
	type Type0 struct {
		a int
	}
	t0 := Type0{7}
	t0p := (*Type0)(nil)
	if err := encAndDec(t0, t0p); err != nil {
		t.Error(err)
	}
}

func TestPtrTypeToType(t *testing.T) {
	// Encode a *T, decode a T
	type Type1 struct {
		a uint
	}
	t1p := &Type1{17}
	var t1 Type1
	if err := encAndDec(t1, t1p); err != nil {
		t.Error(err)
	}
}

func TestTypeToPtrPtrPtrPtrType(t *testing.T) {
	type Type2 struct {
		a ****float
	}
	t2 := Type2{}
	t2.a = new(***float)
	*t2.a = new(**float)
	**t2.a = new(*float)
	***t2.a = new(float)
	****t2.a = 27.4
	t2pppp := new(***Type2)
	if err := encAndDec(t2, t2pppp); err != nil {
		t.Error(err)
	}
	if ****(****t2pppp).a != ****t2.a {
		t.Errorf("wrong value after decode: %g not %g", ****(****t2pppp).a, ****t2.a)
	}
}

func TestSlice(t *testing.T) {
	type Type3 struct {
		a []string
	}
	t3p := &Type3{[]string{"hello", "world"}}
	var t3 Type3
	if err := encAndDec(t3, t3p); err != nil {
		t.Error(err)
	}
}

func TestValueError(t *testing.T) {
	// Encode a *T, decode a T
	type Type4 struct {
		a int
	}
	t4p := &Type4{3}
	var t4 Type4 // note: not a pointer.
	if err := encAndDec(t4p, t4); err == nil || strings.Index(err.String(), "pointer") < 0 {
		t.Error("expected error about pointer; got", err)
	}
}

func TestArray(t *testing.T) {
	type Type5 struct {
		a [3]string
		b [3]byte
	}
	type Type6 struct {
		a [2]string // can't hold t5.a
	}
	t5 := Type5{[3]string{"hello", ",", "world"}, [3]byte{1, 2, 3}}
	var t5p Type5
	if err := encAndDec(t5, &t5p); err != nil {
		t.Error(err)
	}
	var t6 Type6
	if err := encAndDec(t5, &t6); err == nil {
		t.Error("should fail with mismatched array sizes")
	}
}

// Regression test for bug: must send zero values inside arrays
func TestDefaultsInArray(t *testing.T) {
	type Type7 struct {
		b []bool
		i []int
		s []string
		f []float
	}
	t7 := Type7{
		[]bool{false, false, true},
		[]int{0, 0, 1},
		[]string{"hi", "", "there"},
		[]float{0, 0, 1},
	}
	var t7p Type7
	if err := encAndDec(t7, &t7p); err != nil {
		t.Error(err)
	}
}

var testInt int
var testFloat32 float32
var testString string
var testSlice []string
var testMap map[string]int
var testArray [7]int

type SingleTest struct {
	in  interface{}
	out interface{}
	err string
}

var singleTests = []SingleTest{
	SingleTest{17, &testInt, ""},
	SingleTest{float32(17.5), &testFloat32, ""},
	SingleTest{"bike shed", &testString, ""},
	SingleTest{[]string{"bike", "shed", "paint", "color"}, &testSlice, ""},
	SingleTest{map[string]int{"seven": 7, "twelve": 12}, &testMap, ""},
	SingleTest{[7]int{4, 55, 0, 0, 0, 0, 0}, &testArray, ""}, // case that once triggered a bug
	SingleTest{[7]int{4, 55, 1, 44, 22, 66, 1234}, &testArray, ""},

	// Decode errors
	SingleTest{172, &testFloat32, "wrong type"},
}

func TestSingletons(t *testing.T) {
	b := new(bytes.Buffer)
	enc := NewEncoder(b)
	dec := NewDecoder(b)
	for _, test := range singleTests {
		b.Reset()
		err := enc.Encode(test.in)
		if err != nil {
			t.Errorf("error encoding %v: %s", test.in, err)
			continue
		}
		err = dec.Decode(test.out)
		switch {
		case err != nil && test.err == "":
			t.Errorf("error decoding %v: %s", test.in, err)
			continue
		case err == nil && test.err != "":
			t.Errorf("expected error decoding %v: %s", test.in, test.err)
			continue
		case err != nil && test.err != "":
			if strings.Index(err.String(), test.err) < 0 {
				t.Errorf("wrong error decoding %v: wanted %s, got %v", test.in, test.err, err)
			}
			continue
		}
		// Get rid of the pointer in the rhs
		val := reflect.NewValue(test.out).(*reflect.PtrValue).Elem().Interface()
		if !reflect.DeepEqual(test.in, val) {
			t.Errorf("decoding singleton: expected %v got %v", test.in, val)
		}
	}
}

func TestStructNonStruct(t *testing.T) {
	type Struct struct {
		a string
	}
	type NonStruct string
	s := Struct{"hello"}
	var sp Struct
	if err := encAndDec(s, &sp); err != nil {
		t.Error(err)
	}
	var ns NonStruct
	if err := encAndDec(s, &ns); err == nil {
		t.Error("should get error for struct/non-struct")
	} else if strings.Index(err.String(), "type") < 0 {
		t.Error("for struct/non-struct expected type error; got", err)
	}
	// Now try the other way
	var nsp NonStruct
	if err := encAndDec(ns, &nsp); err != nil {
		t.Error(err)
	}
	if err := encAndDec(ns, &s); err == nil {
		t.Error("should get error for non-struct/struct")
	} else if strings.Index(err.String(), "type") < 0 {
		t.Error("for non-struct/struct expected type error; got", err)
	}
}
