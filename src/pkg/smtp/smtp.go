// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Package smtp implements the Simple Mail Transfer Protocol as defined in RFC 5321.
// It also implements the following extensions:
//	8BITMIME  RFC 1652
//	AUTH      RFC 2554
//	STARTTLS  RFC 3207
// Additional extensions may be handled by clients.
package smtp

import (
	"crypto/tls"
	"encoding/base64"
	"io"
	"os"
	"net"
	"net/textproto"
	"strings"
)

// A Client represents a client connection to an SMTP server.
type Client struct {
	// Text is the textproto.Conn used by the Client. It is exported to allow for
	// clients to add extensions.
	Text *textproto.Conn
	// keep a reference to the connection so it can be used to create a TLS
	// connection later
	conn net.Conn
	// whether the Client is using TLS
	tls        bool
	serverName string
	// map of supported extensions
	ext map[string]string
	// supported auth mechanisms
	auth []string
}

// Dial returns a new Client connected to an SMTP server at addr.
func Dial(addr string) (*Client, os.Error) {
	conn, err := net.Dial("tcp", "", addr)
	if err != nil {
		return nil, err
	}
	host := addr[:strings.Index(addr, ":")]
	return NewClient(conn, host)
}

// NewClient returns a new Client using an existing connection and host as a
// server name to be used when authenticating.
func NewClient(conn net.Conn, host string) (*Client, os.Error) {
	text := textproto.NewConn(conn)
	_, msg, err := text.ReadResponse(220)
	if err != nil {
		text.Close()
		return nil, err
	}
	c := &Client{Text: text, conn: conn, serverName: host}
	if strings.Index(msg, "ESMTP") >= 0 {
		err = c.ehlo()
	} else {
		err = c.helo()
	}
	return c, err
}

// cmd is a convenience function that sends a command and returns the response
func (c *Client) cmd(expectCode int, format string, args ...interface{}) (int, string, os.Error) {
	id, err := c.Text.Cmd(format, args...)
	if err != nil {
		return 0, "", err
	}
	c.Text.StartResponse(id)
	defer c.Text.EndResponse(id)
	code, msg, err := c.Text.ReadResponse(expectCode)
	return code, msg, err
}

// helo sends the HELO greeting to the server. It should be used only when the
// server does not support ehlo.
func (c *Client) helo() os.Error {
	c.ext = nil
	_, _, err := c.cmd(250, "HELO localhost")
	return err
}

// ehlo sends the EHLO (extended hello) greeting to the server. It
// should be the preferred greeting for servers that support it.
func (c *Client) ehlo() os.Error {
	_, msg, err := c.cmd(250, "EHLO localhost")
	if err != nil {
		return err
	}
	ext := make(map[string]string)
	extList := strings.Split(msg, "\n", -1)
	if len(extList) > 1 {
		extList = extList[1:]
		for _, line := range extList {
			args := strings.Split(line, " ", 2)
			if len(args) > 1 {
				ext[args[0]] = args[1]
			} else {
				ext[args[0]] = ""
			}
		}
	}
	if mechs, ok := ext["AUTH"]; ok {
		c.auth = strings.Split(mechs, " ", -1)
	}
	c.ext = ext
	return err
}

// StartTLS sends the STARTTLS command and encrypts all further communication.
// Only servers that advertise the STARTTLS extension support this function.
func (c *Client) StartTLS() os.Error {
	_, _, err := c.cmd(220, "STARTTLS")
	if err != nil {
		return err
	}
	c.conn = tls.Client(c.conn, nil)
	c.Text = textproto.NewConn(c.conn)
	c.tls = true
	return c.ehlo()
}

// Verify checks the validity of an email address on the server.
// If Verify returns nil, the address is valid. A non-nil return
// does not necessarily indicate an invalid address. Many servers
// will not verify addresses for security reasons.
func (c *Client) Verify(addr string) os.Error {
	_, _, err := c.cmd(250, "VRFY %s", addr)
	return err
}

