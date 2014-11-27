all: bar.o hist
	g++ -m64 -std=c++11 -g -I ../ extract_lsb_plane.cc bar.o -o extractlsb.o

bar.o: ../common/progress_bar.cc ../common/progress_bar.h
					g++ -O3 -m64 -std=c++11 -g -I . -c ../common/progress_bar.cc -o bar.o

hist: bar.o jpeg_histogram.cc
					g++ -O3 -m64 -std=c++11 -g -I ../ -I ../../lib jpeg_histogram.cc bar.o -o main.o -L ../../lib/libjpeg/ -ljpeg

test: bar.o jpeg_test.cc
					g++ -O3 -m64 -std=c++11 -g -I ../ -I ../../lib jpeg_test.cc bar.o -o test.o -L ../../lib/libjpeg/ -ljpeg
