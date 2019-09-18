#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <unordered_map>
#include <set>
#include <typeinfo>
#include <chrono>
#include <bitset>
#include <iostream>
#include <vector>
#include <dirent.h>
#include <bits/stdc++.h> 
#include <algorithm>

#include "utils.h"

CompressedPerson *person_map;
unsigned int *knows_map;
unsigned int *tags_map;
unsigned int *postings_map;
Date *date_map;
std::unordered_map<unsigned int, char> map;

unsigned long person_length, knows_length, tags_length, postings_length, date_length;

FILE *outfile;

int main(int argc, char *argv[])
{
	if (argc < 1)
	{
		fprintf(stderr, "Usage: [datadir]\n");
		exit(1);
	}
	const char *query_path = (argv[1]);

	/* memory-map files created by loader */
	person_map = (CompressedPerson *)mmapr(makepath((char *)query_path, (char *)"person_bsort", (char *)"bin"), &person_length);
	knows_map = (unsigned int *)mmapr(makepath((char *)query_path, (char *)"knows_bsort", (char *)"bin"), &knows_length);
	postings_map = (unsigned int *)mmapr(makepath((char *)query_path, (char *)"postings", (char *)"bin"), &postings_length);
	tags_map = (unsigned int *)mmapr(makepath((char *)query_path, (char *)"tags", (char *)"bin"), &tags_length);
	date_map = (Date *)mmapr(makepath((char *)query_path, (char *)"date", (char *)"bin"), &date_length);

	outfile = fopen("./debug_person.bin", "w");
	for (int i = 0; i < person_length / sizeof(CompressedPerson); i++) {
		CompressedPerson *p = &person_map[i];
		fprintf(outfile, "%0*hu|%014lu\n", 4, p->birthday, p->person_id);
	}
	fclose(outfile);

	outfile = fopen("./debug_knows.bin", "w");
	for (int i = 0; i < knows_length / sizeof(unsigned int); i++) {
		unsigned int knows_offset = knows_map[i];
		fprintf(outfile, "%014i\n", knows_offset);
	}
	fclose(outfile);

	outfile = fopen("./debug_postings.bin", "w");
	for (int i = 0; i < postings_length / sizeof(unsigned int); i++) {
		unsigned int postings_offset = postings_map[i];
		fprintf(outfile, "%014i\n", postings_offset);
	}
	fclose(outfile);

	outfile = fopen("./debug_tags.bin", "w");
	for (int i = 0; i < tags_length / sizeof(unsigned int); i++) {
		unsigned int *t = &tags_map[i];
		fprintf(outfile, "%014i\n", t->posting_first);
	}
	fclose(outfile);

	outfile = fopen("./debug_date.bin", "w");
	for (int i = 0; i < date_length / sizeof(Date); i++) {
		Date *d = &date_map[i];
		fprintf(outfile, "%014i, %014i\n", d->person_first, d->person_n);
	}
	fclose(outfile);

	return 0;
}