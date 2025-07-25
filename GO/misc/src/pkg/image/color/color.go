// Copyright 2011 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Package color implements a basic color library.
package color

// Color can convert itself to alpha-premultiplied 16-bits per channel RGBA.
// The conversion may be lossy.
type Color interface {
	// RGBA returns the alpha-premultiplied red, green, blue and alpha values
	// for the color. Each value ranges within [0, 0xFFFF], but is represented
	// by a uint32 so that multiplying by a blend factor up to 0xFFFF will not
	// overflow.
	RGBA() (r, g, b, a uint32)
}

// RGBA represents a traditional 32-bit alpha-premultiplied color,
// having 8 bits for each of red, green, blue and alpha.
type RGBA struct {
	R, G, B, A uint8
}

func (c RGBA) RGBA() (r, g, b, a uint32) {
	r = uint32(c.R)
	r |= r << 8
	g = uint32(c.G)
	g |= g << 8
	b = uint32(c.B)
	b |= b << 8
	a = uint32(c.A)
	a |= a << 8
	return
}

// RGBA64 represents a 64-bit alpha-premultiplied color,
// having 16 bits for each of red, green, blue and alpha.
type RGBA64 struct {
	R, G, B, A uint16
}

func (c RGBA64) RGBA() (r, g, b, a uint32) {
	return uint32(c.R), uint32(c.G), uint32(c.B), uint32(c.A)
}

// NRGBA represents a non-alpha-premultiplied 32-bit color.
type NRGBA struct {
	R, G, B, A uint8
}

func (c NRGBA) RGBA() (r, g, b, a uint32) {
	r = uint32(c.R)
	r |= r << 8
	r *= uint32(c.A)
	r /= 0xff
	g = uint32(c.G)
	g |= g << 8
	g *= uint32(c.A)
	g /= 0xff
	b = uint32(c.B)
	b |= b << 8
	b *= uint32(c.A)
	b /= 0xff
	a = uint32(c.A)
	a |= a << 8
	return
}

// NRGBA64 represents a non-alpha-premultiplied 64-bit color,
// having 16 bits for each of red, green, blue and alpha.
type NRGBA64 struct {
	R, G, B, A uint16
}

func (c NRGBA64) RGBA() (r, g, b, a uint32) {
	r = uint32(c.R)
	r *= uint32(c.A)
	r /= 0xffff
	g = uint32(c.G)
	g *= uint32(c.A)
	g /= 0xffff
	b = uint32(c.B)
	b *= uint32(c.A)
	b /= 0xffff
	a = uint32(c.A)
	return
}

// Alpha represents an 8-bit alpha color.
type Alpha struct {
	A uint8
}

func (c Alpha) RGBA() (r, g, b, a uint32) {
	a = uint32(c.A)
	a |= a << 8
	return a, a, a, a
}

// Alpha16 represents a 16-bit alpha color.
type Alpha16 struct {
	A uint16
}

func (c Alpha16) RGBA() (r, g, b, a uint32) {
	a = uint32(c.A)
	return a, a, a, a
}

// Gray represents an 8-bit grayscale color.
type Gray struct {
	Y uint8
}

func (c Gray) RGBA() (r, g, b, a uint32) {
	y := uint32(c.Y)
	y |= y << 8
	return y, y, y, 0xffff
}

// Gray16 represents a 16-bit grayscale color.
type Gray16 struct {
	Y uint16
}

func (c Gray16) RGBA() (r, g, b, a uint32) {
	y := uint32(c.Y)
	return y, y, y, 0xffff
}

// Model can convert any Color to one from its own color model. The conversion
// may be lossy.
type Model interface {
	Convert(c Color) Color
}

// ModelFunc is an adapter type to allow the use of a color conversion
// function as a Model. If f is such a function, ModelFunc(f) is a Model that
// invokes f to implement the conversion.
type ModelFunc func(Color) Color

func (f ModelFunc) Convert(c Color) Color {
	return f(c)
}

// RGBAModel is the Model for RGBA colors.
var RGBAModel Model = ModelFunc(func(c Color) Color {
	if _, ok := c.(RGBA); ok {
		return c
	}
	r, g, b, a := c.RGBA()
	return RGBA{uint8(r >> 8), uint8(g >> 8), uint8(b >> 8), uint8(a >> 8)}
})

// RGBAModel is the Model for RGBA64 colors.
var RGBA64Model Model = ModelFunc(func(c Color) Color {
	if _, ok := c.(RGBA64); ok {
		return c
	}
	r, g, b, a := c.RGBA()
	return RGBA64{uint16(r), uint16(g), uint16(b), uint16(a)}
})

