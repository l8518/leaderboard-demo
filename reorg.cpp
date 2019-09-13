#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "utils.h"
// Type Defs:
// typedef struct {
// 	unsigned short location;
// } LocationBin;

// Variables:
unsigned long person_length, knows_length, interest_length, person_location_length;

Person *person_map;
unsigned int *knows_map;
// LocationBin *location_map;
FILE *outfile;



/**
 * Prepare a bin file, that only contains the location and the person offset (implicitly)
 * */
unsigned short * prepare_person_location_bin() {

	unsigned int i, max_i;
	max_i = person_length / sizeof(Person);
	Person *p;
	// LocationBin* results = (LocationBin*)malloc(max_i * sizeof (LocationBin));
	// unsigned short location_arr[max_i];

	unsigned short* location_arr = new unsigned short[max_i];

	for (i = 0; i < max_i; i++)
		location_arr[i] = (&person_map[i])->location;

	// outfile = fopen("person_location.bin", "w");
	// 	for (i = 0; i < max_i; i++)
	// // 	fprintf(outfile, "%hu\n", results[i].location);

	return location_arr;
}

void filter_person_location(unsigned short* location_arr, char * folder) {

	// FILE *knows_outfile = fopen("location_knows.raw", "w");
	// FILE *person_location_outfile = fopen("location_person.raw", "w");
	FILE   *knows_out;
	FILE   *person_out;
	
	knows_out = open_binout((char*)"location_knows.bin");
	person_out = open_binout((char*)"location_person.bin");

	unsigned int i, max_i;
	unsigned int j, max_j;
	unsigned int new_knows_pos = 0;

	max_i = person_length / sizeof(Person);
	Person *p, *k;
	int total = 0;


	// goal: create a new knows file, that only contains people
	// with the same location
	// consequences:
	// --> write new person.bin with new knows_pos and new knows_n

	for (i = 0; i < max_i; i++) {
		p = &person_map[i];
		max_j = p->knows_first + p->knows_n;
		

		// keep current pointer, to check later if friends remaining with same location.
		unsigned int start_new_knows_pos = new_knows_pos;
		// iterate over all friendships
		for (j = p->knows_first; j < max_j; j++) {
			unsigned int offset = knows_map[j];
			Person *new_f = &person_map[offset];
			// if location or friend is equal, write to output file.
			if (new_f->location == p->location) {
				// fprintf(knows_outfile, "%u\n", offset); //TODO: DEBUG ONLY
				// Write Binary Version:
				fwrite(&offset, sizeof(unsigned int), 1, knows_out);

				// Increase Counter
				new_knows_pos++;
				total++;
			}
		}

		if (true) {
			// Write Binary Version:
			Person *new_p = new Person();
			new_p->person_id = (unsigned long)p->person_id;
			new_p->birthday =  (unsigned short)p->birthday;
			new_p->location =  (unsigned short)p->location;
			new_p->knows_first = (unsigned long)start_new_knows_pos;
			new_p->knows_n = (unsigned short) (new_knows_pos - start_new_knows_pos);
			new_p->interests_first = (unsigned long)p->interests_first;
			new_p->interest_n = (unsigned short)p->interest_n;
			
			// write binary person record to file
			fwrite(new_p, sizeof(Person), 1, person_out);
			// write person:
			// fprintf(person_location_outfile, "%lu|%hu|%hu|%u|%hu|%lu|%hu\n",
			//  new_p->person_id, new_p->birthday, new_p->location, new_p->knows_first, new_p->knows_n,
			//  new_p->interests_first, new_p->interest_n
			//  ); //TODO: DEBUG ONLY
		}
	}
	printf("total person: %d \n", max_i);
	printf("total: %d \n", total);
}

int main(int argc, char *argv[]) {

	char* folder = argv[1];
	char* person_output_file   = makepath((char*)argv[1], (char*)"person",   (char*)"bin");
	char* interest_output_file = makepath((char*)argv[1], (char*)"interest", (char*)"bin");
	char* knows_output_file    = makepath((char*)argv[1], (char*)"knows",    (char*)"bin");
	
	person_map = (Person *)mmapr(person_output_file, &person_length);
	knows_map = (unsigned int *)mmapr(knows_output_file, &knows_length);
	// this does not do anything yet. But it could...
	unsigned short* location_arr = prepare_person_location_bin();
	filter_person_location(location_arr, folder);
	// location_map = (LocationBin *)mmapr((char*)"person_location.bin", &person_location_length);
	return 0;
}

