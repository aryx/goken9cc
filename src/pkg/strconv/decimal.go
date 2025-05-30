// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Multiprecision decimal numbers.
// For floating-point formatting only; not general purpose.
// Only operations are assign and (binary) left/right shift.
// Can do binary floating point in multiprecision decimal precisely
// because 2 divides 10; cannot do decimal floating point
// in multiprecision binary precisely.

package strconv

type decimal struct {
	// TODO(rsc): Can make d[] a bit smaller and add
	// truncated bool;
	d  [2000]byte // digits
	nd int        // number of digits used
	dp int        // decimal point
}

func (a *decimal) String() string {
	n := 10 + a.nd
	if a.dp > 0 {
		n += a.dp
	}
	if a.dp < 0 {
		n += -a.dp
	}

	buf := make([]byte, n)
	w := 0
	switch {
	case a.nd == 0:
		return "0"

	case a.dp <= 0:
		// zeros fill space between decimal point and digits
		buf[w] = '0'
		w++
		buf[w] = '.'
		w++
		w += digitZero(buf[w : w+-a.dp])
		w += copy(buf[w:], a.d[0:a.nd])

	case a.dp < a.nd:
		// decimal point in middle of digits
		w += copy(buf[w:], a.d[0:a.dp])
		buf[w] = '.'
		w++
		w += copy(buf[w:], a.d[a.dp:a.nd])

	default:
		// zeros fill space between digits and decimal point
		w += copy(buf[w:], a.d[0:a.nd])
		w += digitZero(buf[w : w+a.dp-a.nd])
	}
	return string(buf[0:w])
}

func digitZero(dst []byte) int {
	for i := range dst {
		dst[i] = '0'
	}
	return len(dst)
}

// trim trailing zeros from number.
// (They are meaningless; the decimal point is tracked
// independent of the number of digits.)
func trim(a *decimal) {
	for a.nd > 0 && a.d[a.nd-1] == '0' {
		a.nd--
	}
	if a.nd == 0 {
		a.dp = 0
	}
}

// Assign v to a.
func (a *decimal) Assign(v uint64) {
	var buf [50]byte

	// Write reversed decimal in buf.
	n := 0
	for v > 0 {
		v1 := v / 10
		v -= 10 * v1
		buf[n] = byte(v + '0')
		n++
		v = v1
	}

	// Reverse again to produce forward decimal in a.d.
	a.nd = 0
	for n--; n >= 0; n-- {
		a.d[a.nd] = buf[n]
		a.nd++
	}
	a.dp = a.nd
	trim(a)
}

func newDecimal(i uint64) *decimal {
	a := new(decimal)
	a.Assign(i)
	return a
}

// Maximum shift that we can do in one pass without overflow.
// Signed int has 31 bits, and we have to be able to accomodate 9<<k.
const maxShift = 27

// Binary shift right (* 2) by k bits.  k <= maxShift to avoid overflow.
func rightShift(a *decimal, k uint) {
	r := 0 // read pointer
	w := 0 // write pointer

	// Pick up enough leading digits to cover first shift.
	n := 0
	for ; n>>k == 0; r++ {
		if r >= a.nd {
			if n == 0 {
				// a == 0; shouldn't get here, but handle anyway.
				a.nd = 0
				return
			}
			for n>>k == 0 {
				n = n * 10
				r++
			}
			break
		}
		c := int(a.d[r])
		n = n*10 + c - '0'
	}
	a.dp -= r - 1

	// Pick up a digit, put down a digit.
	for ; r < a.nd; r++ {
		c := int(a.d[r])
		dig := n >> k
		n -= dig << k
		a.d[w] = byte(dig + '0')
		w++
		n = n*10 + c - '0'
	}

	// Put down extra digits.
	for n > 0 {
		dig := n >> k
		n -= dig << k
		a.d[w] = byte(dig + '0')
		w++
		n = n * 10
	}

	a.nd = w
	trim(a)
}

// Cheat sheet for left shift: table indexed by shift count giving
// number of new digits that will be introduced by that shift.
//
// For example, leftcheats[4] = {2, "625"}.  That means that
// if we are shifting by 4 (multiplying by 16), it will add 2 digits
// when the string prefix is "625" through "999", and one fewer digit
// if the string prefix is "000" through "624".
//
// Credit for this trick goes to Ken.

type leftCheat struct {
	delta  int    // number of new digits
	cutoff string //   minus one digit if original < a.
}

