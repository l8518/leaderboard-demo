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
#include <limits>



// Variables:
unsigned long person_length, knows_length, interest_length;
CompressedPerson *person_com_map;
unsigned int *knows_map;
unsigned short *interest_map;

bool DEBUG = false;

struct InterestPersonMapping {
	unsigned int poffset = 0;
	unsigned short interest = 0;
} ;

void filter_person_location(char *folder)
{
	// Map relevant files:
	Person *person_map;
	char *person_output_file = makepath(folder, (char *)"person", (char *)"bin");
	char *knows_output_file = makepath(folder, (char *)"knows", (char *)"bin");
	person_map = (Person *)mmapr(person_output_file, &person_length);
	knows_map = (unsigned int *)mmapr(knows_output_file, &knows_length);
	
	// Define + Open Output Files:
	char *person_location_output_file = makepath(folder, (char *)"location_person", (char *)"bin");
	char *knows_location_output_file = makepath(folder, (char *)"location_knows", (char *)"bin");

	FILE *knows_out = open_binout(knows_location_output_file);
	FILE *person_out = open_binout(person_location_output_file);;

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

	fclose(person_out);
	fclose(knows_out);
	delete new_p;
	// Remap files:
	munmap(person_map, person_length);
	munmap(knows_map, knows_length);
	person_com_map = (CompressedPerson *)mmapr(person_location_output_file, &person_length);
	knows_map = (unsigned int *)mmapr(knows_location_output_file, &knows_length);
}

