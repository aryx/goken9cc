package stdio

/*
#include <stdio.h>

typedef unsigned char Uint8;
typedef unsigned short Uint16;

typedef enum {
 MOD1 = 0x0000,
 MODX = 0x8000
} SDLMod;

typedef enum {
 A = 1,
 B = 322,
 SDLK_LAST
} SDLKey;

typedef struct SDL_keysym {
	Uint8 scancode;
	SDLKey sym;
	SDLMod mod;
	Uint16 unicode;
} SDL_keysym;

typedef struct SDL_KeyboardEvent {
	Uint8 typ;
	Uint8 which;
	Uint8 state;
	SDL_keysym keysym;
} SDL_KeyboardEvent;

void makeEvent(SDL_KeyboardEvent *event) {
 unsigned char *p;
 int i;

 p = (unsigned char*)event;
 for (i=0; i<sizeof *event; i++) {
   p[i] = i;
 }
}

int same(SDL_KeyboardEvent* e, Uint8 typ, Uint8 which, Uint8 state, Uint8 scan, SDLKey sym, SDLMod mod, Uint16 uni) {
  return e->typ == typ && e->which == which && e->state == state && e->keysym.scancode == scan && e->keysym.sym == sym && e->keysym.mod == mod && e->keysym.unicode == uni;
}

void cTest(SDL_KeyboardEvent *event) {
 printf("C: %#x %#x %#x %#x %#x %#x %#x\n", event->typ, event->which, event->state,
   event->keysym.scancode, event->keysym.sym, event->keysym.mod, event->keysym.unicode);
 fflush(stdout);
}

*/
import "C"

import (
	"fmt"
	"syscall"
)

func TestAlign() {
	if syscall.ARCH == "amd64" {
		// alignment is known to be broken on amd64.
		// http://code.google.com/p/go/issues/detail?id=609
		return
	}
	var evt C.SDL_KeyboardEvent
	C.makeEvent(&evt)
	if C.same(&evt, evt.typ, evt.which, evt.state, evt.keysym.scancode, evt.keysym.sym, evt.keysym.mod, evt.keysym.unicode) == 0 {
		fmt.Println("*** bad alignment")
		C.cTest(&evt)
		fmt.Printf("Go: %#x %#x %#x %#x %#x %#x %#x\n",
			evt.typ, evt.which, evt.state, evt.keysym.scancode,
			evt.keysym.sym, evt.keysym.mod, evt.keysym.unicode)
		fmt.Println(evt)
	}
}
