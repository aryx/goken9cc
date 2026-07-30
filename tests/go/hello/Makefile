
PROGS=hello_go.exe hello_unicode.exe hello_goroutine.exe hello_web.exe hello_draw.exe

all: $(PROGS)

%.exe: %.6
	6l -o $@ $^

%.6: %.go
	6g $^

clean:
	rm -f $(PROGS)
	rm -f *.6
