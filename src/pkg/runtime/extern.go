// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
	The runtime package contains operations that interact with Go's runtime system,
	such as functions to control goroutines. It also includes the low-level type information
	used by the reflect package; see reflect's documentation for the programmable
	interface to the run-time type system.
*/
package runtime

// Gosched yields the processor, allowing other goroutines to run.  It does not
// suspend the current goroutine, so execution resumes automatically.
func Gosched()

// Goexit terminates the goroutine that calls it.  No other goroutine is affected.
// Goexit runs all deferred calls before terminating the goroutine.
func Goexit()

// Caller reports file and line number information about function invocations on
// the calling goroutine's stack.  The argument skip is the number of stack frames to
// ascend, with 0 identifying the the caller of Caller.  The return values report the
// program counter, file name, and line number within the file of the corresponding
// call.  The boolean ok is false if it was not possible to recover the information.
func Caller(skip int) (pc uintptr, file string, line int, ok bool)

// Callers fills the slice pc with the program counters of function invocations
// on the calling goroutine's stack.  The argument skip is the number of stack frames
// to skip before recording in pc, with 0 starting at the caller of Caller.
// It returns the number of entries written to pc.
func Callers(skip int, pc []uintptr) int

// FuncForPC returns a *Func describing the function that contains the
// given program counter address, or else nil.
func FuncForPC(pc uintptr) *Func

// NOTE(rsc): Func must match struct Func in runtime.h

// Func records information about a function in the program,
// in particular  the mapping from program counters to source
// line numbers within that function.
type Func struct {
	name   string
	typ    string
	src    string
	pcln   []byte
	entry  uintptr
	pc0    uintptr
	ln0    int32
	frame  int32
	args   int32
	locals int32
}

// Name returns the name of the function.
func (f *Func) Name() string { return f.name }

// Entry returns the entry address of the function.
func (f *Func) Entry() uintptr { return f.entry }

// FileLine returns the file name and line number of the
// source code corresponding to the program counter pc.
// The result will not be accurate if pc is not a program
// counter within f.
func (f *Func) FileLine(pc uintptr) (file string, line int) {
	// NOTE(rsc): If you edit this function, also edit
	// symtab.c:/^funcline.
	var pcQuant uintptr = 1
	if GOARCH == "arm" {
		pcQuant = 4
	}

	targetpc := pc
	p := f.pcln
	pc = f.pc0
	line = int(f.ln0)
	file = f.src
	for i := 0; i < len(p) && pc <= targetpc; i++ {
		switch {
		case p[i] == 0:
			line += int(p[i+1]<<24) | int(p[i+2]<<16) | int(p[i+3]<<8) | int(p[i+4])
			i += 4
		case p[i] <= 64:
			line += int(p[i])
		case p[i] <= 128:
			line -= int(p[i] - 64)
		default:
			pc += pcQuant * uintptr(p[i]-129)
		}
		pc += pcQuant
	}
	return
}

// mid returns the current os thread (m) id.
func mid() uint32

// Semacquire waits until *s > 0 and then atomically decrements it.
// It is intended as a simple sleep primitive for use by the synchronization
// library and should not be used directly.
func Semacquire(s *uint32)

// Semrelease atomically increments *s and notifies a waiting goroutine
// if one is blocked in Semacquire.
// It is intended as a simple wakeup primitive for use by the synchronization
// library and should not be used directly.
func Semrelease(s *uint32)

// SetFinalizer sets the finalizer associated with x to f.
// When the garbage collector finds an unreachable block
// with an associated finalizer, it clears the association and runs
// f(x) in a separate goroutine.  This makes x reachable again, but
// now without an associated finalizer.  Assuming that SetFinalizer
// is not called again, the next time the garbage collector sees
// that x is unreachable, it will free x.
//
// SetFinalizer(x, nil) clears any finalizer associated with x.
//
// The argument x must be a pointer to an object allocated by
// calling new or by taking the address of a composite literal.
// The argument f must be a function that takes a single argument
// of x's type and returns no arguments.  If either of these is not
// true, SetFinalizer aborts the program.
//
// Finalizers are run in dependency order: if A points at B, both have
// finalizers, and they are otherwise unreachable, only the finalizer
// for A runs; once A is freed, the finalizer for B can run.
// If a cyclic structure includes a block with a finalizer, that
// cycle is not guaranteed to be garbage collected and the finalizer
// is not guaranteed to run, because there is no ordering that
// respects the dependencies.
//
// The finalizer for x is scheduled to run at some arbitrary time after
// x becomes unreachable.
// There is no guarantee that finalizers will run before a program exits,
// so typically they are useful only for releasing non-memory resources
// associated with an object during a long-running program.
// For example, an os.File object could use a finalizer to close the
// associated operating system file descriptor when a program discards
// an os.File without calling Close, but it would be a mistake
// to depend on a finalizer to flush an in-memory I/O buffer such as a
// bufio.Writer, because the buffer would not be flushed at program exit.
//
// A single goroutine runs all finalizers for a program, sequentially.
// If a finalizer must run for a long time, it should do so by starting
// a new goroutine.
//
// TODO(rsc): allow f to have (ignored) return values
//
func SetFinalizer(x, f interface{})

func getgoroot() string

// GOROOT returns the root of the Go tree.
// It uses the GOROOT environment variable, if set,
// or else the root used during the Go build.
func GOROOT() string {
	s := getgoroot()
	if s != "" {
		return s
	}
	return defaultGoroot
}

// Version returns the Go tree's version string.
// It is either a sequence number or, when possible,
// a release tag like "release.2010-03-04".
// A trailing + indicates that the tree had local modifications
// at the time of the build.
func Version() string {
	return theVersion
}

// GOOS is the Go tree's operating system target:
// one of darwin, freebsd, linux, and so on.
const GOOS string = theGoos

// GOARCH is the Go tree's architecture target:
// 386, amd64, or arm.
const GOARCH string = theGoarch
