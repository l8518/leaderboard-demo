#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <limits>
#include <math.h>

#include "utils.h"

// PGM Variables:
unsigned long person_length, knows_length, interest_length;
CompressedPerson *person_com_map;
unsigned int *knows_map;
unsigned short *interest_map;

// Internally needed structs.
struct InterestPersonMapping {
	unsigned int poffset = 0;
	unsigned short interest = 0;
} ;

struct PersonBirthdayMapping {
	unsigned long  person_id;
	unsigned long  knows_first;
	unsigned long  interests_first;
	unsigned short knows_n;
	unsigned short interest_n;
	unsigned short birthday;
	unsigned int old_offset;
} ;

// Comparator for the InterestPersonMapping
int ipm_comparator(const void *v1, const void *v2)
{
	InterestPersonMapping *r1 = (InterestPersonMapping *)v1;
	InterestPersonMapping *r2 = (InterestPersonMapping *)v2;
	if (r1->interest > r2->interest)
		return +1;
	else if (r1->interest < r2->interest)
		return -1;
	else if (r1->poffset > r2->poffset)
		return +1;
	else if (r1->poffset < r2->poffset)
		return -1;
	else
		return 0;
}

// Comparator for the PersonBirthdayMapping
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

/**
 * Filter each person's friendships
 * for the same location.
 * */
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

	// Iterate over every person, and filter their friendships.
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

/**
 * This function filters each person's friendships
 * and strips all interests of people that have
 * been removed during the filter process (having no friends left)
 * */
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

	// Write the person.bin new for mutual only friendships.
	for (i = 0; i < person_length / sizeof(CompressedPerson); i++)
	{
		p = &person_com_map[i];

		// Don't write person, if it doesn't have any friends.
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
			// Write friendship, if mutual
			if (is_mutual == true) {
				unsigned int newOff= offset_map[offset];
				fwrite(&newOff, sizeof(unsigned int), 1, knows_out);
				new_knows_pos++;
			}
		}

		// Rewrite the whole interests 
		// (as we will have superfluous ones from removed entries in person.bin)
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

/**
 * This function sorts the Person.bin for the birthday column.
 * */
