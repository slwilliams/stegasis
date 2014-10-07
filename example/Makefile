all: example

example: example.c 
	gcc example.c `pkg-config fuse --cflags --libs` -o ../bin/example

clean:
	rm example

