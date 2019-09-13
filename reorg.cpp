#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "utils.h"
// Type Defs:
typedef struct {
	unsigned short location;
} LocationBin;

// Variables:
unsigned long person_length, knows_length, interest_length, person_location_length;
Person *person_map;
LocationBin *location_map;
FILE *outfile;



/**
 * Prepare a bin file, that only contains the location and the person offset (implicitly)
 * */
void prepare_person_location_bin() {

	unsigned int i, max_i;
	max_i = person_length / sizeof(Person);
	Person *p;
	LocationBin* results = (LocationBin*)malloc(max_i * sizeof (LocationBin));

	for (i = 0; i < max_i; i++)
		results[i].location = (&person_map[i])->location;

	outfile = fopen("person_location.bin", "w");
	// output
	for (i = 0; i < max_i; i++)
		fprintf(outfile, "%hu\n", results[i].location);

	return;
}

int main(int argc, char *argv[]) {
	char* person_output_file   = makepath((char*)argv[1], (char*)"person",   (char*)"bin");
	char* interest_output_file = makepath((char*)argv[1], (char*)"interest", (char*)"bin");
	char* knows_output_file    = makepath((char*)argv[1], (char*)"knows",    (char*)"bin");
	
	person_map = (Person *)mmapr(person_output_file, &person_length);
	// this does not do anything yet. But it could...
	prepare_person_location_bin();
	location_map = (LocationBin *)mmapr((char*)"person_location.bin", &person_location_length);
	return 0;
}

