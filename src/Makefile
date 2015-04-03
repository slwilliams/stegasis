LIB = -L ../lib/crypto/ -L ../lib/libjpeg/ -L ../lib/gflags/ -static -L/usr/local/lib -lgflags -lcryptopp -ljpeg -ldl -static -lm
#LIB = -L ../lib/crypto/ -L ../lib/libjpeg/ -L ../lib/gflags/ -pthread -static -L/usr/local/lib -lavdevice -lavfilter -lpostproc -lavformat -lavcodec -lxvidcore -lx264 -lpthread -ldl -lvorbisenc -lvorbis -ltheoraenc -ltheoradec -logg -lopencore-amrwb -lopencore-amrnb -lmp3lame -lgsm -lfaac -lbz2 -lz -lswresample -lswscale -lavutil -lm -lgflags -lcryptopp -ljpeg
INCLUDE = -I . -I ../lib
CFLAGS = -DFUSE_USE_VERSION=26 -ldl `pkg-config fuse --cflags` $(LIB)
LINKFLAGS = -ldl `pkg-config fuse --libs` $(LIB)

all: main

clean:
	        rm -rf obj && rm stegasis

main: obj obj/fswrapper.o obj/bar.o obj/main.o
	        g++ -g -O3 -m64 -o stegasis obj/* $(LINKFLAGS) 

obj:
	        mkdir -p obj

obj/fswrapper.o: obj/stegfs.o fs/fswrapper.cc fs/fswrapper.h
	        g++ -g -O3 -m64 -std=c++11 $(INCLUDE) -c fs/fswrapper.cc -o obj/fswrapper.o $(CFLAGS) 

obj/stegfs.o: obj/logging.o  obj/steg.o fs/stegfs.cc fs/stegfs.h
	        g++ -g -O3 -m64 -std=c++11 $(INCLUDE) -c fs/stegfs.cc -o obj/stegfs.o $(CFLAGS) 

obj/steg.o: steg/*.cc steg/*.h
					g++ -g -O3 -m64 -std=c++11 $(INCLUDE) -c steg/lcg.cc -o obj/steg.o $(CLFAGS)

obj/bar.o: common/progress_bar.cc common/progress_bar.h
					g++ -g -O3 -m64 -std=c++11 $(INCLUDE) -c common/progress_bar.cc -o obj/bar.o $(CLFAGS)

obj/video.o: video/other_decoder.cc
					g++ -g -O3 -m64 -std=c++11 $(INCLUDE) -c video/other_decoder.cc -o obj/video.o $(CLFAGS)
					
obj/logging.o: common/logging.cc common/logging.h
	        g++ -g -O3 -m64 -std=c++11 $(INCLUDE) -c common/logging.cc -o obj/logging.o $(CFLAGS) 

obj/main.o: main.cc
					g++ -g -O3 -m64 -std=c++11 $(INCLUDE) -c main.cc -o obj/main.o $(CFLAGS)