// NRGBAModel is the Model for NRGBA colors.
var NRGBAModel Model = ModelFunc(func(c Color) Color {
	if _, ok := c.(NRGBA); ok {
		return c
	}
	r, g, b, a := c.RGBA()
	if a == 0xffff {
		return NRGBA{uint8(r >> 8), uint8(g >> 8), uint8(b >> 8), 0xff}
	}
	if a == 0 {
		return NRGBA{0, 0, 0, 0}
	}
	// Since Color.RGBA returns a alpha-premultiplied color, we should have r <= a && g <= a && b <= a.
	r = (r * 0xffff) / a
	g = (g * 0xffff) / a
	b = (b * 0xffff) / a
	return NRGBA{uint8(r >> 8), uint8(g >> 8), uint8(b >> 8), uint8(a >> 8)}
})

// NRGBAModel is the Model for NRGBA64 colors.
var NRGBA64Model Model = ModelFunc(func(c Color) Color {
	if _, ok := c.(NRGBA64); ok {
		return c
	}
	r, g, b, a := c.RGBA()
	if a == 0xffff {
		return NRGBA64{uint16(r), uint16(g), uint16(b), 0xffff}
	}
	if a == 0 {
		return NRGBA64{0, 0, 0, 0}
	}
	// Since Color.RGBA returns a alpha-premultiplied color, we should have r <= a && g <= a && b <= a.
	r = (r * 0xffff) / a
	g = (g * 0xffff) / a
	b = (b * 0xffff) / a
	return NRGBA64{uint16(r), uint16(g), uint16(b), uint16(a)}
})

// AlphaModel is the Model for Alpha colors.
var AlphaModel Model = ModelFunc(func(c Color) Color {
	if _, ok := c.(Alpha); ok {
		return c
	}
	_, _, _, a := c.RGBA()
	return Alpha{uint8(a >> 8)}
})

// Alpha16Model is the Model for Alpha16 colors.
var Alpha16Model Model = ModelFunc(func(c Color) Color {
	if _, ok := c.(Alpha16); ok {
		return c
	}
	_, _, _, a := c.RGBA()
	return Alpha16{uint16(a)}
})

// GrayModel is the Model for Gray colors.
var GrayModel Model = ModelFunc(func(c Color) Color {
	if _, ok := c.(Gray); ok {
		return c
	}
	r, g, b, _ := c.RGBA()
	y := (299*r + 587*g + 114*b + 500) / 1000
	return Gray{uint8(y >> 8)}
})

// Gray16Model is the Model for Gray16 colors.
var Gray16Model Model = ModelFunc(func(c Color) Color {
	if _, ok := c.(Gray16); ok {
		return c
	}
	r, g, b, _ := c.RGBA()
	y := (299*r + 587*g + 114*b + 500) / 1000
	return Gray16{uint16(y)}
})

// Palette is a palette of colors.
type Palette []Color

func diff(a, b uint32) uint32 {
	if a > b {
		return a - b
	}
	return b - a
}

// Convert returns the palette color closest to c in Euclidean R,G,B space.
func (p Palette) Convert(c Color) Color {
	if len(p) == 0 {
		return nil
	}
	return p[p.Index(c)]
}

// Index returns the index of the palette color closest to c in Euclidean
// R,G,B space.
func (p Palette) Index(c Color) int {
	cr, cg, cb, _ := c.RGBA()
	// Shift by 1 bit to avoid potential uint32 overflow in sum-squared-difference.
	cr >>= 1
	cg >>= 1
	cb >>= 1
	ret, bestSSD := 0, uint32(1<<32-1)
	for i, v := range p {
		vr, vg, vb, _ := v.RGBA()
		vr >>= 1
		vg >>= 1
		vb >>= 1
		dr, dg, db := diff(cr, vr), diff(cg, vg), diff(cb, vb)
		ssd := (dr * dr) + (dg * dg) + (db * db)
		if ssd < bestSSD {
			if ssd == 0 {
				return i
			}
			ret, bestSSD = i, ssd
		}
	}
	return ret
}

var (
	// Black is an opaque black Color.
	Black = Gray16{0}
	// White is an opaque white Color.
	White = Gray16{0xffff}
	// Transparent is a fully transparent Color.
	Transparent = Alpha16{0}
	// Opaque is a fully opaque Color.
	Opaque = Alpha16{0xffff}
)
