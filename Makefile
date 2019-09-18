# remove the # in the following line to enable reorg compilation (and running)
all: cruncher reorg

cruncher: cruncher.cpp utils.h
	g++ -I. -std=c++11 -O3 -o cruncher cruncher.cpp

reorg: reorg.cpp utils.h
	g++ -std=c++11 -I. -O3 -o reorg reorg.cpp

loader: loader.c utils.h
	gcc -I. -O3 -o loader loader.c 

filedebug: file_debug.cpp utils.h
	g++ -std=c++11 -I. -O3 -o file_debug file_debug.cpp

clean:
	rm -f loader cruncher reorg filedebug
