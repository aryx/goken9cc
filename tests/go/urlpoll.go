// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import (
	"http"
	"log"
	"time"
)

const (
	numPollers     = 2           // number of Poller goroutines to launch
	second         = 1e9         // one second is 1e9 nanoseconds
	pollInterval   = 60 * second // how often to poll each URL
	statusInterval = 10 * second // how often to log status to stdout
	errTimeout     = 10 * second // back-off timeout on error
)

var urls = []string{
	"http://www.google.com/",
	"http://golang.org/",
	"http://blog.golang.org/",
}

// State represents the last-known state of a URL.
type State struct {
	url    string
	status string
}

// StateMonitor maintains a map that stores the state of the URLs being
// polled, and prints the current state every updateInterval nanoseconds.
// It returns a chan State to which resource state should be sent.
func StateMonitor(updateInterval int64) chan<- State {
	updates := make(chan State)
	urlStatus := make(map[string]string)
	ticker := time.NewTicker(updateInterval)
	go func() {
		for {
			select {
			case <-ticker.C:
				logState(urlStatus)
			case s := <-updates:
				urlStatus[s.url] = s.status
			}
		}
	}()
	return updates
}

// logState prints a state map.
func logState(s map[string]string) {
	log.Stdout("Current state:")
	for k, v := range s {
		log.Stdoutf(" %s %s", k, v)
	}
}

// Resource represents an HTTP URL to be polled by this program.
type Resource struct {
	url      string
	errCount int64
}

// Poll executes an HTTP HEAD request for url
// and returns the HTTP status string or an error string.
func (r *Resource) Poll() string {
	resp, err := http.Head(r.url)
	if err != nil {
		log.Stderr("Error", r.url, err)
		r.errCount++
		return err.String()
	}
	r.errCount = 0
	return resp.Status
}

// Sleep sleeps for an appropriate interval (dependant on error state)
// before sending the Resource to done.
func (r *Resource) Sleep(done chan *Resource) {
	time.Sleep(pollInterval + errTimeout*r.errCount)
	done <- r
}

func Poller(in <-chan *Resource, out chan<- *Resource, status chan<- State) {
	for r := range in {
		s := r.Poll()
		status <- State{r.url, s}
		out <- r
	}
}

func main() {
	// create our input and output channels
	pending, complete := make(chan *Resource), make(chan *Resource)

	// launch the StateMonitor
	status := StateMonitor(statusInterval)

	// launch some Poller goroutines
	for i := 0; i < numPollers; i++ {
		go Poller(pending, complete, status)
	}

	// send some Resources to the pending queue
	go func() {
		for _, url := range urls {
			pending <- &Resource{url: url}
		}
	}()

	for r := range complete {
		go r.Sleep(pending)
	}
}