// Auth authenticates a client using the provided authentication mechanism.
// A failed authentication closes the connection.
// Only servers that advertise the AUTH extension support this function.
func (c *Client) Auth(a Auth) os.Error {
	encoding := base64.StdEncoding
	mech, resp, err := a.Start(&ServerInfo{c.serverName, c.tls, c.auth})
	if err != nil {
		c.Quit()
		return err
	}
	resp64 := make([]byte, encoding.EncodedLen(len(resp)))
	encoding.Encode(resp64, resp)
	code, msg64, err := c.cmd(0, "AUTH %s %s", mech, resp64)
	for err == nil {
		var msg []byte
		switch code {
		case 334:
			msg = make([]byte, encoding.DecodedLen(len(msg64)))
			_, err = encoding.Decode(msg, []byte(msg64))
		case 235:
			// the last message isn't base64 because it isn't a challenge
			msg = []byte(msg64)
		default:
			err = &textproto.Error{code, msg64}
		}
		resp, err = a.Next(msg, code == 334)
		if err != nil {
			// abort the AUTH
			c.cmd(501, "*")
			c.Quit()
			break
		}
		if resp == nil {
			break
		}
		resp64 = make([]byte, encoding.EncodedLen(len(resp)))
		encoding.Encode(resp64, resp)
		code, msg64, err = c.cmd(0, string(resp64))
	}
	return err
}

// Mail issues a MAIL command to the server using the provided email address.
// If the server supports the 8BITMIME extension, Mail adds the BODY=8BITMIME
// parameter.
// This initiates a mail transaction and is followed by one or more Rcpt calls.
func (c *Client) Mail(from string) os.Error {
	cmdStr := "MAIL FROM:<%s>"
	if c.ext != nil {
		if _, ok := c.ext["8BITMIME"]; ok {
			cmdStr += " BODY=8BITMIME"
		}
	}
	_, _, err := c.cmd(250, cmdStr, from)
	return err
}

// Rcpt issues a RCPT command to the server using the provided email address.
// A call to Rcpt must be preceded by a call to Mail and may be followed by
// a Data call or another Rcpt call.
func (c *Client) Rcpt(to string) os.Error {
	_, _, err := c.cmd(25, "RCPT TO:<%s>", to)
	return err
}

type dataCloser struct {
	c *Client
	io.WriteCloser
}

func (d *dataCloser) Close() os.Error {
	d.WriteCloser.Close()
	_, _, err := d.c.Text.ReadResponse(250)
	return err
}

// Data issues a DATA command to the server and returns a writer that
// can be used to write the data. The caller should close the writer
// before calling any more methods on c.
// A call to Data must be preceded by one or more calls to Rcpt.
func (c *Client) Data() (io.WriteCloser, os.Error) {
	_, _, err := c.cmd(354, "DATA")
	if err != nil {
		return nil, err
	}
	return &dataCloser{c, c.Text.DotWriter()}, nil
}

// SendMail connects to the server at addr, switches to TLS if possible,
// authenticates with mechanism a if possible, and then sends an email from
// address from, to addresses to, with message msg.
func SendMail(addr string, a Auth, from string, to []string, msg []byte) os.Error {
	c, err := Dial(addr)
	if err != nil {
		return err
	}
	if ok, _ := c.Extension("STARTTLS"); ok {
		if err = c.StartTLS(); err != nil {
			return err
		}
	}
	if a != nil && c.ext != nil {
		if _, ok := c.ext["AUTH"]; ok {
			if err = c.Auth(a); err != nil {
				return err
			}
		}
	}
	if err = c.Mail(from); err != nil {
		return err
	}
	for _, addr := range to {
		if err = c.Rcpt(addr); err != nil {
			return err
		}
	}
	w, err := c.Data()
	if err != nil {
		return err
	}
	_, err = w.Write(msg)
	if err != nil {
		return err
	}
	err = w.Close()
	if err != nil {
		return err
	}
	return c.Quit()
}

// Extension reports whether an extension is support by the server.
// The extension name is case-insensitive. If the extension is supported,
// Extension also returns a string that contains any parameters the
// server specifies for the extension.
func (c *Client) Extension(ext string) (bool, string) {
	if c.ext == nil {
		return false, ""
	}
	ext = strings.ToUpper(ext)
	param, ok := c.ext[ext]
	return ok, param
}

// Reset sends the RSET command to the server, aborting the current mail
// transaction.
func (c *Client) Reset() os.Error {
	_, _, err := c.cmd(250, "RSET")
	return err
}

// Quit sends the QUIT command and closes the connection to the server.
func (c *Client) Quit() os.Error {
	_, _, err := c.cmd(221, "QUIT")
	if err != nil {
		return err
	}
	return c.Text.Close()
}
