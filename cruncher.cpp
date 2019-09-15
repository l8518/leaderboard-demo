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

#include "utils.h"

#define QUERY_FIELD_QID 0
#define QUERY_FIELD_A1 1
#define QUERY_FIELD_A2 2
#define QUERY_FIELD_A3 3
#define QUERY_FIELD_A4 4
#define QUERY_FIELD_BS 5
#define QUERY_FIELD_BE 6

CompressedPerson *person_map;
unsigned int *knows_map;
Tag *tags_map;
unsigned int *postings_map;
std::unordered_map<unsigned int, char> map;

unsigned long person_length, knows_length, tags_length, postings_length;

FILE *outfile;

int result_comparator(const void *v1, const void *v2)
{
	Result *r1 = (Result *)v1;
	Result *r2 = (Result *)v2;
	if (r1->score > r2->score)
		return -1;
	else if (r1->score < r2->score)
		return +1;
	else if (r1->person_id < r2->person_id)
		return -1;
	else if (r1->person_id > r2->person_id)
		return +1;
	else if (r1->knows_id < r2->knows_id)
		return -1;
	else if (r1->knows_id > r2->knows_id)
		return +1;
	else
		return 0;
}

void calculate_bitmap(unsigned short artist, std::vector<bool> *bitmap) {
	Tag *t = &tags_map[artist];
	// TODO: Here is an issue with n (don't know why -> probs reorg)
	Tag *t2 = &tags_map[artist + 1];
	unsigned int i;
	for (i = t->posting_first; i < t2->posting_first; i++ ) {
		unsigned int poffset = postings_map[i];
		(*bitmap)[poffset] = 1;		
	}
}

void fetch_postings(Tag *t, Tag *t2, std::vector<bool> *bitmap) {
	// TODO: Here is an issue with n (don't know why -> probs reorg)
	for (unsigned int i = t->posting_first; i < t2->posting_first; i++ ) {
		unsigned int poffset = postings_map[i];

		// candidates should not like A1
		if ((*bitmap)[poffset]) continue;

		// should like at least one of those (calculate score)
		if (map.find(poffset) == map.end()) {
			map[poffset] = 1;
		} else {
			map[poffset] = map[poffset] + 1;
		}
	}
}

void build_person_candidates(unsigned short a2, unsigned short a3, unsigned short a4, std::vector<bool> *bitmap) {
	fetch_postings(&tags_map[a2], &tags_map[a2+1], bitmap);
	fetch_postings(&tags_map[a3], &tags_map[a3+1], bitmap);
	fetch_postings(&tags_map[a4], &tags_map[a4+1], bitmap);
}

void query(unsigned short qid, unsigned short artist, unsigned short areltd[], unsigned short bdstart, unsigned short bdend)
{
	std::vector<bool> bitmap(person_length / sizeof(CompressedPerson));
	map.clear();

	calculate_bitmap(artist, &bitmap);
	build_person_candidates(areltd[0], areltd[1], areltd[2], &bitmap);

	unsigned int result_length = 0, result_idx, result_set_size = 15000;
	unsigned int kpoffset;
	unsigned int candidate_poffset;
	Result* results = (Result*)malloc(result_set_size * sizeof (Result));

	CompressedPerson *p, *f;
	for(const auto it : map) {
		candidate_poffset = it.first;
		p = &person_map[candidate_poffset];
		
		 if (p->birthday < bdstart || p->birthday > bdend)
		 	continue;

		for (int koffset = p->knows_first; koffset < p->knows_first + p->knows_n; koffset++) {

			kpoffset = knows_map[koffset];
			
			if (!bitmap[kpoffset]) continue;
			f = &person_map[kpoffset];

			// realloc result array if we run out of space
			if (result_length >= result_set_size) {
				result_set_size *= 1.5;
				results = (Result *)realloc(results, result_set_size * sizeof (Result));
			}
			results[result_length].person_id = p->person_id;
			results[result_length].knows_id = f->person_id;
			results[result_length].score = it.second;;
			result_length++;

		}
	}

	// sort result
	qsort(results, result_length, sizeof(Result), &result_comparator);

	// output
	for (result_idx = 0; result_idx < result_length; result_idx++) {
		fprintf(outfile, "%d|%d|%lu|%lu\n", qid, results[result_idx].score, 
			results[result_idx].person_id, results[result_idx].knows_id);
	}
}

void query_line_handler(unsigned char nfields, char **tokens)
{
	unsigned short q_id, q_artist, q_bdaystart, q_bdayend;
	unsigned short q_relartists[3];

	q_id = atoi(tokens[QUERY_FIELD_QID]);
	q_artist = atoi(tokens[QUERY_FIELD_A1]);
	q_relartists[0] = atoi(tokens[QUERY_FIELD_A2]);
	q_relartists[1] = atoi(tokens[QUERY_FIELD_A3]);
	q_relartists[2] = atoi(tokens[QUERY_FIELD_A4]);
	q_bdaystart = birthday_to_short(tokens[QUERY_FIELD_BS]);
	q_bdayend = birthday_to_short(tokens[QUERY_FIELD_BE]);

	query(q_id, q_artist, q_relartists, q_bdaystart, q_bdayend);
}

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		fprintf(stderr, "Usage: [datadir] [query file] [results file]\n");
		exit(1);
	}
	const char *query_path = (argv[1]);

	/* memory-map files created by loader */
	person_map = (CompressedPerson *)mmapr(makepath((char *)query_path, (char *)"location_friends_mutual_person", (char *)"bin"), &person_length);
	knows_map = (unsigned int *)mmapr(makepath((char *)query_path, (char *)"location_friends_mutual_knows", (char *)"bin"), &knows_length);
	postings_map = (unsigned int *)mmapr(makepath((char *)query_path, (char *)"postings", (char *)"bin"), &postings_length);
	tags_map = (Tag *)mmapr(makepath((char *)query_path, (char *)"tags", (char *)"bin"), &tags_length);

	outfile = fopen(argv[3], "w");
	if (outfile == NULL)
	{
		fprintf(stderr, "Can't write to output file at %s\n", argv[3]);
		exit(-1);
	}

	/* run through queries */
	parse_csv(argv[2], &query_line_handler);

	return 0;
}
