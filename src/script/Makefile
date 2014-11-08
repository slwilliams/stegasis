all: bar.o
	g++ -m64 -std=c++11 -g -I ../ extract_lsb_plane.cc bar.o -o extractlsb.o

bar.o: ../common/progress_bar.cc ../common/progress_bar.h
					g++ -O3 -m64 -std=c++11 -g -I . -c ../common/progress_bar.cc -o bar.o $(CLFAGS)
