# remove the # in the following line to enable reorg compilation (and running)
all: cruncher

cruncher: cruncher.cpp reorg.cpp utils.h
	g++ -std=c++11 -I. -O3 -o reorg reorg.cpp
	g++ -I. -std=c++11 -O3 -o cruncher cruncher.cpp

loader: loader.c utils.h
	gcc -I. -O3 -o loader loader.c 

clean:
	rm -f loader cruncher reorg