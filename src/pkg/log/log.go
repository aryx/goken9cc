// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Simple logging package. It defines a type, Logger, with methods
// for formatting output. It also has a predefined 'standard' Logger
// accessible through helper functions Print[f|ln], Exit[f|ln], and
// Panic[f|ln], which are easier to use than creating a Logger manually.
// That logger writes to standard error and prints the date and time
// of each logged message.
// The Exit functions call os.Exit(1) after writing the log message.
// The Panic functions call panic after writing the log message.
package log

import (
	"bytes"
	"fmt"
	"io"
	"runtime"
	"os"
	"time"
)

// These flags define which text to prefix to each log entry generated by the Logger.
const (
	// Bits or'ed together to control what's printed. There is no control over the
	// order they appear (the order listed here) or the format they present (as
	// described in the comments).  A colon appears after these items:
	//	2009/0123 01:23:23.123123 /a/b/c/d.go:23: message
	Ldate         = 1 << iota // the date: 2009/0123
	Ltime                     // the time: 01:23:23
	Lmicroseconds             // microsecond resolution: 01:23:23.123123.  assumes Ltime.
	Llongfile                 // full file name and line number: /a/b/c/d.go:23
	Lshortfile                // final file name element and line number: d.go:23. overrides Llongfile
)

// Logger represents an active logging object.
type Logger struct {
	out    io.Writer // destination for output
	prefix string    // prefix to write at beginning of each line
	flag   int       // properties
}

// New creates a new Logger.   The out variable sets the
// destination to which log data will be written.
// The prefix appears at the beginning of each generated log line.
// The flag argument defines the logging properties.
func New(out io.Writer, prefix string, flag int) *Logger {
	return &Logger{out, prefix, flag}
}

var std = New(os.Stderr, "", Ldate|Ltime)

// Cheap integer to fixed-width decimal ASCII.  Give a negative width to avoid zero-padding.
// Knows the buffer has capacity.
func itoa(buf *bytes.Buffer, i int, wid int) {
	var u uint = uint(i)
	if u == 0 && wid <= 1 {
		buf.WriteByte('0')
		return
	}

	// Assemble decimal in reverse order.
	var b [32]byte
	bp := len(b)
	for ; u > 0 || wid > 0; u /= 10 {
		bp--
		wid--
		b[bp] = byte(u%10) + '0'
	}

	// avoid slicing b to avoid an allocation.
	for bp < len(b) {
		buf.WriteByte(b[bp])
		bp++
	}
}

func (l *Logger) formatHeader(buf *bytes.Buffer, ns int64, calldepth int) {
	buf.WriteString(l.prefix)
	if l.flag&(Ldate|Ltime|Lmicroseconds) != 0 {
		t := time.SecondsToLocalTime(ns / 1e9)
		if l.flag&Ldate != 0 {
			itoa(buf, int(t.Year), 4)
			buf.WriteByte('/')
			itoa(buf, int(t.Month), 2)
			buf.WriteByte('/')
			itoa(buf, int(t.Day), 2)
			buf.WriteByte(' ')
		}
		if l.flag&(Ltime|Lmicroseconds) != 0 {
			itoa(buf, int(t.Hour), 2)
			buf.WriteByte(':')
			itoa(buf, int(t.Minute), 2)
			buf.WriteByte(':')
			itoa(buf, int(t.Second), 2)
			if l.flag&Lmicroseconds != 0 {
				buf.WriteByte('.')
				itoa(buf, int(ns%1e9)/1e3, 6)
			}
			buf.WriteByte(' ')
		}
	}
	if l.flag&(Lshortfile|Llongfile) != 0 {
		_, file, line, ok := runtime.Caller(calldepth)
		if ok {
			if l.flag&Lshortfile != 0 {
				short := file
				for i := len(file) - 1; i > 0; i-- {
					if file[i] == '/' {
						short = file[i+1:]
						break
					}
				}
				file = short
			}
		} else {
			file = "???"
			line = 0
		}
		buf.WriteString(file)
		buf.WriteByte(':')
		itoa(buf, line, -1)
		buf.WriteString(": ")
	}
}

// Output writes the output for a logging event.  The string s contains
// the text to print after the prefix specified by the flags of the
// Logger.  A newline is appended if the last character of s is not
// already a newline.  Calldepth is used to recover the PC and is
// provided for generality, although at the moment on all pre-defined
// paths it will be 2.
func (l *Logger) Output(calldepth int, s string) os.Error {
	now := time.Nanoseconds() // get this early.
	buf := new(bytes.Buffer)
	l.formatHeader(buf, now, calldepth+1)
	buf.WriteString(s)
	if len(s) > 0 && s[len(s)-1] != '\n' {
		buf.WriteByte('\n')
	}
	_, err := l.out.Write(buf.Bytes())
	return err
}

// Printf calls l.Output to print to the logger.
// Arguments are handled in the manner of fmt.Printf.
func (l *Logger) Printf(format string, v ...interface{}) {
	l.Output(2, fmt.Sprintf(format, v...))
}

// Print calls l.Output to print to the logger.
// Arguments are handled in the manner of fmt.Print.
func (l *Logger) Print(v ...interface{}) { l.Output(2, fmt.Sprint(v...)) }

// Println calls l.Output to print to the logger.
// Arguments are handled in the manner of fmt.Println.
func (l *Logger) Println(v ...interface{}) { l.Output(2, fmt.Sprintln(v...)) }

// SetOutput sets the output destination for the standard logger.
func SetOutput(w io.Writer) {
	std.out = w
}

// SetFlags sets the output flags for the standard logger.
func SetFlags(flag int) {
	std.flag = flag
}

// SetPrefix sets the output prefix for the standard logger.
func SetPrefix(prefix string) {
	std.prefix = prefix
}

// These functions write to the standard logger.

// Print calls Output to print to the standard logger.
// Arguments are handled in the manner of fmt.Print.
func Print(v ...interface{}) {
	std.Output(2, fmt.Sprint(v...))
}

// Printf calls Output to print to the standard logger.
// Arguments are handled in the manner of fmt.Printf.
func Printf(format string, v ...interface{}) {
	std.Output(2, fmt.Sprintf(format, v...))
}

// Println calls Output to print to the standard logger.
// Arguments are handled in the manner of fmt.Println.
func Println(v ...interface{}) {
	std.Output(2, fmt.Sprintln(v...))
}

// Exit is equivalent to Print() followed by a call to os.Exit(1).
func Exit(v ...interface{}) {
	std.Output(2, fmt.Sprint(v...))
	os.Exit(1)
}

// Exitf is equivalent to Printf() followed by a call to os.Exit(1).
func Exitf(format string, v ...interface{}) {
	std.Output(2, fmt.Sprintf(format, v...))
	os.Exit(1)
}

// Exitln is equivalent to Println() followed by a call to os.Exit(1).
func Exitln(v ...interface{}) {
	std.Output(2, fmt.Sprintln(v...))
	os.Exit(1)
}

// Panic is equivalent to Print() followed by a call to panic().
func Panic(v ...interface{}) {
	s := fmt.Sprint(v...)
	std.Output(2, s)
	panic(s)
}

// Panicf is equivalent to Printf() followed by a call to panic().
func Panicf(format string, v ...interface{}) {
	s := fmt.Sprintf(format, v...)
	std.Output(2, s)
	panic(s)
}

// Panicln is equivalent to Println() followed by a call to panic().
func Panicln(v ...interface{}) {
	s := fmt.Sprintln(v...)
	std.Output(2, s)
	panic(s)
}
