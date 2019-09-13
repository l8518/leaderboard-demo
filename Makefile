# remove the # in the following line to enable reorg compilation (and running)
all: cruncher reorg

cruncher: cruncher.cpp utils.h
	g++ -I. -std=c++11 -O3 -o cruncher cruncher.cpp

cruncher-profile: cruncher.cpp utils.h
	g++ -pg -I. -O3 -o cruncher-profile cruncher.cpp

cruncher-profile-wo: cruncher.cpp utils.h
	g++ -pg -I. -O0 -o cruncher-profile-wo cruncher.cpp

loader: loader.c utils.h
	gcc -I. -O3 -o loader loader.c 

reorg: reorg.c utils.h
	g++ -I. -O3 -o reorg reorg.c

clean:
	rm -f loader cruncher reorg