void sort_person(char *folder) {
	printf("Enter sort_person\n");
	
	// Sort Person After Birthday And Update Knows.bin
	PersonBirthdayMapping* person_birthday_mapping = NULL;
	person_birthday_mapping = new PersonBirthdayMapping[person_length / sizeof(CompressedPerson)];
	unsigned short bday_max = std::numeric_limits<unsigned short>::min();;
	unsigned short bday_min = std::numeric_limits<unsigned short>::max();;

	// Read every person into a memory, with the new offset.
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
		bday_max = std::max(bday_max, (unsigned short)p->birthday);
		bday_min = std::min(bday_min, (unsigned short)p->birthday);
	}

	// Sort this new person structure.
	qsort(person_birthday_mapping, person_length / sizeof(CompressedPerson), sizeof(PersonBirthdayMapping), &pbm_comparator);

	// Build the offset map, mapping each old offset to the new one.
	unsigned int* offset_map = NULL;
	offset_map = new unsigned int[person_length / sizeof(CompressedPerson)];
	for (unsigned int i = 0; i < person_length / sizeof(CompressedPerson); i++) {
		offset_map[person_birthday_mapping[i].old_offset] = i;
	}

	// Define Output files for the now birthday sorted files.
	char *person_file = makepath(folder, (char *)"person_bsort", (char *)"bin");
	char *knows_file = makepath(folder, (char *)"knows_bsort", (char *)"bin");
	char *date_file = makepath(folder, (char *)"date", (char *)"bin");
	FILE *person_out = open_binout(person_file);
	FILE *knows_out = open_binout(knows_file);
	FILE *date_out = open_binout(date_file);

	unsigned int current_person_pos = 0;
	unsigned int start_current_person_pos = 0;
	unsigned int new_knows_pos = 0;

	/**
	 * Create the new person.bin and knows.bin that are sorted by 
	 * birthday. Addtionally, create a index on the birthday.
	 * */
	PersonBirthdayMapping *p;
	CompressedPerson *new_p = new CompressedPerson();
	Date *new_date = new Date();
	for (unsigned short current_bday = 0; current_bday <= bday_max; current_bday++) {
		start_current_person_pos = current_person_pos;

		for (unsigned int i = start_current_person_pos; i < person_length / sizeof(CompressedPerson); i++)
		{
			p = &person_birthday_mapping[i];

			// Skip this loop, when we haven't reached the person's birthday
			if (p->birthday != current_bday) break;

			// Iterate over each friendship, and write it with new offset.
			unsigned long start_new_knows_pos = new_knows_pos;
			for (unsigned int j = p->knows_first; j < p->knows_first + p->knows_n; j++)
			{
				unsigned int offset = offset_map[knows_map[j]];
				fwrite(&offset, sizeof(unsigned int), 1, knows_out);
				new_knows_pos++;
			}

			// Write the new person and the new offset.
			new_p->person_id = p->person_id;
			new_p->birthday = p->birthday;
			new_p->knows_first = start_new_knows_pos;
			new_p->knows_n = (new_knows_pos - start_new_knows_pos);
			new_p->interests_first = p->interests_first;
			new_p->interest_n = p->interest_n;
			fwrite(new_p, sizeof(CompressedPerson), 1, person_out);
			current_person_pos++;
		}

		// Write date.bin index.
		new_date->person_first = start_current_person_pos;
		new_date->person_n = current_person_pos - start_current_person_pos;
		fwrite(new_date, sizeof(Date), 1, date_out);
	}

	// free memory:
	delete new_p;
	delete offset_map;
	delete person_birthday_mapping;

	// Close files
	fclose(person_out);
	fclose(knows_out);

	munmap(person_com_map, person_length);
	munmap(knows_map, knows_length);
	person_com_map = (CompressedPerson *)mmapr(person_file, &person_length);
	knows_map = (unsigned int *)mmapr(knows_file, &knows_length);
	printf("Exit sort_person\n");
}

/**
 * Build inverted lists with the following steps:
 * - Determine the range of interests (highest and lowest value)
 *   to determine a how many interests need to be written into
 *   each bucket.
 * - Then split for every person their interests into these buckets
 *   with their own offset. Write each bucket to disk.
 * - Afterwards, sort each bucket with quicksort
 * - Merge each sorted bucket.
 * 
 * */
