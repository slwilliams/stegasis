CFLAGS = -DFUSE_USE_VERSION=26 `pkg-config fuse --cflags`
LINKFLAGS = `pkg-config fuse --libs`

all: main

clean:
	        rm -rf obj

main: obj/fswrapper.o obj/main.o
	        g++ -g -o main.o obj/* $(LINKFLAGS) 

obj:
	        mkdir -p obj

obj/fswrapper.o: fs/fswrapper.cc fs/fswrapper.h
	        g++ -g -I ~/Dropbox/stegasis/src/ -c fs/fswrapper.cc -o obj/fswrapper.o $(CFLAGS) 

obj/main.o: main.cc fs/fswrapper.h
	        gcc -g -I ~/Dropbox/stegasis/src/ -c main.cc -o obj/main.o $(CFLAGS) 


