#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <set>
#include <typeinfo>

#include "utils.h"

#define QUERY_FIELD_QID 0
#define QUERY_FIELD_A1 1
#define QUERY_FIELD_A2 2
#define QUERY_FIELD_A3 3
#define QUERY_FIELD_A4 4
#define QUERY_FIELD_BS 5
#define QUERY_FIELD_BE 6

Person *person_map;
unsigned int *knows_map;
unsigned short *interest_map;

unsigned long person_length, knows_length, interest_length;

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

std::set<unsigned int> read_people_by_birthday(unsigned short bdstart, unsigned short bdend)
{
	unsigned int person_offset;
	unsigned int person_max_iterations = person_length / sizeof(Person);
	std::set<unsigned int> select_people;
	Person *person;
	printf("Person.bin is %d rows long.\n", person_max_iterations);

	for (person_offset = 0; person_offset < person_max_iterations; person_offset++)
	{
		person = &person_map[person_offset];
		if (person->birthday < bdstart || person->birthday > bdend)
			continue;
		select_people.insert(person_offset);
	}
	printf("Filtered Person.bin is %d rows long.\n", select_people.size());
	return select_people;
}

unsigned char person_get_score(Person *person, unsigned short areltd[])
{
	long interest_offset;
	unsigned short interest;
	unsigned char score = 0;
	for (interest_offset = person->interests_first;
		 interest_offset < person->interests_first + person->interest_n;
		 interest_offset++)
	{

		interest = interest_map[interest_offset];
		if (areltd[0] == interest)
			score++;
		if (areltd[1] == interest)
			score++;
		if (areltd[2] == interest)
			score++;
		// early exit
		if (score > 2)
		{
			break;
		}
	}
	return score;
}

char person_likes_artist(Person *person, unsigned short artist)
{
	long interest_offset;
	unsigned short interest;
	unsigned short likesartist = 0;

	for (interest_offset = person->interests_first;
		 interest_offset < person->interests_first + person->interest_n;
		 interest_offset++)
	{

		interest = interest_map[interest_offset];
		if (interest == artist)
		{
			likesartist = 1;
			break;
		}
	}
	return likesartist;
}

std::set<unsigned int> filter_by_interests(std::set<unsigned int> selected_people, unsigned short artist, unsigned short areltd[])
{

	long interest_offset;
	unsigned short interest;
	unsigned char score;
	std::set<unsigned int> filtered;

	printf("Filtered Person.bin is %d rows long.\n", selected_people.size());
	for (const auto person_offset : selected_people)
	{
		Person *current_person = &person_map[person_offset];
		// person must not like artist yet
		if (person_likes_artist(current_person, artist))
			continue;

		// person must like some of these other guys
		score = person_get_score(current_person, areltd);
		if (score < 1)
			continue;

		// add to filterd:
		filtered.insert(person_offset);
	}
	printf("Refiltered Person.bin is %d rows long.\n", filtered.size());

	return filtered;
}

std::set<Person *> read_friends_by_interest(unsigned short bdstart, unsigned short bdend)
{
	unsigned int person_offset;
	unsigned int person_max_iterations = person_length / sizeof(Person);
	std::set<Person *> select_people;
	Person *person;
	printf("Person.bin is %d rows long.\n", person_max_iterations);

	for (person_offset = 0; person_offset < person_max_iterations; person_offset++)
	{
		person = &person_map[person_offset];
		if (person->birthday < bdstart || person->birthday > bdend)
			continue;
		select_people.insert(person);
	}
	printf("Filtered Person.bin is %d rows long.\n", select_people.size());
	return select_people;
}

void legacy_query(unsigned short qid, std::set<unsigned int> selected_people, unsigned short artist, unsigned short areltd[]) {
	unsigned long knows_offset, knows_offset2;

	Person *person, *knows;

	unsigned int result_length = 0, result_idx, result_set_size = 1000;
	Result* results = (Result*)malloc(result_set_size * sizeof (Result));
	
	for (const auto person_offset : selected_people) {
		person = &person_map[person_offset];

		if (person_offset > 0 && person_offset % REPORTING_N == 0) {
			printf("%.2f%%\n", 100 * (person_offset * 1.0/(person_length/sizeof(Person))));
		}

		// check if friend lives in same city and likes artist 
		for (knows_offset = person->knows_first; 
			knows_offset < person->knows_first + person->knows_n; 
			knows_offset++) {

			knows = &person_map[knows_map[knows_offset]];
			if (person->location != knows->location) continue; 

			// friend must already like the artist
			if (!person_likes_artist(knows, artist)) continue;

			// friendship must be mutual
			for (knows_offset2 = knows->knows_first;
				knows_offset2 < knows->knows_first + knows->knows_n;
				knows_offset2++) {
			
				if (knows_map[knows_offset2] == person_offset) {
					// realloc result array if we run out of space
					if (result_length >= result_set_size) {
						result_set_size *= 2;
						results = (Result *)realloc(results, result_set_size * sizeof (Result));
					}
					results[result_length].person_id = person->person_id;
					results[result_length].knows_id = knows->person_id;
					results[result_length].score = person_get_score(person, areltd);;
					result_length++;
					break;
				}
			}
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

void query(unsigned short qid, unsigned short artist, unsigned short areltd[], unsigned short bdstart, unsigned short bdend)
{
	printf("Running query %d\n", qid);
	auto select_people = read_people_by_birthday(bdstart, bdend);
	auto filtered_people = filter_by_interests(select_people, artist, areltd);

	legacy_query(qid, filtered_people, artist, areltd);
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
	person_map = (Person *)mmapr(makepath((char *)query_path, (char *)"person", (char *)"bin"), &person_length);
	interest_map = (unsigned short *)mmapr(makepath((char *)query_path, (char *)"interest", (char *)"bin"), &interest_length);
	knows_map = (unsigned int *)mmapr(makepath((char *)query_path, (char *)"knows", (char *)"bin"), &knows_length);

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
