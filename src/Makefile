CFLAGS = -L ../lib/crypto/ -lcryptopp -DFUSE_USE_VERSION=26 `pkg-config fuse --cflags` 
LINKFLAGS = -L ../lib/crypto/ -lcryptopp `pkg-config fuse --libs`

all: main

clean:
	        rm -rf obj

main: obj obj/fswrapper.o obj/bar.o obj/main.o
	        g++ -O3 -m64 -g -o stegasis.o obj/* $(LINKFLAGS) 

obj:
	        mkdir -p obj

obj/fswrapper.o: obj/stegfs.o fs/fswrapper.cc fs/fswrapper.h
	        g++ -O3 -m64 -std=c++11 -g -I . -I ../lib/crypto -c fs/fswrapper.cc -o obj/fswrapper.o $(CFLAGS) 

obj/stegfs.o: obj/logging.o  obj/steg.o fs/stegfs.cc fs/stegfs.h
	        g++ -O3 -m64 -std=c++11 -g -I . -I ../lib/crypto -c fs/stegfs.cc -o obj/stegfs.o $(CFLAGS) 

obj/steg.o: steg/*.cc steg/*.h
					g++ -O3 -m64 -std=c++11 -g -I . -I ../lib/crypto -c steg/lcg.cc -o obj/steg.o $(CLFAGS)

obj/bar.o: common/progress_bar.cc common/progress_bar.h
					g++ -O3 -m64 -std=c++11 -g -I . -I ../lib/crypto -c common/progress_bar.cc -o obj/bar.o $(CLFAGS)
					
obj/logging.o: common/logging.cc common/logging.h
	        g++ -O3 -m64 -std=c++11 -g -I . -I ../lib/crypto -c common/logging.cc -o obj/logging.o $(CFLAGS) 

obj/main.o: main.cc
					g++ -O3 -m64 -std=c++11 -g -I . -I ../lib/crypto -c main.cc -o obj/main.o $(CFLAGS)




