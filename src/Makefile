CFLAGS = -L ../lib/crypto/ -L ../lib/libjpeg/ -lcryptopp -ljpeg -DFUSE_USE_VERSION=26 `pkg-config fuse --cflags` 
LINKFLAGS = -L ../lib/crypto/ -L ../lib/libjpeg/ -lcryptopp -ljpeg `pkg-config fuse --libs`

all: main

clean:
	        rm -rf obj

main: obj obj/fswrapper.o obj/bar.o obj/main.o
	        g++ -g -O3 -m64 -g -o stegasis.o obj/* $(LINKFLAGS) 

obj:
	        mkdir -p obj

obj/fswrapper.o: obj/stegfs.o fs/fswrapper.cc fs/fswrapper.h
	        g++ -g -O3 -m64 -std=c++11 -g -I . -I ../lib -c fs/fswrapper.cc -o obj/fswrapper.o $(CFLAGS) 

obj/stegfs.o: obj/logging.o  obj/steg.o fs/stegfs.cc fs/stegfs.h
	        g++ -g -O3 -m64 -std=c++11 -g -I . -I ../lib -c fs/stegfs.cc -o obj/stegfs.o $(CFLAGS) 

obj/steg.o: steg/*.cc steg/*.h
					g++ -g -O3 -m64 -std=c++11 -g -I . -I ../lib -c steg/lcg.cc -o obj/steg.o $(CLFAGS)

obj/bar.o: common/progress_bar.cc common/progress_bar.h
					g++ -g -O3 -m64 -std=c++11 -g -I . -I ../lib -c common/progress_bar.cc -o obj/bar.o $(CLFAGS)

obj/video.o: video/other_decoder.cc
					g++ -g -O3 -m64 -std=c++11 -g -I . -I ../lib -c video/other_decoder.cc -o obj/video.o $(CLFAGS)
					
obj/logging.o: common/logging.cc common/logging.h
	        g++ -g -O3 -m64 -std=c++11 -g -I . -I ../lib -c common/logging.cc -o obj/logging.o $(CFLAGS) 

obj/main.o: main.cc
					g++ -g -O3 -m64 -std=c++11 -g -I . -I ../lib -c main.cc -o obj/main.o $(CFLAGS)




