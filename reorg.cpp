#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "utils.h"

int main(int argc, char *argv[]) {
	char* person_output_file   = makepath((char*)argv[1], (char*)"person",   (char*)"bin");
	char* interest_output_file = makepath((char*)argv[1], (char*)"interest", (char*)"bin");
	char* knows_output_file    = makepath((char*)argv[1], (char*)"knows",    (char*)"bin");
	
	// this does not do anything yet. But it could...
	
	return 0;
}

