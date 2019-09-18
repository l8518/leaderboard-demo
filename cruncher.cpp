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

#define QUERY_FIELD_QID 0
#define QUERY_FIELD_A1 1
#define QUERY_FIELD_A2 2
#define QUERY_FIELD_A3 3
#define QUERY_FIELD_A4 4
#define QUERY_FIELD_BS 5
#define QUERY_FIELD_BE 6

PackedPerson *person_map;
unsigned int *knows_map;
unsigned int *tags_map;
unsigned int *postings_map;
Date *date_map;
char* score_map = NULL;

unsigned long person_length, knows_length, tags_length, postings_length, date_length;
unsigned int person_elem_count, knows_elem_count, tags_elem_count, postings_elem_count, date_elem_count;

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

void calculate_bitmap(unsigned short artist, unsigned int from, unsigned int to, std::vector<bool> *bitmap) {
	for (unsigned int i = from; i < to; i++ ) {
		unsigned int poffset = postings_map[i];
		(*bitmap)[poffset] = 1;		
	}
}

unsigned int lower_bound(unsigned int *a, unsigned int s, unsigned int e, unsigned int v) {

	int i = s;
	int j = e;
	int m;
	while (i <= j) {
		m = (i + j) / 2;

		if (v < a[m])
			j = m - 1;
		else if (v > a[m])
			i = m + 1;
		else
			return m;
	}
	return std::max(s, (unsigned int)m);
}

unsigned int upper_bound(unsigned int *a, unsigned int s, unsigned int e, unsigned int v) {

	int i = s;
	int j = e;
	int m;
	while (i <= j) {
		m = (i + j) / 2;
		if (v < a[m])
			j = m - 1;
		else if (v > a[m])
			i = m + 1;
		else
			return m;
	}
	return std::min(e, (unsigned int) m + 1);
}


void fetch_postings(unsigned int t, unsigned int t2, std::vector<bool> *bitmap, unsigned int poffset_lower, unsigned int poffset_upper) {

	unsigned int start = t;
	unsigned int end = t2 - 1;

	unsigned int bs = lower_bound(postings_map, start, end, poffset_lower) + 1;
	unsigned int be = upper_bound(postings_map, start, end, poffset_upper) - 1;

	unsigned int bsp = postings_map[bs];
	unsigned int bep = postings_map[be];
	// Hotfix for broken bin search for bounds; TODO
	if (postings_map[bs - 1] < poffset_lower) {
		bs = bs;
	} else {
		bs = bs - 1;
	}

	if (postings_map[be] > poffset_upper) {
		be = be - 1;
	} else {
		be = be;
	}

	for (unsigned int i = bs; i <= be; i++ ) {
		unsigned int poffset = postings_map[i];
		// candidates should not like A1
		if ((*bitmap)[poffset]) continue;
		score_map[poffset] += 1;
	}
}

void build_person_candidates(unsigned short a2, unsigned short a3, unsigned short a4, std::vector<bool> *bitmap, unsigned int poffset_lower, unsigned int poffset_upper) {
	fetch_postings(tags_map[a2], tags_map[a2+1], bitmap, poffset_lower, poffset_upper);
	fetch_postings(tags_map[a3], tags_map[a3+1], bitmap, poffset_lower, poffset_upper);
	fetch_postings(tags_map[a4], tags_map[a4+1], bitmap, poffset_lower, poffset_upper);
}

void query(unsigned short qid, unsigned short artist, unsigned short areltd[], unsigned short bdstart, unsigned short bdend)
{
	std::vector<bool> bitmap(person_length / sizeof(PackedPerson));
	// fast allocation with calloc
	score_map = (char*) calloc (person_length / sizeof(PackedPerson), sizeof(char));
	// init bitmap for candidates
	calculate_bitmap(artist, tags_map[artist], tags_map[std::min( (unsigned int) artist + 1, tags_elem_count)], &bitmap);

	build_person_candidates(areltd[0], areltd[1], areltd[2], &bitmap, date_map[bdstart].person_first,date_map[bdend+1].person_first);

	unsigned int result_length = 0, result_idx, result_set_size = 15000;
	unsigned int kpoffset;
	Result* results = (Result*)malloc(result_set_size * sizeof (Result));

	PackedPerson *p, *f;
	for(unsigned int i = 0; i < person_elem_count; i++) {
		char score = score_map[i];
		if (score == 0) continue;

		p = &person_map[i];

		int max_knows = (i + 1 > person_elem_count ) ?  knows_elem_count : person_map[i + 1].knows_first;
		
		for (int koffset = p->knows_first; koffset < max_knows; koffset++) {
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
			results[result_length].score = score;
			result_length++;
		}
	}

	delete(score_map);

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
	person_map = (PackedPerson *)mmapr(makepath((char *)query_path, (char *)"packed_person", (char *)"bin"), &person_length);
	knows_map = (unsigned int *)mmapr(makepath((char *)query_path, (char *)"knows_bsort", (char *)"bin"), &knows_length);
	postings_map = (unsigned int *)mmapr(makepath((char *)query_path, (char *)"postings", (char *)"bin"), &postings_length);
	tags_map = (unsigned int *)mmapr(makepath((char *)query_path, (char *)"tags", (char *)"bin"), &tags_length);
	date_map = (Date *)mmapr(makepath((char *)query_path, (char *)"date", (char *)"bin"), &date_length);

	person_elem_count = person_length / sizeof(PackedPerson);
	knows_elem_count = knows_length / sizeof(unsigned int);
	postings_elem_count = postings_length / sizeof(unsigned int);
	tags_elem_count = tags_length / sizeof(unsigned int);
	date_elem_count = person_length / sizeof(Date);

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
