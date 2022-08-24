// intexer.c : Defines the entry point for the console application.

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

struct sigrecord {
	unsigned int signum;
	unsigned char signame[7];
	unsigned char sigdesc[100];
};


// Read a size_t from in
// If successful, value read goes into result & return 1
// Otherwise, return 0
int read_size(FILE* in, unsigned long* result) {
	char buff[25]; // large enough to hold a size_t
	char *end_ptr;

	if (1 != fscanf( in, " %24s ", buff)) {
		return 0;
	}
	errno = 0;
	*result = strtoul(buff, &end_ptr, 10);

	if (ERANGE == errno) {
		return 0;
	} else if (end_ptr == buff) {
		return 0;
	} else if ('\n' != *end_ptr && '\0' != *end_ptr) {
		return 0;
	} else
		return 1;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s data_base\n", argv[0]);
		return 0;
	}

	// Construct full pathname of sig database from args & env
	char *full_path; // path used to open file
	char *new_path = NULL; // newly malloc'd path to free later
	const char *data_path = getenv("DATA_PATH");
	if (data_path == NULL) {
		full_path = argv[1];
	} else {
		size_t path_size = 0;
		const size_t dplen = strlen(data_path);
		const size_t fnlen = strlen(argv[1]);
		if (dplen > SIZE_MAX - fnlen)  { 
			fprintf(stderr, "integer overflow.\n");
			return -1;
		}	else {
			path_size = dplen + fnlen;
		}
		if (path_size > SIZE_MAX - 2)  { 
			fprintf(stderr, "integer overflow.\n");
			return -1;
		}	else {
			path_size += 2;
		}

		if ((new_path = (char*) malloc(path_size)) == NULL) {
			fprintf(stderr, "cannot allocate memory for full path.\n");
			return -1;
		}
		full_path = new_path;
		strcpy(full_path, data_path);     
		if (full_path[dplen-1] != '/') {
			strcat(full_path, "/");
		}
		strcat(full_path, argv[1]);
	}

	// Open database file for reading
	FILE *in;
	struct sigrecord *sigdb = NULL;
	if ((in = fopen(full_path, "r")) == NULL) {
		fprintf(stderr, "Cannot open input file \n");
		goto free_path;
	}

	// Read total number of signals
	unsigned long input_number;
	if (!read_size( in, &input_number)) {
		fprintf(stderr, "Could not read number of records from database.\n");
		goto close_files;
	}
	if (input_number == 0 || input_number >= SIZE_MAX) {
		fprintf(stderr, "Invalid db_size specified\n");
		goto close_files;
	}
	size_t db_size;
	db_size = ((size_t) input_number) + 1;

	// Allocate space to hold signal database
	// The database runs from [0, db_size], and is indexed by
	// signal number. (So the 0th index is unused).
	if (db_size > SIZE_MAX/sizeof(struct sigrecord)) {
		fprintf(stderr, "integer overflow.\n");
		goto close_files;
	}
	if ((sigdb = (struct sigrecord*) malloc(db_size * sizeof(struct sigrecord)))
			== NULL) {
		fprintf(stderr, "Could not allocate memory for database.\n");
		goto close_files;
	}

	// Mark all array elements as invalid; will later be overwritten with
	// valid values
	size_t i;
	for (i = 0; i < db_size; i++) {
		sigdb[i].signum = 0; // invalid value
	}

	// Now read all signals from database
	for (i = 1; i < db_size; i++) {
		// read in number, serves as index key
		if (!read_size( in, &input_number)) {
			fprintf(stderr, "Could not read signal number from database.\n");
			goto close_files;
		}
		if (input_number >= db_size) {
			fprintf(stderr, "Invalid signal number from database.\n");
			goto close_files;
		}
		sigdb[input_number].signum = (unsigned int) input_number;

		// read in name
		if (1 != fscanf(in, "%6s", sigdb[input_number].signame)) { // 6+1=7==sizeof( signame) //???
			fprintf(stderr, "Could not read signal name from database.\n");
			goto close_files;
		}

		// read in description
		size_t j = 0;
		int c;
		while ((c = fgetc(in)) != EOF) {

			// don't read more than 100 chars or until a line feed or carriage return is found
			if ( (j == 99) || (c == 0x0A) || (c == 0x0D) ) break;
			sigdb[input_number].sigdesc[j] = (unsigned char)c;
			j++;
		} // read end of file not reached
		sigdb[input_number].sigdesc[j] = '\0';
	} // end for each signal record 


	/* Loop until 'q' and print out signal information */
	char input[10];
	while (1) {
		printf("Enter a signal number, or 'q' to quit\n");

		if (1 != scanf("%9s", input)) { // 9+1=10==sizeof( input) //???
			fprintf(stderr, "Could not read input signal number.\n");
			break;
		}
		if (input[0] == 'q') {
			break;
		}	else {
			unsigned long ul;
			char *end_ptr;

			errno = 0;
			ul = strtoul(input, &end_ptr, 0);

			if (ERANGE == errno) {
				fprintf( stderr, "number out of range\n");
			} else if (end_ptr == input) {
				fprintf( stderr, "invalid numeric input\n");
			} else if (ul > db_size) {
				fprintf( stderr, "value out of range\n");
			} else if (sigdb[ul].signum == 0) {
				fprintf( stderr, "invalid signal number\n");
			}	else {
				unsigned int idx;
				idx = (unsigned int)ul;
				printf("%u %s %s\n", sigdb[idx].signum, sigdb[idx].signame, sigdb[idx].sigdesc);
			}
		} // end if not quiting	
	} // end wile loop for input

 close_files:
	fclose(in);

 free_path:
	free(new_path);
	free(sigdb);

	return 0;
}