var leftcheats = []leftCheat{
	// Leading digits of 1/2^i = 5^i.
	// 5^23 is not an exact 64-bit floating point number,
	// so have to use bc for the math.
	/*
		seq 27 | sed 's/^/5^/' | bc |
		awk 'BEGIN{ print "\tleftCheat{ 0, \"\" }," }
		{
			log2 = log(2)/log(10)
			printf("\tleftCheat{ %d, \"%s\" },\t// * %d\n",
				int(log2*NR+1), $0, 2**NR)
		}'
	*/
	leftCheat{0, ""},
	leftCheat{1, "5"},                   // * 2
	leftCheat{1, "25"},                  // * 4
	leftCheat{1, "125"},                 // * 8
	leftCheat{2, "625"},                 // * 16
	leftCheat{2, "3125"},                // * 32
	leftCheat{2, "15625"},               // * 64
	leftCheat{3, "78125"},               // * 128
	leftCheat{3, "390625"},              // * 256
	leftCheat{3, "1953125"},             // * 512
	leftCheat{4, "9765625"},             // * 1024
	leftCheat{4, "48828125"},            // * 2048
	leftCheat{4, "244140625"},           // * 4096
	leftCheat{4, "1220703125"},          // * 8192
	leftCheat{5, "6103515625"},          // * 16384
	leftCheat{5, "30517578125"},         // * 32768
	leftCheat{5, "152587890625"},        // * 65536
	leftCheat{6, "762939453125"},        // * 131072
	leftCheat{6, "3814697265625"},       // * 262144
	leftCheat{6, "19073486328125"},      // * 524288
	leftCheat{7, "95367431640625"},      // * 1048576
	leftCheat{7, "476837158203125"},     // * 2097152
	leftCheat{7, "2384185791015625"},    // * 4194304
	leftCheat{7, "11920928955078125"},   // * 8388608
	leftCheat{8, "59604644775390625"},   // * 16777216
	leftCheat{8, "298023223876953125"},  // * 33554432
	leftCheat{8, "1490116119384765625"}, // * 67108864
	leftCheat{9, "7450580596923828125"}, // * 134217728
}

// Is the leading prefix of b lexicographically less than s?
func prefixIsLessThan(b []byte, s string) bool {
	for i := 0; i < len(s); i++ {
		if i >= len(b) {
			return true
		}
		if b[i] != s[i] {
			return b[i] < s[i]
		}
	}
	return false
}

// Binary shift left (/ 2) by k bits.  k <= maxShift to avoid overflow.
func leftShift(a *decimal, k uint) {
	delta := leftcheats[k].delta
	if prefixIsLessThan(a.d[0:a.nd], leftcheats[k].cutoff) {
		delta--
	}

	r := a.nd         // read index
	w := a.nd + delta // write index
	n := 0

	// Pick up a digit, put down a digit.
	for r--; r >= 0; r-- {
		n += (int(a.d[r]) - '0') << k
		quo := n / 10
		rem := n - 10*quo
		w--
		a.d[w] = byte(rem + '0')
		n = quo
	}

	// Put down extra digits.
	for n > 0 {
		quo := n / 10
		rem := n - 10*quo
		w--
		a.d[w] = byte(rem + '0')
		n = quo
	}

	a.nd += delta
	a.dp += delta
	trim(a)
}

// Binary shift left (k > 0) or right (k < 0).
// Returns receiver for convenience.
func (a *decimal) Shift(k int) *decimal {
	switch {
	case a.nd == 0:
		// nothing to do: a == 0
	case k > 0:
		for k > maxShift {
			leftShift(a, maxShift)
			k -= maxShift
		}
		leftShift(a, uint(k))
	case k < 0:
		for k < -maxShift {
			rightShift(a, maxShift)
			k += maxShift
		}
		rightShift(a, uint(-k))
	}
	return a
}

// If we chop a at nd digits, should we round up?
func shouldRoundUp(a *decimal, nd int) bool {
	if nd < 0 || nd >= a.nd {
		return false
	}
	if a.d[nd] == '5' && nd+1 == a.nd { // exactly halfway - round to even
		return nd > 0 && (a.d[nd-1]-'0')%2 != 0
	}
	// not halfway - digit tells all
	return a.d[nd] >= '5'
}

// Round a to nd digits (or fewer).
// Returns receiver for convenience.
// If nd is zero, it means we're rounding
// just to the left of the digits, as in
// 0.09 -> 0.1.
func (a *decimal) Round(nd int) *decimal {
	if nd < 0 || nd >= a.nd {
		return a
	}
	if shouldRoundUp(a, nd) {
		return a.RoundUp(nd)
	}
	return a.RoundDown(nd)
}

// Round a down to nd digits (or fewer).
// Returns receiver for convenience.
func (a *decimal) RoundDown(nd int) *decimal {
	if nd < 0 || nd >= a.nd {
		return a
	}
	a.nd = nd
	trim(a)
	return a
}

// Round a up to nd digits (or fewer).
// Returns receiver for convenience.
func (a *decimal) RoundUp(nd int) *decimal {
	if nd < 0 || nd >= a.nd {
		return a
	}

	// round up
	for i := nd - 1; i >= 0; i-- {
		c := a.d[i]
		if c < '9' { // can stop after this digit
			a.d[i]++
			a.nd = i + 1
			return a
		}
	}

	// Number is all 9s.
	// Change to single 1 with adjusted decimal point.
	a.d[0] = '1'
	a.nd = 1
	a.dp++
	return a
}

// Extract integer part, rounded appropriately.
// No guarantees about overflow.
func (a *decimal) RoundedInteger() uint64 {
	if a.dp > 20 {
		return 0xFFFFFFFFFFFFFFFF
	}
	var i int
	n := uint64(0)
	for i = 0; i < a.dp && i < a.nd; i++ {
		n = n*10 + uint64(a.d[i]-'0')
	}
	for ; i < a.dp; i++ {
		n *= 10
	}
	if shouldRoundUp(a, a.dp) {
		n++
	}
	return n
}
