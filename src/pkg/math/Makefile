# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

include ../../Make.inc

TARG=math

OFILES_amd64=\
	exp_amd64.$O\
	fabs_amd64.$O\
	fdim_amd64.$O\
	hypot_amd64.$O\
	log_amd64.$O\
	sincos_amd64.$O\
	sqrt_amd64.$O\

OFILES_386=\
	asin_386.$O\
	atan_386.$O\
	atan2_386.$O\
	exp_386.$O\
	exp2_386.$O\
	expm1_386.$O\
	fabs_386.$O\
	floor_386.$O\
	frexp_386.$O\
	fmod_386.$O\
	hypot_386.$O\
	ldexp_386.$O\
	log_386.$O\
	log10_386.$O\
	log1p_386.$O\
	modf_386.$O\
	remainder_386.$O\
	sin_386.$O\
	sincos_386.$O\
	sqrt_386.$O\
	tan_386.$O\

OFILES=\
	$(OFILES_$(GOARCH))

ALLGOFILES=\
	acosh.go\
	asin.go\
	asinh.go\
	atan.go\
	atanh.go\
	atan2.go\
	bits.go\
	cbrt.go\
	const.go\
	copysign.go\
	erf.go\
	exp.go\
	exp2.go\
	expm1.go\
	fabs.go\
	fdim.go\
	floor.go\
	fmod.go\
	frexp.go\
	gamma.go\
	hypot.go\
	hypot_port.go\
	j0.go\
	j1.go\
	jn.go\
	lgamma.go\
	ldexp.go\
	log.go\
	log10.go\
	log1p.go\
	logb.go\
	modf.go\
	nextafter.go\
	pow.go\
	pow10.go\
	remainder.go\
	signbit.go\
	sin.go\
	sincos.go\
	sinh.go\
	sqrt.go\
	sqrt_port.go\
	tan.go\
	tanh.go\
	unsafe.go\

NOGOFILES=\
	$(subst _$(GOARCH).$O,.go,$(OFILES_$(GOARCH)))

GOFILES=\
	$(filter-out $(NOGOFILES),$(ALLGOFILES))\
	$(subst .go,_decl.go,$(NOGOFILES))\

include ../../Make.pkg