void filter_mutual_friends_and_reduce_interests(char *folder) {

	// Map files
	char *interest_output_file = makepath(folder, (char *)"interest", (char *)"bin");
	interest_map = (unsigned short *)mmapr(interest_output_file, &interest_length);

	// Define Output files
	char *person_location_friends_mutual_output_file = makepath(folder, (char *)"location_friends_mutual_person", (char *)"bin");
	char *knows_location_friends_mutual_output_file = makepath(folder, (char *)"location_friends_mutual_knows", (char *)"bin");
	char *interest_location_friends_mutual_output_file = makepath(folder, (char *)"location_friends_mutual_interest", (char *)"bin");
	FILE *person_out = open_binout(person_location_friends_mutual_output_file);
	FILE *knows_out = open_binout(knows_location_friends_mutual_output_file);;
	FILE *interest_out = open_binout(interest_location_friends_mutual_output_file);;

	printf("Enter filter_mutual_friends_and_reduce_interests \n");
	unsigned int i, j, y;
	unsigned int new_knows_pos = 0;
	unsigned long new_interest_pos = 0;
	unsigned int new_i = 0;
	unsigned int* offset_map = NULL;
	CompressedPerson *p, *k;
	CompressedPerson *new_p = new CompressedPerson();


	offset_map = new unsigned int[person_length / sizeof(CompressedPerson)];

	// Calculate the mapping of current person.bin offsets and the new offsets if person removed;
	for (i = 0; i < person_length / sizeof(CompressedPerson); i++)
		offset_map[i] = (0 == (&person_com_map[i])->knows_n) ? new_i : new_i++;;

	for (i = 0; i < person_length / sizeof(CompressedPerson); i++)
	{
		p = &person_com_map[i];
		if (p->knows_n == 0) continue;

		unsigned long start_new_knows_pos = new_knows_pos;
		unsigned long start_new_interest_pos = new_interest_pos;
		// iterate over all friendships
		for (j = p->knows_first; j < p->knows_first + p->knows_n; j++)
		{
			unsigned int offset = knows_map[j];

			k = &person_com_map[offset];

			// check mutual friendship:
			bool is_mutual = false;
			for (y = k->knows_first; y < k->knows_first + k->knows_n; y++) {
				unsigned int offset_to_p = knows_map[y];
				if (offset_to_p != i) continue;
				is_mutual = true;
				break;
			}

			if (is_mutual == true) {
				unsigned int newOff= offset_map[offset];
				fwrite(&newOff, sizeof(unsigned int), 1, knows_out);
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
		fwrite(new_p, sizeof(CompressedPerson), 1, person_out);
	}

	delete offset_map;
	delete new_p;

	printf("Exit filter_mutual_friends_and_reduce_interests \n");

	fclose(person_out);
	fclose(knows_out);
	fclose(interest_out);

	munmap(person_com_map, person_length);
	munmap(knows_map, knows_length);
	munmap(interest_map, interest_length);
	person_com_map = (CompressedPerson *)mmapr(person_location_friends_mutual_output_file, &person_length);
	knows_map = (unsigned int *)mmapr(knows_location_friends_mutual_output_file, &knows_length);
	interest_map = (unsigned short *)mmapr(interest_location_friends_mutual_output_file, &interest_length);
}

int ipm_comparator(const void *v1, const void *v2)
{
	InterestPersonMapping *r1 = (InterestPersonMapping *)v1;
	InterestPersonMapping *r2 = (InterestPersonMapping *)v2;
	if (r1->interest > r2->interest)
		return +1;
	else if (r1->interest < r2->interest)
		return -1;
	else if (r1->poffset < r2->poffset)
		return +1;
	else if (r1->poffset > r2->poffset)
		return -1;
	else
		return 0;
}

struct PersonBirthdayMapping {
	unsigned long  person_id;
	unsigned long  knows_first;
	unsigned long  interests_first;
	unsigned short knows_n;
	unsigned short interest_n;
	unsigned short birthday;
	unsigned int old_offset;
} ;

int pbm_comparator(const void *v1, const void *v2)
{
	PersonBirthdayMapping *r1 = (PersonBirthdayMapping *)v1;
	PersonBirthdayMapping *r2 = (PersonBirthdayMapping *)v2;
	if (r1->birthday > r2->birthday)
		return +1;
	else if (r1->birthday < r2->birthday)
		return -1;
	else if (r1->old_offset > r2->old_offset)
		return +1;
	else if (r1->old_offset < r2->old_offset)
		return -1;
	else
		return 0;
}

void sort_person(char *folder) {
	printf("Enter sort_person\n");
	// Sort Person After Birthday And Update Knows.bin
	PersonBirthdayMapping* person_birthday_mapping = NULL;
	person_birthday_mapping = new PersonBirthdayMapping[person_length / sizeof(CompressedPerson)];
	// Current Order - whatever it is!
	for (unsigned int i = 0; i < person_length / sizeof(CompressedPerson); i++) {
	 	CompressedPerson *p = &person_com_map[i];
		person_birthday_mapping[i].person_id = p->person_id;
		person_birthday_mapping[i].knows_first = p->knows_first;
		person_birthday_mapping[i].interests_first = p->interests_first;
		person_birthday_mapping[i].birthday = p->birthday;
		person_birthday_mapping[i].knows_n = p->knows_n;
		person_birthday_mapping[i].interest_n = p->interest_n;
		person_birthday_mapping[i].birthday = p->birthday;
		person_birthday_mapping[i].old_offset = (unsigned int)i;
	}

	qsort(person_birthday_mapping, person_length / sizeof(CompressedPerson), sizeof(PersonBirthdayMapping), &pbm_comparator);

	unsigned int* offset_map = NULL;
	offset_map = new unsigned int[person_length / sizeof(CompressedPerson)];
	for (unsigned int i = 0; i < person_length / sizeof(CompressedPerson); i++) {
		offset_map[person_birthday_mapping[i].old_offset] = i;
	}

	// Define Output files
	char *person_file = makepath(folder, (char *)"person_bsort", (char *)"bin");
	char *knows_file = makepath(folder, (char *)"knows_bsort", (char *)"bin");
	FILE *person_out = open_binout(person_file);
	FILE *knows_out = open_binout(knows_file);

	unsigned int new_knows_pos = 0;
	PersonBirthdayMapping *p;
	CompressedPerson *new_p = new CompressedPerson();
	for (unsigned int i = 0; i < person_length / sizeof(CompressedPerson); i++)
	{
		p = &person_birthday_mapping[i];
		// TODO Write Birthday Index

		// iterate over all friendships
		unsigned long start_new_knows_pos = new_knows_pos;

		for (unsigned int j = p->knows_first; j < p->knows_first + p->knows_n; j++)
		{
			unsigned int offset = offset_map[knows_map[j]];
			fwrite(&offset, sizeof(unsigned int), 1, knows_out);
			new_knows_pos++;
		}

		// Write Binary Version:
		new_p->person_id = p->person_id;
		new_p->birthday = p->birthday;
		new_p->knows_first = start_new_knows_pos;
		new_p->knows_n = (new_knows_pos - start_new_knows_pos);
		new_p->interests_first = p->interests_first;
		new_p->interest_n = p->interest_n;

		// write binary person record to file
		fwrite(new_p, sizeof(CompressedPerson), 1, person_out);
	}

	// free memory:
	delete new_p;
	delete offset_map;
	delete person_birthday_mapping;

	fclose(person_out);
	fclose(knows_out);

	munmap(person_com_map, person_length);
	munmap(knows_map, knows_length);
	person_com_map = (CompressedPerson *)mmapr(person_file, &person_length);
	knows_map = (unsigned int *)mmapr(knows_file, &knows_length);

	printf("Exit sort_person\n");
}

void build_inverted_list(char *folder) {
	printf("Enter build_inverted_list\n");
	unsigned int i, j;
	unsigned int max_i =  person_length / sizeof(CompressedPerson);
	CompressedPerson *p;
	InterestPersonMapping *ipm = new InterestPersonMapping();

	/**
	 * Create a Mapping between the Interest and the Person
	 * Afterwards sort that mapping by interest, so we can use it
	 * to create the bin file that is sorted by interest.
	 */
	unsigned int ipm_index = 0;
	InterestPersonMapping* arr = NULL;
	arr = new InterestPersonMapping[interest_length / sizeof(unsigned short)];
	for (i = 0; i < person_length / sizeof(CompressedPerson); i++) {
	 	p = &person_com_map[i];
	 	for (j = p->interests_first; j < p->interests_first + p->interest_n; j++)
	 	{
	 		arr[ipm_index].interest = interest_map[j];
	 		arr[ipm_index].poffset = i;
	 		ipm_index++;
	 	}
	}

	qsort(arr, ipm_index, sizeof(InterestPersonMapping), &ipm_comparator);

	/**
	 * Write the binary file to rmap later and determine min and max values
	 * for the interests. If gaps occur, we can consider that for tine inverted
	 * list, so that the offset == interest.
	 * */
	char *interest_person_mapping = makepath(folder, (char *)"interest_person_mapping", (char *)"bin");
	FILE *ipm_out = open_binout(interest_person_mapping);
	
	unsigned short max = std::numeric_limits<unsigned short>::min();;
	unsigned short min = std::numeric_limits<unsigned short>::max();;
	for (int i = 0; i < ipm_index; i++) {
		unsigned short interest = arr[i].interest;
		ipm->poffset = arr[i].poffset;
		ipm->interest = interest;
		max = std::max(max, (unsigned short)interest);
		min = std::min(min, (unsigned short)interest);
		fwrite(ipm, sizeof(InterestPersonMapping), 1, ipm_out); // Write Interest Person Mapping
	}
	fclose(ipm_out);
	delete arr;
	delete ipm;

	/**
	 * Write Tags and Postings, based on the PersonInterestMapping
	 * */
	printf(" \t Writing Tag and Postings\n");
	unsigned long interest_person_mapping_length;
	InterestPersonMapping *interest_person_map;
	interest_person_map = (InterestPersonMapping *)mmapr(interest_person_mapping, &interest_person_mapping_length);

	char *tags_path = makepath(folder, (char *)"tags", (char *)"bin");
	FILE *tags_out = open_binout(tags_path);
	Tag *new_tag = new Tag();
	unsigned short tag_offset = 0;

	unsigned int current_posting_offset = 0;
	char *postings_path = makepath(folder, (char *)"postings", (char *)"bin");
	FILE *postings_out = open_binout(postings_path);

	ipm_index = 0;
	unsigned int tags_written = 0;
	InterestPersonMapping *cipm = &interest_person_map[ipm_index];
	for (unsigned interest = min; interest <= max; interest++) {
		unsigned int start_posting_offset = current_posting_offset;

		while(ipm_index < interest_person_mapping_length / sizeof(InterestPersonMapping) && cipm->interest == interest) {
			fwrite(&cipm->poffset, sizeof(unsigned int), 1, postings_out);
			current_posting_offset++;
			cipm = &interest_person_map[++ipm_index]; // get next ipm
		}

		new_tag->posting_first = start_posting_offset;
		new_tag->posting_n = current_posting_offset - start_posting_offset;
		fwrite(new_tag, sizeof(Tag), 1, tags_out);
		tags_written++;
	}
	fclose(tags_out);
	fclose(postings_out);
	delete new_tag;
	printf(" \t Finished Writing Tag (%d) and Postings(%u)\n", tags_written, current_posting_offset);
}

int main(int argc, char *argv[])
{

	fprintf(stdout, "Starting reorg\n");
	auto t1 = std::chrono::high_resolution_clock::now();

	char *folder = argv[1];

	// STEP 01: Filter for locality
	filter_person_location(folder);

	// STEP 03: Filter for mutual friends only and reduce intersts.bin:
	filter_mutual_friends_and_reduce_interests(folder);

	sort_person(folder);

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
