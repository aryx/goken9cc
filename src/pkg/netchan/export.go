// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
	The netchan package implements type-safe networked channels:
	it allows the two ends of a channel to appear on different
	computers connected by a network.  It does this by transporting
	data sent to a channel on one machine so it can be recovered
	by a receive of a channel of the same type on the other.

	An exporter publishes a set of channels by name.  An importer
	connects to the exporting machine and imports the channels
	by name. After importing the channels, the two machines can
	use the channels in the usual way.

	Networked channels are not synchronized; they always behave
	as if they are buffered channels of at least one element.
*/
package netchan

// BUG: can't use range clause to receive when using ImportNValues to limit the count.

import (
	"log"
	"net"
	"os"
	"reflect"
	"sync"
)

// Export

// expLog is a logging convenience function.  The first argument must be a string.
func expLog(args ...interface{}) {
	args[0] = "netchan export: " + args[0].(string)
	log.Print(args...)
}

// An Exporter allows a set of channels to be published on a single
// network port.  A single machine may have multiple Exporters
// but they must use different ports.
type Exporter struct {
	*clientSet
	listener net.Listener
}

type expClient struct {
	*encDec
	exp     *Exporter
	mu      sync.Mutex // protects remaining fields
	errored bool       // client has been sent an error
	seqNum  int64      // sequences messages sent to client; has value of highest sent
	ackNum  int64      // highest sequence number acknowledged
}

func newClient(exp *Exporter, conn net.Conn) *expClient {
	client := new(expClient)
	client.exp = exp
	client.encDec = newEncDec(conn)
	client.seqNum = 0
	client.ackNum = 0
	return client

}

func (client *expClient) sendError(hdr *header, err string) {
	error := &error{err}
	expLog("sending error to client:", error.error)
	client.encode(hdr, payError, error) // ignore any encode error, hope client gets it
	client.mu.Lock()
	client.errored = true
	client.mu.Unlock()
}

func (client *expClient) getChan(hdr *header, dir Dir) *chanDir {
	exp := client.exp
	exp.mu.Lock()
	ech, ok := exp.chans[hdr.name]
	exp.mu.Unlock()
	if !ok {
		client.sendError(hdr, "no such channel: "+hdr.name)
		return nil
	}
	if ech.dir != dir {
		client.sendError(hdr, "wrong direction for channel: "+hdr.name)
		return nil
	}
	return ech
}

// The function run manages sends and receives for a single client.  For each
// (client Recv) request, this will launch a serveRecv goroutine to deliver
// the data for that channel, while (client Send) requests are handled as
// data arrives from the client.
func (client *expClient) run() {
	hdr := new(header)
	hdrValue := reflect.NewValue(hdr)
	req := new(request)
	reqValue := reflect.NewValue(req)
	error := new(error)
	for {
		*hdr = header{}
		if err := client.decode(hdrValue); err != nil {
			expLog("error decoding client header:", err)
			break
		}
		switch hdr.payloadType {
		case payRequest:
			*req = request{}
			if err := client.decode(reqValue); err != nil {
				expLog("error decoding client request:", err)
				break
			}
			switch req.dir {
			case Recv:
				go client.serveRecv(*hdr, req.count)
			case Send:
				// Request to send is clear as a matter of protocol
				// but not actually used by the implementation.
				// The actual sends will have payload type payData.
				// TODO: manage the count?
			default:
				error.error = "request: can't handle channel direction"
				expLog(error.error, req.dir)
				client.encode(hdr, payError, error)
			}
		case payData:
			client.serveSend(*hdr)
		case payClosed:
			client.serveClosed(*hdr)
		case payAck:
			client.mu.Lock()
			if client.ackNum != hdr.seqNum-1 {
				// Since the sequence number is incremented and the message is sent
				// in a single instance of locking client.mu, the messages are guaranteed
				// to be sent in order.  Therefore receipt of acknowledgement N means
				// all messages <=N have been seen by the recipient.  We check anyway.
				expLog("sequence out of order:", client.ackNum, hdr.seqNum)
			}
			if client.ackNum < hdr.seqNum { // If there has been an error, don't back up the count. 
				client.ackNum = hdr.seqNum
			}
			client.mu.Unlock()
		default:
			log.Exit("netchan export: unknown payload type", hdr.payloadType)
		}
	}
	client.exp.delClient(client)
}

// Send all the data on a single channel to a client asking for a Recv.
// The header is passed by value to avoid issues of overwriting.
func (client *expClient) serveRecv(hdr header, count int64) {
	ech := client.getChan(&hdr, Send)
	if ech == nil {
		return
	}
	for {
		val := ech.ch.Recv()
		if ech.ch.Closed() {
			if err := client.encode(&hdr, payClosed, nil); err != nil {
				expLog("error encoding server closed message:", err)
			}
			break
		}
		// We hold the lock during transmission to guarantee messages are
		// sent in sequence number order.  Also, we increment first so the
		// value of client.seqNum is the value of the highest used sequence
		// number, not one beyond.
		client.mu.Lock()
		client.seqNum++
		hdr.seqNum = client.seqNum
		err := client.encode(&hdr, payData, val.Interface())
		client.mu.Unlock()
		if err != nil {
			expLog("error encoding client response:", err)
			client.sendError(&hdr, err.String())
			break
		}
		// Negative count means run forever.
		if count >= 0 {
			if count--; count <= 0 {
				break
			}
		}
	}
}

