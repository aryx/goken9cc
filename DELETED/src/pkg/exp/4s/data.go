// games/4s - a tetris clone
//
// Derived from Plan 9's /sys/src/games/xs.c
// http://plan9.bell-labs.com/sources/plan9/sys/src/games/xs.c
//
// Copyright (C) 2003, Lucent Technologies Inc. and others. All Rights Reserved.
// Portions Copyright 2009 The Go Authors.  All Rights Reserved.
// Distributed under the terms of the Lucent Public License Version 1.02
// See http://plan9.bell-labs.com/plan9/license.html

package main

import . "image"

var pieces4 = []Piece{
	Piece{0, 0, Point{4, 1}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{1, 0, Point{1, 4}, []Point{Point{0, 0}, Point{0, 1}, Point{0, 1}, Point{0, 1}}, nil, nil},
	Piece{2, 0, Point{4, 1}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{3, 0, Point{1, 4}, []Point{Point{0, 0}, Point{0, 1}, Point{0, 1}, Point{0, 1}}, nil, nil},

	Piece{0, 3, Point{2, 2}, []Point{Point{0, 1}, Point{1, 0}, Point{0, -1}, Point{-1, 0}}, nil, nil},
	Piece{1, 3, Point{2, 2}, []Point{Point{0, 1}, Point{1, 0}, Point{0, -1}, Point{-1, 0}}, nil, nil},
	Piece{2, 3, Point{2, 2}, []Point{Point{0, 1}, Point{1, 0}, Point{0, -1}, Point{-1, 0}}, nil, nil},
	Piece{3, 3, Point{2, 2}, []Point{Point{0, 1}, Point{1, 0}, Point{0, -1}, Point{-1, 0}}, nil, nil},

	Piece{0, 1, Point{3, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{1, 1, Point{2, 3}, []Point{Point{1, 0}, Point{0, 1}, Point{0, 1}, Point{-1, 0}}, nil, nil},
	Piece{2, 1, Point{3, 2}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{3, 1, Point{2, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{-1, 1}, Point{0, 1}}, nil, nil},

	Piece{0, 2, Point{3, 2}, []Point{Point{0, 1}, Point{1, 0}, Point{1, 0}, Point{0, -1}}, nil, nil},
	Piece{1, 2, Point{2, 3}, []Point{Point{0, 0}, Point{0, 1}, Point{0, 1}, Point{1, 0}}, nil, nil},
	Piece{2, 2, Point{3, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{-2, 1}}, nil, nil},
	Piece{3, 2, Point{2, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{0, 1}, Point{0, 1}}, nil, nil},

	Piece{0, 4, Point{3, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{-1, 1}}, nil, nil},
	Piece{1, 4, Point{2, 3}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{2, 4, Point{3, 2}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{3, 4, Point{2, 3}, []Point{Point{0, 0}, Point{0, 1}, Point{0, 1}, Point{1, -1}}, nil, nil},

	Piece{0, 5, Point{3, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{0, 1}, Point{1, 0}}, nil, nil},
	Piece{1, 5, Point{2, 3}, []Point{Point{1, 0}, Point{0, 1}, Point{-1, 0}, Point{0, 1}}, nil, nil},
	Piece{2, 5, Point{3, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{0, 1}, Point{1, 0}}, nil, nil},
	Piece{3, 5, Point{2, 3}, []Point{Point{1, 0}, Point{0, 1}, Point{-1, 0}, Point{0, 1}}, nil, nil},

	Piece{0, 6, Point{3, 2}, []Point{Point{0, 1}, Point{1, 0}, Point{0, -1}, Point{1, 0}}, nil, nil},
	Piece{1, 6, Point{2, 3}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{2, 6, Point{3, 2}, []Point{Point{0, 1}, Point{1, 0}, Point{0, -1}, Point{1, 0}}, nil, nil},
	Piece{3, 6, Point{2, 3}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{0, 1}}, nil, nil},
}

var pieces5 = []Piece{
	Piece{0, 1, Point{5, 1}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{1, 1, Point{1, 5}, []Point{Point{0, 0}, Point{0, 1}, Point{0, 1}, Point{0, 1}, Point{0, 1}}, nil, nil},
	Piece{2, 1, Point{5, 1}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{3, 1, Point{1, 5}, []Point{Point{0, 0}, Point{0, 1}, Point{0, 1}, Point{0, 1}, Point{0, 1}}, nil, nil},

	Piece{0, 0, Point{4, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{1, 0, Point{2, 4}, []Point{Point{1, 0}, Point{0, 1}, Point{0, 1}, Point{0, 1}, Point{-1, 0}}, nil, nil},
	Piece{2, 0, Point{4, 2}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{3, 0, Point{2, 4}, []Point{Point{0, 0}, Point{1, 0}, Point{-1, 1}, Point{0, 1}, Point{0, 1}}, nil, nil},

	Piece{0, 2, Point{4, 2}, []Point{Point{0, 0}, Point{0, 1}, Point{1, -1}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{1, 2, Point{2, 4}, []Point{Point{0, 0}, Point{1, 0}, Point{0, 1}, Point{0, 1}, Point{0, 1}}, nil, nil},
	Piece{2, 2, Point{4, 2}, []Point{Point{0, 1}, Point{1, 0}, Point{1, 0}, Point{1, 0}, Point{0, -1}}, nil, nil},
	Piece{3, 2, Point{2, 4}, []Point{Point{0, 0}, Point{0, 1}, Point{0, 1}, Point{0, 1}, Point{1, 0}}, nil, nil},

	Piece{0, 7, Point{3, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{0, 1}, Point{0, 1}}, nil, nil},
	Piece{1, 7, Point{3, 3}, []Point{Point{0, 2}, Point{1, 0}, Point{1, 0}, Point{0, -1}, Point{0, -1}}, nil, nil},
	Piece{2, 7, Point{3, 3}, []Point{Point{0, 0}, Point{0, 1}, Point{0, 1}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{3, 7, Point{3, 3}, []Point{Point{0, 2}, Point{0, -1}, Point{0, -1}, Point{1, 0}, Point{1, 0}}, nil, nil},

	Piece{0, 3, Point{3, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{-2, 1}, Point{1, 0}}, nil, nil},
	Piece{1, 3, Point{2, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{2, 3, Point{3, 2}, []Point{Point{1, 0}, Point{1, 0}, Point{-2, 1}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{3, 3, Point{2, 3}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{-1, 1}, Point{1, 0}}, nil, nil},

	Piece{0, 4, Point{3, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{-1, 1}, Point{1, 0}}, nil, nil},
	Piece{1, 4, Point{2, 3}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{-1, 1}, Point{1, 0}}, nil, nil},
	Piece{2, 4, Point{3, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{3, 4, Point{2, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{-1, 1}}, nil, nil},

	Piece{0, 7, Point{3, 2}, []Point{Point{0, 0}, Point{2, 0}, Point{-2, 1}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{1, 7, Point{2, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{-1, 1}, Point{0, 1}, Point{1, 0}}, nil, nil},
	Piece{2, 7, Point{3, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{-2, 1}, Point{2, 0}}, nil, nil},
	Piece{3, 7, Point{2, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{0, 1}, Point{-1, 1}, Point{1, 0}}, nil, nil},

	Piece{0, 5, Point{3, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{0, 1}, Point{1, 0}, Point{-1, 1}}, nil, nil},
	Piece{1, 5, Point{3, 3}, []Point{Point{2, 0}, Point{-2, 1}, Point{1, 0}, Point{1, 0}, Point{-1, 1}}, nil, nil},
	Piece{2, 5, Point{3, 3}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{0, 1}, Point{1, 0}}, nil, nil},
	Piece{3, 5, Point{3, 3}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{1, 0}, Point{-2, 1}}, nil, nil},

	Piece{0, 6, Point{3, 3}, []Point{Point{1, 0}, Point{1, 0}, Point{-2, 1}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{1, 6, Point{3, 3}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{2, 6, Point{3, 3}, []Point{Point{1, 0}, Point{0, 1}, Point{1, 0}, Point{-2, 1}, Point{1, 0}}, nil, nil},
	Piece{3, 6, Point{3, 3}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{1, 0}, Point{-1, 1}}, nil, nil},

	Piece{0, 0, Point{4, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{1, 0}, Point{-2, 1}}, nil, nil},
	Piece{1, 0, Point{2, 4}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{0, 1}, Point{0, 1}}, nil, nil},
	Piece{2, 0, Point{4, 2}, []Point{Point{2, 0}, Point{-2, 1}, Point{1, 0}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{3, 0, Point{2, 4}, []Point{Point{0, 0}, Point{0, 1}, Point{0, 1}, Point{1, 0}, Point{-1, 1}}, nil, nil},

	Piece{0, 2, Point{4, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{1, 0}, Point{-1, 1}}, nil, nil},
	Piece{1, 2, Point{2, 4}, []Point{Point{1, 0}, Point{0, 1}, Point{-1, 1}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{2, 2, Point{4, 2}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{3, 2, Point{2, 4}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{-1, 1}, Point{0, 1}}, nil, nil},

	Piece{0, 1, Point{3, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{0, 1}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{1, 1, Point{3, 3}, []Point{Point{2, 0}, Point{-1, 1}, Point{1, 0}, Point{-2, 1}, Point{1, 0}}, nil, nil},
	Piece{2, 1, Point{3, 3}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{0, 1}, Point{1, 0}}, nil, nil},
	Piece{3, 1, Point{3, 3}, []Point{Point{1, 0}, Point{1, 0}, Point{-2, 1}, Point{1, 0}, Point{-1, 1}}, nil, nil},

	Piece{0, 3, Point{3, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{-1, 1}, Point{0, 1}}, nil, nil},
	Piece{1, 3, Point{3, 3}, []Point{Point{2, 0}, Point{-2, 1}, Point{1, 0}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{2, 3, Point{3, 3}, []Point{Point{1, 0}, Point{0, 1}, Point{-1, 1}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{3, 3, Point{3, 3}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{1, 0}, Point{-2, 1}}, nil, nil},

	Piece{0, 4, Point{3, 3}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{1, 0}, Point{-1, 1}}, nil, nil},
	Piece{1, 4, Point{3, 3}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{1, 0}, Point{-1, 1}}, nil, nil},
	Piece{2, 4, Point{3, 3}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{1, 0}, Point{-1, 1}}, nil, nil},
	Piece{3, 4, Point{3, 3}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{1, 0}, Point{-1, 1}}, nil, nil},

	Piece{0, 8, Point{4, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{0, 1}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{1, 8, Point{2, 4}, []Point{Point{1, 0}, Point{-1, 1}, Point{1, 0}, Point{-1, 1}, Point{0, 1}}, nil, nil},
	Piece{2, 8, Point{4, 2}, []Point{Point{0, 0}, Point{1, 0}, Point{1, 0}, Point{0, 1}, Point{1, 0}}, nil, nil},
	Piece{3, 8, Point{2, 4}, []Point{Point{1, 0}, Point{0, 1}, Point{-1, 1}, Point{1, 0}, Point{-1, 1}}, nil, nil},

	Piece{0, 9, Point{4, 2}, []Point{Point{2, 0}, Point{1, 0}, Point{-3, 1}, Point{1, 0}, Point{1, 0}}, nil, nil},
	Piece{1, 9, Point{2, 4}, []Point{Point{0, 0}, Point{0, 1}, Point{0, 1}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{2, 9, Point{4, 2}, []Point{Point{1, 0}, Point{1, 0}, Point{1, 0}, Point{-3, 1}, Point{1, 0}}, nil, nil},
	Piece{3, 9, Point{2, 4}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{0, 1}, Point{0, 1}}, nil, nil},

	Piece{0, 5, Point{3, 3}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{1, 5, Point{3, 3}, []Point{Point{1, 0}, Point{1, 0}, Point{-1, 1}, Point{-1, 1}, Point{1, 0}}, nil, nil},
	Piece{2, 5, Point{3, 3}, []Point{Point{0, 0}, Point{0, 1}, Point{1, 0}, Point{1, 0}, Point{0, 1}}, nil, nil},
	Piece{3, 5, Point{3, 3}, []Point{Point{1, 0}, Point{1, 0}, Point{-1, 1}, Point{-1, 1}, Point{1, 0}}, nil, nil},

	Piece{0, 6, Point{3, 3}, []Point{Point{2, 0}, Point{-2, 1}, Point{1, 0}, Point{1, 0}, Point{-2, 1}}, nil, nil},
	Piece{1, 6, Point{3, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{0, 1}, Point{0, 1}, Point{1, 0}}, nil, nil},
	Piece{2, 6, Point{3, 3}, []Point{Point{2, 0}, Point{-2, 1}, Point{1, 0}, Point{1, 0}, Point{-2, 1}}, nil, nil},
	Piece{3, 6, Point{3, 3}, []Point{Point{0, 0}, Point{1, 0}, Point{0, 1}, Point{0, 1}, Point{1, 0}}, nil, nil},
}
