#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "utils.h"
#include <chrono>
#include <map>
#include <algorithm>    // std::max
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>



// Variables:
unsigned long person_length, knows_length, interest_length;
Person *person_map;
CompressedPerson *person_com_map;
unsigned int *knows_map;
unsigned short *interest_map;

bool DEBUG = false;

struct InterestPersonMapping {
	unsigned int poffset = 0;
	unsigned short interest = 0;
} ;

struct InterestPersonMappingIndex {
	unsigned int mapping_first;
	unsigned short mapping_n;
} ;

void filter_person_location(FILE *knows_out, FILE *person_out)
{
	unsigned int i, max_i;
	unsigned int j, max_j;
	unsigned int new_knows_pos = 0;

	max_i = person_length / sizeof(Person);
	Person *p, *k;
	CompressedPerson *new_p = new CompressedPerson();

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
		fwrite(new_p, sizeof(CompressedPerson), 1, person_out);
	}
}

void filter_person_no_friends(FILE *knows_out, FILE *person_out) {
	unsigned int i, max_i;
	unsigned int j, max_j;
	unsigned int new_i = 0;
	unsigned int new_knows_pos = 0;
	max_i = person_length / sizeof(CompressedPerson);
	std::map<int, unsigned int> keeper;
	CompressedPerson *p, *k;
	CompressedPerson *new_p = new CompressedPerson();
	
	// determine all people to keep.
	for (i = 0; i < max_i; i++) {
		p = &person_com_map[i];
		if (p->knows_n == 0) continue;
		keeper[i] = new_i;
		new_i++;
	}

	for (i = 0; i < max_i; i++)
	{
		if (keeper.find(i) == keeper.end()) continue;

		p = &person_com_map[i];
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
		fwrite(new_p, sizeof(CompressedPerson), 1, person_out);
	}

}

void filter_mutual_friends_and_reduce_interests(char *folder, FILE *knows_out, FILE *person_out, FILE *interest_out) {

	printf("Enter filter_mutual_friends_and_reduce_interests \n");
	unsigned int i, j, y;
	unsigned int max_i = person_length / sizeof(CompressedPerson);;
	unsigned int max_j;
	unsigned int max_y;
	unsigned int new_knows_pos = 0;
	unsigned long new_interest_pos = 0;
	CompressedPerson *p, *k;
	CompressedPerson *new_p = new CompressedPerson();

	FILE *debug_person = fopen(makepath(folder, (char *)"debug_person", (char *)"csv"), "w");
	FILE *debug_knows = fopen(makepath(folder, (char *)"debug_knows", (char *)"csv"), "w");
	FILE *debug_interest = fopen(makepath(folder, (char *)"debug_interest", (char *)"csv"), "w");

	for (i = 0; i < max_i; i++)
	{
		p = &person_com_map[i];
		max_j = p->knows_first + p->knows_n;

		unsigned long start_new_knows_pos = new_knows_pos;
		unsigned long start_new_interest_pos = new_interest_pos;
		// iterate over all friendships
		for (j = p->knows_first; j < max_j; j++)
		{
			unsigned int offset = knows_map[j];
			k = &person_com_map[offset];
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
				if(DEBUG) fprintf(debug_knows, "%u,\n", offset );
				new_knows_pos++;
			}
		}

		// rewrite the whole interests (as we will have superfluous ones)
		for (j = p->interests_first; j < p->interests_first + p->interest_n; j++)
		{
			unsigned short interest = interest_map[j];
			fwrite(&interest, sizeof(unsigned short), 1, interest_out);
			if(DEBUG) fprintf(debug_interest, "%hu,\n", interest );
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

		if(DEBUG) fprintf(debug_person, "%lu, %hu, %hu, %lu, %hu, %lu, %hu\n", new_p->person_id, new_p->birthday, new_p->location, new_p->knows_first, new_p->knows_n, new_p->interests_first, new_p->interest_n );

		// write binary person record to file
		fwrite(new_p, sizeof(CompressedPerson), 1, person_out);
	}

	fclose(debug_person);
	fclose(debug_knows);
	fclose(debug_interest);
	printf("Exit filter_mutual_friends_and_reduce_interests \n");
}