void build_inverted_list(char *folder) {
	printf("Enter build_inverted_list\n");

	InterestPersonMapping *ipm;

	// Determine the MAX and MIN value of the interests.
	unsigned short max = std::numeric_limits<unsigned short>::min();;
	unsigned short min = std::numeric_limits<unsigned short>::max();;

	for (int i = 0; i < interest_length / sizeof(unsigned short); i++) {
		unsigned short interest = interest_map[i];
		max = std::max(max, (unsigned short)interest);
		min = std::min(min, (unsigned short)interest);
	}

	// Create the according buckets.
	const int max_files = 10;
	const int interest_range = ceil(max / max_files) + 1;
	FILE *interest_buckets[10];
	for (int i = 0; i < max_files; i++) {
		std::string filepart = "interest_split_" + std::to_string(i);
		char *file_name = makepath(folder, (char *)filepart.c_str(), (char *)"bin");
		interest_buckets[i] = fopen(file_name, "wb");
	}

	// Write each bucket by going over every person and their interest.
	unsigned int ipm_index = 0;
	ipm = new InterestPersonMapping();
	CompressedPerson *p;
	for (unsigned int i = 0; i < person_length / sizeof(CompressedPerson); i++) {
	 	p = &person_com_map[i];
	 	for (int j = p->interests_first; j < p->interests_first + p->interest_n; j++)
	 	{
			unsigned short interest = interest_map[j];
			ipm->interest = interest;
			ipm->poffset = i;
			int bucket = floor(interest / interest_range);
			fwrite(ipm, sizeof(InterestPersonMapping), 1, interest_buckets[bucket]);
	 	}
	}
	delete ipm;

	// Close each file, so we can mmap it later.
	for (int i = 0; i < max_files; i++) {
		fclose(interest_buckets[i]);
	}

	// MMAP files and sort them via quicksort + merge into new file.
	unsigned long ipm_length;
	char *interest_person_mapping = makepath(folder, (char *)"interest_person_mapping", (char *)"bin");
	FILE *ipm_out = open_binout(interest_person_mapping);
	for (int i = 0; i < max_files; i++) {
		std::string filepart = "interest_split_" + std::to_string(i);
		char *file_name = makepath(folder, (char *)filepart.c_str(), (char *)"bin");
		InterestPersonMapping *ipm_map = (InterestPersonMapping *)mmaprw(file_name, &ipm_length);
		qsort(ipm_map, ipm_length / sizeof(InterestPersonMapping), sizeof(InterestPersonMapping), &ipm_comparator);
		// append to the file
		for (int y = 0; y < ipm_length / sizeof(InterestPersonMapping); y++) {
			ipm = &ipm_map[y];
			fwrite(ipm, sizeof(InterestPersonMapping), 1, ipm_out);
		}
		munmap(ipm_map, ipm_length);
	}
	fclose(ipm_out);

	// Delete the previously created buckets.
	for (int i = 0; i < max_files; i++) {
		std::string filepart = "interest_split_" + std::to_string(i);
		char *file_name = makepath(folder, (char *)filepart.c_str(), (char *)"bin");
		remove(file_name);
	}

	// Write the tag.bin and posting.bin
	printf(" \t Writing Tag and Postings\n");
	unsigned long interest_person_mapping_length;
	InterestPersonMapping *interest_person_map;
	interest_person_map = (InterestPersonMapping *)mmapr(interest_person_mapping, &interest_person_mapping_length);

	char *tags_path = makepath(folder, (char *)"tags", (char *)"bin");
	FILE *tags_out = open_binout(tags_path);
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

		fwrite(&start_posting_offset, sizeof(unsigned int), 1, tags_out);
		tags_written++;
	}
	fclose(tags_out);
	fclose(postings_out);
	printf(" \t Finished Writing Tag (%d) and Postings(%u)\n", tags_written, current_posting_offset);
}

/**
 * Compress the person.bin into a smaller more packed version (also get rid of 
 * not required attributes)
 * */
void pack_person(char *folder) {
	char *pperson_path = makepath(folder, (char *)"packed_person", (char *)"bin");
	FILE *pperson_out = open_binout(pperson_path);
	CompressedPerson *p;
	PackedPerson *pp = new PackedPerson();
	for (int i = 0; i < person_length / sizeof(CompressedPerson); i++) {
		p = &person_com_map[i];
		pp->person_id = p->person_id;
		pp->knows_first = p->knows_first;
		fwrite(pp, sizeof(PackedPerson), 1, pperson_out);
	}
	fclose(pperson_out);
}

/**
 * This reorg program reduces the amount of entries for person.bin and knows.bin and
 * prepares different data structures to allow easier access.
 * */
int main(int argc, char *argv[])
{
	char *folder = argv[1];

	// Filter for same location
	filter_person_location(folder);

	// Filter for mutual friendships, reduce interests bin
	filter_mutual_friends_and_reduce_interests(folder);

	// Sort Peron.bin by birthday
	sort_person(folder);

	// Build the inverted list for interests.
	build_inverted_list(folder);

	// Write a more compact version of the person.bin
	pack_person(folder);

	printf("Lines in latest person.bin: \t %lu \n", person_length / sizeof(CompressedPerson) );
	printf("Lines in latest knows.bin: \t %lu \n", knows_length / sizeof(unsigned int) );
	printf("Lines in latest interest.bin: \t %lu \n", interest_length / sizeof(unsigned short) );
	return 0;
}
