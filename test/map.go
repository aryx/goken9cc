// $G $F.go && $L $F.$A && ./$A.out

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import (
	"fmt"
	"strconv"
)

const count = 100

func P(a []string) string {
	s := "{"
	for i := 0; i < len(a); i++ {
		if i > 0 {
			s += ","
		}
		s += `"` + a[i] + `"`
	}
	s +="}"
	return s
}

func main() {
	// Test a map literal.
	mlit := map[string] int { "0":0, "1":1, "2":2, "3":3, "4":4 }
	for i := 0; i < len(mlit); i++ {
		s := string([]byte{byte(i)+'0'})
		if mlit[s] != i {
			fmt.Printf("mlit[%s] = %d\n", s, mlit[s])
		}
	}

	mib := make(map[int] bool)
	mii := make(map[int] int)
	mfi := make(map[float] int)
	mif := make(map[int] float)
	msi := make(map[string] int)
	mis := make(map[int] string)
	mss := make(map[string] string)
	mspa := make(map[string] []string)
	// BUG need an interface map both ways too

	type T struct {
		i int64	// can't use string here; struct values are only compared at the top level
		f float
	}
	mipT := make(map[int] *T)
	mpTi := make(map[*T] int)
	mit := make(map[int] T)
//	mti := make(map[T] int)

	type M map[int] int
	mipM := make(map[int] M)

	var apT [2*count]*T

	for i := 0; i < count; i++ {
		s := strconv.Itoa(i)
		s10 := strconv.Itoa(i*10)
		f := float(i)
		t := T{int64(i),f}
		apT[i] = new(T)
		apT[i].i = int64(i)
		apT[i].f = f
		apT[2*i] = new(T)	// need twice as many entries as we use, for the nonexistence check
		apT[2*i].i = int64(i)
		apT[2*i].f = f
		m := M{i: i+1}
		mib[i] = (i != 0)
		mii[i] = 10*i
		mfi[float(i)] = 10*i
		mif[i] = 10.0*f
		mis[i] = s
		msi[s] = i
		mss[s] = s10
		mss[s] = s10
		as := make([]string, 2)
			as[0] = s10
			as[1] = s10
		mspa[s] = as
		mipT[i] = apT[i]
		mpTi[apT[i]] = i
		mipM[i] = m
		mit[i] = t
	//	mti[t] = i
	}

	// test len
	if len(mib) != count {
		fmt.Printf("len(mib) = %d\n", len(mib))
	}
	if len(mii) != count {
		fmt.Printf("len(mii) = %d\n", len(mii))
	}
	if len(mfi) != count {
		fmt.Printf("len(mfi) = %d\n", len(mfi))
	}
	if len(mif) != count {
		fmt.Printf("len(mif) = %d\n", len(mif))
	}
	if len(msi) != count {
		fmt.Printf("len(msi) = %d\n", len(msi))
	}
	if len(mis) != count {
		fmt.Printf("len(mis) = %d\n", len(mis))
	}
	if len(mss) != count {
		fmt.Printf("len(mss) = %d\n", len(mss))
	}
	if len(mspa) != count {
		fmt.Printf("len(mspa) = %d\n", len(mspa))
	}
	if len(mipT) != count {
		fmt.Printf("len(mipT) = %d\n", len(mipT))
	}
	if len(mpTi) != count {
		fmt.Printf("len(mpTi) = %d\n", len(mpTi))
	}
//	if len(mti) != count {
//		fmt.Printf("len(mti) = %d\n", len(mti))
//	}
	if len(mipM) != count {
		fmt.Printf("len(mipM) = %d\n", len(mipM))
	}
//	if len(mti) != count {
//		fmt.Printf("len(mti) = %d\n", len(mti))
//	}
	if len(mit) != count {
		fmt.Printf("len(mit) = %d\n", len(mit))
	}

	// test construction directly
	for i := 0; i < count; i++ {
		s := strconv.Itoa(i)
		s10 := strconv.Itoa(i*10)
		f := float(i)
		// BUG m := M(i, i+1)
		if mib[i] != (i != 0) {
			fmt.Printf("mib[%d] = %t\n", i, mib[i])
		}
		if(mii[i] != 10*i) {
			fmt.Printf("mii[%d] = %d\n", i, mii[i])
		}
		if(mfi[f] != 10*i) {
			fmt.Printf("mfi[%d] = %d\n", i, mfi[f])
		}
		if(mif[i] != 10.0*f) {
			fmt.Printf("mif[%d] = %g\n", i, mif[i])
		}
		if(mis[i] != s) {
			fmt.Printf("mis[%d] = %s\n", i, mis[i])
		}
		if(msi[s] != i) {
			fmt.Printf("msi[%s] = %d\n", s, msi[s])
		}
		if mss[s] != s10 {
			fmt.Printf("mss[%s] = %g\n", s, mss[s])
		}
		for j := 0; j < len(mspa[s]); j++ {
			if mspa[s][j] != s10 {
				fmt.Printf("mspa[%s][%d] = %s\n", s, j, mspa[s][j])
			}
		}
		if(mipT[i].i != int64(i) || mipT[i].f != f) {
			fmt.Printf("mipT[%d] = %v\n", i, mipT[i])
		}
		if(mpTi[apT[i]] != i) {
			fmt.Printf("mpTi[apT[%d]] = %d\n", i, mpTi[apT[i]])
		}
	//	if(mti[t] != i) {
	//		fmt.Printf("mti[%s] = %s\n", s, mti[t])
	//	}
		if (mipM[i][i] != i + 1) {
			fmt.Printf("mipM[%d][%d] = %d\n", i, i, mipM[i][i])
		}
	//	if(mti[t] != i) {
	//		fmt.Printf("mti[%v] = %d\n", t, mti[t])
	//	}
		if(mit[i].i != int64(i) || mit[i].f != f) {
			fmt.Printf("mit[%d] = {%d %g}\n", i, mit[i].i, mit[i].f)
		}
	}

	// test existence with tuple check
	// failed lookups yield a false value for the boolean.
	for i := 0; i < count; i++ {
		s := strconv.Itoa(i)
		f := float(i)
		{
			_, b := mib[i]
			if !b {
				fmt.Printf("tuple existence decl: mib[%d]\n", i)
			}
			_, b = mib[i]
			if !b {
				fmt.Printf("tuple existence assign: mib[%d]\n", i)
			}
		}
		{
			_, b := mii[i]
			if !b {
				fmt.Printf("tuple existence decl: mii[%d]\n", i)
			}
			_, b = mii[i]
			if !b {
				fmt.Printf("tuple existence assign: mii[%d]\n", i)
			}
		}
		{
			_, b := mfi[f]
			if !b {
				fmt.Printf("tuple existence decl: mfi[%d]\n", i)
			}
			_, b = mfi[f]
			if !b {
				fmt.Printf("tuple existence assign: mfi[%d]\n", i)
			}
		}
		{
			_, b := mif[i]
			if !b {
				fmt.Printf("tuple existence decl: mif[%d]\n", i)
			}
			_, b = mif[i]
			if !b {
				fmt.Printf("tuple existence assign: mif[%d]\n", i)
			}
		}
		{
			_, b := mis[i]
			if !b {
				fmt.Printf("tuple existence decl: mis[%d]\n", i)
			}
			_, b = mis[i]
			if !b {
				fmt.Printf("tuple existence assign: mis[%d]\n", i)
			}
		}
		{
			_, b := msi[s]
			if !b {
				fmt.Printf("tuple existence decl: msi[%d]\n", i)
			}
			_, b = msi[s]
			if !b {
				fmt.Printf("tuple existence assign: msi[%d]\n", i)
			}
		}
		{
			_, b := mss[s]
			if !b {
				fmt.Printf("tuple existence decl: mss[%d]\n", i)
			}
			_, b = mss[s]
			if !b {
				fmt.Printf("tuple existence assign: mss[%d]\n", i)
			}
		}
		{
			_, b := mspa[s]
			if !b {
				fmt.Printf("tuple existence decl: mspa[%d]\n", i)
			}
			_, b = mspa[s]
			if !b {
				fmt.Printf("tuple existence assign: mspa[%d]\n", i)
			}
		}
		{
			_, b := mipT[i]
			if !b {
				fmt.Printf("tuple existence decl: mipT[%d]\n", i)
			}
			_, b = mipT[i]
			if !b {
				fmt.Printf("tuple existence assign: mipT[%d]\n", i)
			}
		}
		{
			_, b := mpTi[apT[i]]
			if !b {
				fmt.Printf("tuple existence decl: mpTi[apT[%d]]\n", i)
			}
			_, b = mpTi[apT[i]]
			if !b {
				fmt.Printf("tuple existence assign: mpTi[apT[%d]]\n", i)
			}
		}
		{
			_, b := mipM[i]
			if !b {
				fmt.Printf("tuple existence decl: mipM[%d]\n", i)
			}
			_, b = mipM[i]
			if !b {
				fmt.Printf("tuple existence assign: mipM[%d]\n", i)
			}
		}
		{
			_, b := mit[i]
			if !b {
				fmt.Printf("tuple existence decl: mit[%d]\n", i)
			}
			_, b = mit[i]
			if !b {
				fmt.Printf("tuple existence assign: mit[%d]\n", i)
			}
		}
//		{
//			_, b := mti[t]
//			if !b {
//				fmt.Printf("tuple existence decl: mti[%d]\n", i)
//			}
//			_, b = mti[t]
//			if !b {
//				fmt.Printf("tuple existence assign: mti[%d]\n", i)
//			}
//		}
	}

	// test nonexistence with tuple check
	// failed lookups yield a false value for the boolean.
	for i := count; i < 2*count; i++ {
		s := strconv.Itoa(i)
		f := float(i)
		{
			_, b := mib[i]
			if b {
				fmt.Printf("tuple nonexistence decl: mib[%d]", i)
			}
			_, b = mib[i]
			if b {
				fmt.Printf("tuple nonexistence assign: mib[%d]", i)
			}
		}
		{
			_, b := mii[i]
			if b {
				fmt.Printf("tuple nonexistence decl: mii[%d]", i)
			}
			_, b = mii[i]
			if b {
				fmt.Printf("tuple nonexistence assign: mii[%d]", i)
			}
		}
		{
			_, b := mfi[f]
			if b {
				fmt.Printf("tuple nonexistence decl: mfi[%d]", i)
			}
			_, b = mfi[f]
			if b {
				fmt.Printf("tuple nonexistence assign: mfi[%d]", i)
			}
		}
		{
			_, b := mif[i]
			if b {
				fmt.Printf("tuple nonexistence decl: mif[%d]", i)
			}
			_, b = mif[i]
			if b {
				fmt.Printf("tuple nonexistence assign: mif[%d]", i)
			}
		}
		{
			_, b := mis[i]
			if b {
				fmt.Printf("tuple nonexistence decl: mis[%d]", i)
			}
			_, b = mis[i]
			if b {
				fmt.Printf("tuple nonexistence assign: mis[%d]", i)
			}
		}
		{
			_, b := msi[s]
			if b {
				fmt.Printf("tuple nonexistence decl: msi[%d]", i)
			}
			_, b = msi[s]
			if b {
				fmt.Printf("tuple nonexistence assign: msi[%d]", i)
			}
		}
		{
			_, b := mss[s]
			if b {
				fmt.Printf("tuple nonexistence decl: mss[%d]", i)
			}
			_, b = mss[s]
			if b {
				fmt.Printf("tuple nonexistence assign: mss[%d]", i)
			}
		}
		{
			_, b := mspa[s]
			if b {
				fmt.Printf("tuple nonexistence decl: mspa[%d]", i)
			}
			_, b = mspa[s]
			if b {
				fmt.Printf("tuple nonexistence assign: mspa[%d]", i)
			}
		}
		{
			_, b := mipT[i]
			if b {
				fmt.Printf("tuple nonexistence decl: mipT[%d]", i)
			}
			_, b = mipT[i]
			if b {
				fmt.Printf("tuple nonexistence assign: mipT[%d]", i)
			}
		}
		{
			_, b := mpTi[apT[i]]
			if b {
				fmt.Printf("tuple nonexistence decl: mpTi[apt[%d]]", i)
			}
			_, b = mpTi[apT[i]]
			if b {
				fmt.Printf("tuple nonexistence assign: mpTi[apT[%d]]", i)
			}
		}
		{
			_, b := mipM[i]
			if b {
				fmt.Printf("tuple nonexistence decl: mipM[%d]", i)
			}
			_, b = mipM[i]
			if b {
				fmt.Printf("tuple nonexistence assign: mipM[%d]", i)
			}
		}
//		{
//			_, b := mti[t]
//			if b {
//				fmt.Printf("tuple nonexistence decl: mti[%d]", i)
//			}
//			_, b = mti[t]
//			if b {
//				fmt.Printf("tuple nonexistence assign: mti[%d]", i)
//			}
//		}
		{
			_, b := mit[i]
			if b {
				fmt.Printf("tuple nonexistence decl: mit[%d]", i)
			}
			_, b = mit[i]
			if b {
				fmt.Printf("tuple nonexistence assign: mit[%d]", i)
			}
		}
	}


	// tests for structured map element updates
	for i := 0; i < count; i++ {
		s := strconv.Itoa(i)
		mspa[s][i % 2] = "deleted"
		if mspa[s][i % 2] != "deleted" {
			fmt.Printf("update mspa[%s][%d] = %s\n", s, i %2, mspa[s][i % 2])
		}

		mipT[i].i += 1
		if mipT[i].i != int64(i)+1 {
			fmt.Printf("update mipT[%d].i = %d\n", i, mipT[i].i)
		}
		mipT[i].f = float(i + 1)
		if (mipT[i].f != float(i + 1)) {
			fmt.Printf("update mipT[%d].f = %g\n", i, mipT[i].f)
		}

		mipM[i][i]++
		if mipM[i][i] != (i + 1) + 1 {
			fmt.Printf("update mipM[%d][%d] = %i\n", i, i, mipM[i][i])
		}
	}

	// test range on nil map
	var mnil map[string] int
	for _, _ = range mnil {
		panic("range mnil")
	}
}