void build_inverted_list(char *folder) {
	printf("Enter build_inverted_list\n");
	unsigned int i, j;
	unsigned int max_i =  person_length / sizeof(CompressedPerson);
	CompressedPerson *p;
	InterestPersonMapping *ipm = new InterestPersonMapping();

	// write interest personoffset
	char *ipm_debug_file = makepath(folder, (char *)"interest_person_mapping", (char *)"csv");
	FILE *ipm_debug = fopen(ipm_debug_file, "w");
	
	for (i = 0; i < person_length / sizeof(CompressedPerson); i++) {
		
		p = &person_com_map[i];

		for (j = p->interests_first; j < p->interests_first + p->interest_n; j++)
		{
			unsigned short interest = interest_map[j];
			ipm->interest = interest;
			ipm->poffset = i;
			fprintf(ipm_debug, "%u %hu \n", ipm->poffset, ipm->interest);
		}
	}
	fclose(ipm_debug);

	printf(" \t PersonMapping Sortable written\n");

	//Sort the Outputfile
	char *ipm_sorted = makepath(folder, (char *)"interest_person_mapping_sorted", (char *)"csv");
	std::string str1 = (std::string)makepath(folder, (char *)"interest_person_mapping", (char *)"csv");
	std::string str2 = (std::string)ipm_sorted;
	std::string cmd = "sort -S 512MB --field-separator=' ' --key=2n " + str1 + " > " + str2;
	std::system(cmd.c_str());

	printf(" \t PersonMapping SORTED\n");

	// write interest personoffset
	char *interest_person_mapping = makepath(folder, (char *)"interest_person_mapping", (char *)"bin");
	FILE *ipm_out = open_binout(interest_person_mapping);

	std::ifstream infile(ipm_sorted);
	std::string line;
	
	while (getline(infile, line))
	{
		unsigned int poffset;
		unsigned short interest;

		std::stringstream sl(line);
		sl >> poffset;
		sl >> interest;
		ipm->poffset = poffset;
		ipm->interest = interest;
		fwrite(ipm, sizeof(InterestPersonMapping), 1, ipm_out); // Write Interest Person Mapping
	}
	fclose(ipm_out);

	printf(" \t PersonMapping Binary Written\n");
	
	int max = 0;
	int min = 99999999;
	for (i = 0; i < interest_length / sizeof(unsigned short); i++) {
		unsigned short interest = interest_map[i];
		max = std::max(max, (int)interest);
		min = std::min(min, (int)interest);
	}

	printf(" \t Max Interest Value: %d \n", max);
	printf(" \t Min Interest Value: %d \n", min);

	unsigned long interest_person_mapping_length;
	InterestPersonMapping *interest_person_map;
	interest_person_map = (InterestPersonMapping *)mmapr(interest_person_mapping, &interest_person_mapping_length);

	char *tags_path = makepath(folder, (char *)"tags", (char *)"bin");
	FILE *tag_debug = fopen(makepath(folder, (char *)"debug_tags", (char *)"csv"), "w");
	FILE *tags_out = open_binout(tags_path);
	Tag *new_tag = new Tag();
	unsigned short tag_offset = 0;

	unsigned int current_posting_offset = 0;
	char *postings_path = makepath(folder, (char *)"postings", (char *)"bin");
	FILE *postings_out = open_binout(postings_path);
	FILE *postings_debug = fopen(makepath(folder, (char *)"debug_postings", (char *)"csv"), "w");

	printf(" \t Writing Tag and Postings\n");
	unsigned int ipm_index = 0;
	unsigned int tags_written = 0;
	InterestPersonMapping *cipm = &interest_person_map[ipm_index];
	for (unsigned interest = min; interest <= max; interest++) {
		unsigned int start_posting_offset = current_posting_offset;

		while(ipm_index < interest_person_mapping_length / sizeof(InterestPersonMapping) && cipm->interest == interest) {
			fwrite(&cipm->poffset, sizeof(unsigned int), 1, postings_out);
			if(DEBUG) fprintf(postings_debug, "%u\n", cipm->poffset);
			current_posting_offset++;
			cipm = &interest_person_map[++ipm_index]; // get next ipm
		}

		new_tag->posting_first = start_posting_offset;
		new_tag->posting_n = current_posting_offset - start_posting_offset;
		if(DEBUG) fprintf(tag_debug, "%u, %hu\n", new_tag->posting_first, new_tag->posting_n);
		fwrite(new_tag, sizeof(Tag), 1, tags_out);
		tags_written++;
	}
	fclose(tags_out);
	fclose(postings_out);
	fclose(tag_debug);
	fclose(postings_debug);
	printf(" \t Finished Writing Tag (%d) and Postings(%u)\n", tags_written, current_posting_offset);
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
	person_com_map = (CompressedPerson *)mmapr(person_location_output_file, &person_length);
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

	// Unmap previous person_com_map and remap to processed one:
	munmap(person_com_map, person_length);
	munmap(knows_map, knows_length);
	person_com_map = (CompressedPerson *)mmapr(person_location_friends_output_file, &person_length);
	knows_map = (unsigned int *)mmapr(knows_location_friends_output_file, &knows_length);

	// STEP 03: Filter for mutual friends only and reduce intersts.bin:
	char *person_location_friends_mutual_output_file = makepath(folder, (char *)"location_friends_mutual_person", (char *)"bin");
	char *knows_location_friends_mutual_output_file = makepath(folder, (char *)"location_friends_mutual_knows", (char *)"bin");
	char *interest_location_friends_mutual_output_file = makepath(folder, (char *)"location_friends_mutual_interest", (char *)"bin");
	person_out = open_binout(person_location_friends_mutual_output_file);
	knows_out = open_binout(knows_location_friends_mutual_output_file);;
	FILE *interest_out = open_binout(interest_location_friends_mutual_output_file);;
	filter_mutual_friends_and_reduce_interests(folder, knows_out, person_out, interest_out);
	fclose(person_out);
	fclose(knows_out);
	fclose(interest_out);

	// Remap for counting:
	munmap(person_com_map, person_length);
	munmap(knows_map, knows_length);
	munmap(interest_map, interest_length);
	person_com_map = (CompressedPerson *)mmapr(person_location_friends_mutual_output_file, &person_length);
	knows_map = (unsigned int *)mmapr(knows_location_friends_mutual_output_file, &knows_length);
	interest_map = (unsigned short *)mmapr(interest_location_friends_mutual_output_file, &interest_length);

	// build interest inverted lists:	
	build_inverted_list(folder);

	printf("Finished reorg \n");
	printf("Lines in latest person.bin: \t %u \n", person_length / sizeof(CompressedPerson) );
	printf("Lines in latest knows.bin: \t %u \n", knows_length / sizeof(unsigned int) );
	printf("Lines in latest interest.bin: \t %u \n", interest_length / sizeof(unsigned short) );

	//take time:
	auto t2 = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

	printf("Execution took microseconds: %d \n", duration);
	printf("Execution took seconds: %d \n", duration / 1000000);
	printf("Execution took minutes : %d \n", duration / 1000000 / 60);
	return 0;
}
