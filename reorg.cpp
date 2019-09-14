#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "utils.h"
#include <chrono>
#include <map>

// Variables:
unsigned long person_length, knows_length, interest_length;
Person *person_map;
unsigned int *knows_map;
unsigned short *interest_map;

void filter_person_location(FILE *knows_out, FILE *person_out)
{
	unsigned int i, max_i;
	unsigned int j, max_j;
	unsigned int new_knows_pos = 0;

	max_i = person_length / sizeof(Person);
	Person *p, *k;
	Person *new_p = new Person();

	for (i = 0; i < max_i; i++)
	{
		p = &person_map[i];
		max_j = p->knows_first + p->knows_n;

		// keep current pointer, to check later if friends remaining with same location.
		unsigned int start_new_knows_pos = new_knows_pos;
		// iterate over all friendships
		for (j = p->knows_first; j < max_j; j++)
		{
			unsigned int offset = knows_map[j];
			Person *new_f = &person_map[offset];
			// if location or friend is equal, write to output file.
			if (new_f->location == p->location)
			{
				fwrite(&offset, sizeof(unsigned int), 1, knows_out);
				new_knows_pos++;
			}
		}

		// Write Binary Version:
		new_p->person_id = (unsigned long)p->person_id;
		new_p->birthday = (unsigned short)p->birthday;
		new_p->location = (unsigned short)p->location;
		new_p->knows_first = (unsigned long)start_new_knows_pos;
		new_p->knows_n = (unsigned short)(new_knows_pos - start_new_knows_pos);
		new_p->interests_first = (unsigned long)p->interests_first;
		new_p->interest_n = (unsigned short)p->interest_n;

		// write binary person record to file
		fwrite(new_p, sizeof(Person), 1, person_out);
	}
}

void filter_person_no_friends(FILE *knows_out, FILE *person_out) {
	unsigned int i, max_i;
	unsigned int j, max_j;
	unsigned int new_i = 0;
	unsigned int new_knows_pos = 0;
	max_i = person_length / sizeof(Person);
	std::map<int, unsigned int> keeper;
	Person *p, *k;
	Person *new_p = new Person();
	
	// determine all people to keep.
	for (i = 0; i < max_i; i++) {
		p = &person_map[i];
		if (p->knows_n == 0) continue;
		keeper[i] = new_i;
		new_i++;
	}

	for (i = 0; i < max_i; i++)
	{
		if (keeper.find(i) == keeper.end()) continue;

		p = &person_map[i];
		max_j = p->knows_first + p->knows_n;

		unsigned int start_new_knows_pos = new_knows_pos;
		// iterate over all friendships
		for (j = p->knows_first; j < max_j; j++)
		{
			unsigned int offset = knows_map[j];
			if (keeper.find(offset) == keeper.end()) continue;
			unsigned int newOff= keeper[offset];
			fwrite(&newOff, sizeof(unsigned int), 1, knows_out);
			new_knows_pos++;
		}

		// Write Binary Version:
		new_p->person_id = (unsigned long)p->person_id;
		new_p->birthday = (unsigned short)p->birthday;
		new_p->location = (unsigned short)p->location;
		new_p->knows_first = (unsigned long)start_new_knows_pos;
		new_p->knows_n = (unsigned short)(new_knows_pos - start_new_knows_pos);
		new_p->interests_first = (unsigned long)p->interests_first;
		new_p->interest_n = (unsigned short)p->interest_n;

		// write binary person record to file
		fwrite(new_p, sizeof(Person), 1, person_out);
	}

}

