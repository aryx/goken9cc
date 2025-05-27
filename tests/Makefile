
#TODO: hello_draw compile but does not run correctly
PROGS=hello_go hello_unicode hello_goroutine hello_web hello_draw

all: $(PROGS)

%: %.6
	6l -o $@ $^

%.6: %.go
	6g $^

clean:
	rm -f $(PROGS)
	rm -f *.6
