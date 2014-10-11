CFLAGS = -DFUSE_USE_VERSION=26 `pkg-config fuse --cflags`
LINKFLAGS = `pkg-config fuse --libs`

all: main

clean:
	        rm -rf obj

main: obj obj/fswrapper.o obj/main.o
	        g++ -g -o main.o obj/* $(LINKFLAGS) 

obj:
	        mkdir -p obj

obj/fswrapper.o: obj/stegfs.o fs/fswrapper.cc fs/fswrapper.h
	        g++ -g -I ~/Dropbox/stegasis/src/ -c fs/fswrapper.cc -o obj/fswrapper.o $(CFLAGS) 

obj/stegfs.o: obj/logging.o fs/stegfs.cc fs/stegfs.h
	        g++ -g -I ~/Dropbox/stegasis/src/ -c fs/stegfs.cc -o obj/stegfs.o $(CFLAGS) 

obj/logging.o: common/logging.cc common/logging.h
	        g++ -g -I ~/Dropbox/stegasis/src/ -c common/logging.cc -o obj/logging.o $(CFLAGS) 

obj/main.o: main.cc 
	        gcc -g -I ~/Dropbox/stegasis/src/ -c main.cc -o obj/main.o $(CFLAGS) 




