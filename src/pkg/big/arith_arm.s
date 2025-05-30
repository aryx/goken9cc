// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This file provides fast assembly versions for the elementary
// arithmetic operations on vectors implemented in arith.go.

// TODO(gri) Implement these routines.
TEXT ·addVV(SB),7,$0
	B ·addVV_g(SB)

TEXT ·subVV(SB),7,$0
	B ·subVV_g(SB)

TEXT ·addVW(SB),7,$0
	B ·addVW_g(SB)

TEXT ·subVW(SB),7,$0
	B ·subVW_g(SB)

TEXT ·shlVW(SB),7,$0
	B ·shlVW_g(SB)

TEXT ·shrVW(SB),7,$0
	B ·shrVW_g(SB)

TEXT ·mulAddVWW(SB),7,$0
	B ·mulAddVWW_g(SB)

TEXT ·addMulVVW(SB),7,$0
	B ·addMulVVW_g(SB)

TEXT ·divWVW(SB),7,$0
	B ·divWVW_g(SB)

TEXT ·divWW(SB),7,$0
	B ·divWW_g(SB)

TEXT ·mulWW(SB),7,$0
	B ·mulWW_g(SB)