void filter_mutual_friends(FILE *knows_out, FILE *person_out, FILE *interest_out) {

	unsigned int i, j, y;
	unsigned int max_i = person_length / sizeof(Person);;
	unsigned int max_j;
	unsigned int max_y;
	unsigned int new_knows_pos = 0;
	unsigned long new_interest_pos = 0;
	Person *p, *k;
	Person *new_p = new Person();

	// determine all people to keep.
	for (i = 0; i < max_i; i++)
	{
		p = &person_map[i];
		max_j = p->knows_first + p->knows_n;

		unsigned long start_new_knows_pos = new_knows_pos;
		unsigned long start_new_interest_pos = new_interest_pos;
		// iterate over all friendships
		for (j = p->knows_first; j < max_j; j++)
		{
			unsigned int offset = knows_map[j];
			k = &person_map[offset];
			max_y = k->knows_first + k->knows_n;

			// check mutual friendship:
			bool is_mutual = false;
			for (y = k->knows_first; y < max_y; y++) {
				unsigned int offset_to_p = knows_map[y];
				if (offset_to_p != i) continue;
				is_mutual = true;
				break;
			}

			if (is_mutual == true) {
				fwrite(&offset, sizeof(unsigned int), 1, knows_out);
				new_knows_pos++;
			}
		}

		// rewrite the whole interests (as we will have superfluous ones)
		for (j = p->interests_first; j < p->interests_first + p->interest_n; j++)
		{
			unsigned short interest = interest_map[j];
			fwrite(&interest, sizeof(unsigned short), 1, interest_out);
			new_interest_pos++;
		}

		// Write Binary Version:
		new_p->person_id = (unsigned long)p->person_id;
		new_p->birthday = (unsigned short)p->birthday;
		new_p->location = (unsigned short)p->location;
		new_p->knows_first = (unsigned long)start_new_knows_pos;
		new_p->knows_n = (unsigned short)(new_knows_pos - start_new_knows_pos);
		new_p->interests_first = (unsigned long)start_new_interest_pos;
		new_p->interest_n = (unsigned short)(new_interest_pos - start_new_interest_pos);

		// write binary person record to file
		fwrite(new_p, sizeof(Person), 1, person_out);
	}

}

int main(int argc, char *argv[])
{

	fprintf(stdout, "Starting reorg\n");
	auto t1 = std::chrono::high_resolution_clock::now();

	char *folder = argv[1];
	char *person_output_file = makepath(folder, (char *)"person", (char *)"bin");
	char *interest_output_file = makepath(folder, (char *)"interest", (char *)"bin");
	char *knows_output_file = makepath(folder, (char *)"knows", (char *)"bin");
	// MMAP source files
	person_map = (Person *)mmapr(person_output_file, &person_length);
	knows_map = (unsigned int *)mmapr(knows_output_file, &knows_length);
	interest_map = (unsigned short *)mmapr(interest_output_file, &interest_length);

	// STEP 01: Filter for locality
	char *person_location_output_file = makepath(folder, (char *)"location_person", (char *)"bin");
	char *knows_location_output_file = makepath(folder, (char *)"location_knows", (char *)"bin");
	FILE *knows_out = open_binout(knows_location_output_file);
	FILE *person_out = open_binout(person_location_output_file);;
	filter_person_location(knows_out, person_out);
	fclose(person_out);
	fclose(knows_out);

	// Unmap previous person_map and remap to processed one:
	printf("%d \n", knows_length);
	munmap(person_map, person_length);
	munmap(knows_map, knows_length);
	person_map = (Person *)mmapr(person_location_output_file, &person_length);
	knows_map = (unsigned int *)mmapr(knows_location_output_file, &knows_length);

	printf("%d \n", knows_length);
	// STEP 02: Filter for no friends (as they will not be considerd - only mutal friends)
	char *person_location_friends_output_file = makepath(folder, (char *)"location_friends_person", (char *)"bin");
	char *knows_location_friends_output_file = makepath(folder, (char *)"location_friends_knows", (char *)"bin");
	person_out = open_binout(person_location_friends_output_file);
	knows_out = open_binout(knows_location_friends_output_file);;
	filter_person_no_friends(knows_out, person_out);
	fclose(person_out);
	fclose(knows_out);

	// Unmap previous person_map and remap to processed one:
	munmap(person_map, person_length);
	munmap(knows_map, knows_length);
	person_map = (Person *)mmapr(person_location_friends_output_file, &person_length);
	knows_map = (unsigned int *)mmapr(knows_location_friends_output_file, &knows_length);

	// STEP 03: Filter for mutual friends only:
	char *person_location_friends_mutual_output_file = makepath(folder, (char *)"location_friends_mutual_person", (char *)"bin");
	char *knows_location_friends_mutual_output_file = makepath(folder, (char *)"location_friends_mutual_knows", (char *)"bin");
	char *interest_location_friends_mutual_output_file = makepath(folder, (char *)"location_friends_mutual_interest", (char *)"bin");
	person_out = open_binout(person_location_friends_mutual_output_file);
	knows_out = open_binout(knows_location_friends_mutual_output_file);;
	FILE *interest_out = open_binout(interest_location_friends_mutual_output_file);;
	filter_mutual_friends(knows_out, person_out, interest_out);
	fclose(person_out);
	fclose(knows_out);
	fclose(interest_out);

	printf("Starting reorg \n");
	//take time:
	auto t2 = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

	printf("Execution took microseconds: %d \n", duration);
	printf("Execution took seconds: %d \n", duration / 1000000);
	printf("Execution took minutes : %d \n", duration / 1000000 / 60);
	return 0;
}