// Receive and deliver locally one item from a client asking for a Send
// The header is passed by value to avoid issues of overwriting.
func (client *expClient) serveSend(hdr header) {
	ech := client.getChan(&hdr, Recv)
	if ech == nil {
		return
	}
	// Create a new value for each received item.
	val := reflect.MakeZero(ech.ch.Type().(*reflect.ChanType).Elem())
	if err := client.decode(val); err != nil {
		expLog("value decode:", err)
		return
	}
	ech.ch.Send(val)
}

// Report that client has closed the channel that is sending to us.
// The header is passed by value to avoid issues of overwriting.
func (client *expClient) serveClosed(hdr header) {
	ech := client.getChan(&hdr, Recv)
	if ech == nil {
		return
	}
	ech.ch.Close()
}

func (client *expClient) unackedCount() int64 {
	client.mu.Lock()
	n := client.seqNum - client.ackNum
	client.mu.Unlock()
	return n
}

func (client *expClient) seq() int64 {
	client.mu.Lock()
	n := client.seqNum
	client.mu.Unlock()
	return n
}

func (client *expClient) ack() int64 {
	client.mu.Lock()
	n := client.seqNum
	client.mu.Unlock()
	return n
}

// Wait for incoming connections, start a new runner for each
func (exp *Exporter) listen() {
	for {
		conn, err := exp.listener.Accept()
		if err != nil {
			expLog("listen:", err)
			break
		}
		client := exp.addClient(conn)
		go client.run()
	}
}

// NewExporter creates a new Exporter to export channels
// on the network and local address defined as in net.Listen.
func NewExporter(network, localaddr string) (*Exporter, os.Error) {
	listener, err := net.Listen(network, localaddr)
	if err != nil {
		return nil, err
	}
	e := &Exporter{
		listener: listener,
		clientSet: &clientSet{
			chans:   make(map[string]*chanDir),
			clients: make(map[unackedCounter]bool),
		},
	}
	go e.listen()
	return e, nil
}

// addClient creates a new expClient and records its existence
func (exp *Exporter) addClient(conn net.Conn) *expClient {
	client := newClient(exp, conn)
	exp.clients[client] = true
	exp.mu.Unlock()
	return client
}

// delClient forgets the client existed
func (exp *Exporter) delClient(client *expClient) {
	exp.mu.Lock()
	exp.clients[client] = false, false
	exp.mu.Unlock()
}

// Drain waits until all messages sent from this exporter/importer, including
// those not yet sent to any client and possibly including those sent while
// Drain was executing, have been received by the importer.  In short, it
// waits until all the exporter's messages have been received by a client.
// If the timeout (measured in nanoseconds) is positive and Drain takes
// longer than that to complete, an error is returned.
func (exp *Exporter) Drain(timeout int64) os.Error {
	// This wrapper function is here so the method's comment will appear in godoc.
	return exp.clientSet.drain(timeout)
}

// Sync waits until all clients of the exporter have received the messages
// that were sent at the time Sync was invoked.  Unlike Drain, it does not
// wait for messages sent while it is running or messages that have not been
// dispatched to any client.  If the timeout (measured in nanoseconds) is
// positive and Sync takes longer than that to complete, an error is
// returned.
func (exp *Exporter) Sync(timeout int64) os.Error {
	// This wrapper function is here so the method's comment will appear in godoc.
	return exp.clientSet.sync(timeout)
}

// Addr returns the Exporter's local network address.
func (exp *Exporter) Addr() net.Addr { return exp.listener.Addr() }

func checkChan(chT interface{}, dir Dir) (*reflect.ChanValue, os.Error) {
	chanType, ok := reflect.Typeof(chT).(*reflect.ChanType)
	if !ok {
		return nil, os.ErrorString("not a channel")
	}
	if dir != Send && dir != Recv {
		return nil, os.ErrorString("unknown channel direction")
	}
	switch chanType.Dir() {
	case reflect.BothDir:
	case reflect.SendDir:
		if dir != Recv {
			return nil, os.ErrorString("to import/export with Send, must provide <-chan")
		}
	case reflect.RecvDir:
		if dir != Send {
			return nil, os.ErrorString("to import/export with Recv, must provide chan<-")
		}
	}
	return reflect.NewValue(chT).(*reflect.ChanValue), nil
}

// Export exports a channel of a given type and specified direction.  The
// channel to be exported is provided in the call and may be of arbitrary
// channel type.
// Despite the literal signature, the effective signature is
//	Export(name string, chT chan T, dir Dir)
func (exp *Exporter) Export(name string, chT interface{}, dir Dir) os.Error {
	ch, err := checkChan(chT, dir)
	if err != nil {
		return err
	}
	exp.mu.Lock()
	defer exp.mu.Unlock()
	_, present := exp.chans[name]
	if present {
		return os.ErrorString("channel name already being exported:" + name)
	}
	exp.chans[name] = &chanDir{ch, dir}
	return nil
}
