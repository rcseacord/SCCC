/** \file Header, library and main program for intexer */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

/** \file better-intexer.h */

#ifndef INTEXER_H
#define INTEXER_H

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

#ifdef _WIN32
const char *OS_PATH_SEP = "\\";
#else
const char *OS_PATH_SEP = "/";
#endif

struct sigrecord {
	unsigned short signum;
	char signame[7];
	char sigdesc[100];
};

size_t checked_add( size_t lhs, size_t rhs );
void *checked_malloc( size_t nmemb, size_t size );
struct sigrecord *checked_fgets( struct sigrecord *rec, FILE *fh );
FILE *datafile_open( const char *path_arg );

#endif /* end file better-intexer.h */

/** \brief Checks for wraparound and sets the result to 0 if so. */
size_t checked_add( size_t lhs, size_t rhs )
{
	size_t result = lhs + rhs;

	if( SIZE_MAX - rhs < lhs )
	{
		result = 0;
	}

	return result;
}

/** \brief Wrapper for malloc that validates the arguments. Errno is set to EINVAL on error. */
void *checked_malloc(size_t nmemb, size_t size )
{
	void *ret = NULL;
	if( size > 0 && nmemb > 0 && nmemb <= SIZE_MAX / size )
	{
		ret = malloc(nmemb * size);
	}
	else
	{
		errno = EINVAL;
	}
	return ret;
}

/** \brief Attempts to read the id, name and description of a \p rec from an \fh

    \returns \p rec or NULL on error, check errno.
*/
struct sigrecord *checked_fgets( struct sigrecord *rec, FILE *fh )
{
	char buff[sizeof(rec->signame) + sizeof(rec->sigdesc)];

	if( 1 == fscanf(fh, "%hu ", &rec->signum)
		&& NULL != fgets(buff, sizeof(buff), fh) )
	{
		char *sep = strchr(buff, ' ');

		if( NULL != sep && sep - buff < sizeof(rec->signame) )
		{
			char *nl;

			strncpy(rec->signame, buff, sep - buff);
			rec->signame[sep - buff] = '\0';
			strncpy(rec->sigdesc, sep + 1, sizeof(rec->sigdesc));
			rec->sigdesc[sizeof(rec->sigdesc)-1] = '\0';

			nl = strchr(rec->sigdesc, '\n');
			if(nl) *nl = '\0';
		}
		else
		{
			errno = EINVAL;
			return NULL;
		}
	}
	else
	{
		errno = EINVAL;
		return NULL;
	}

	return rec;
}

/** \brief handles concatenation of the DATA_PATH and the user-provided path */
static int handle_path_arg(size_t size, char *full_path, const char *path_arg)
{
	int sret;
	char *data_path = getenv("DATA_PATH");
	size_t data_path_len = data_path ? strlen(data_path) : 0;

	const char *sep = NULL;

	if(data_path_len > 0 && data_path[data_path_len-1] != *OS_PATH_SEP)
	{
		sep = OS_PATH_SEP;
	}

	sret = snprintf(full_path, size, "%s%s%s",
			data_path ? data_path : "",
			sep ? sep : "",
			path_arg ? path_arg : ""
			);
	return sret;
}

/** \brief Opens a handle to the file specified by path_arg.

    Prepends file path with the DATA_PATH environment variable if set.

    \returns A file pointer that the user is responsible for closing upon successful completion.
             Otherwise, NULL is return and errno is set to indicate the error.
*/
FILE *datafile_open( const char *path_arg )
{
	FILE *ret = NULL;
	char full_path[MAX_PATH];
	int sret = handle_path_arg(sizeof(full_path), full_path, path_arg);

	if( sret > 0 && sret < MAX_PATH )
	{
		ret = fopen( full_path, "r");
	}
	else
	{
		errno = EINVAL;
	}

	return ret;
}

struct sigrecord *read_records(FILE *in, size_t *size)
{
	struct sigrecord *sigdb = NULL;
	size_t i = 0;

	if( 1 == fscanf(in, "%zu", size) )
	{
		sigdb = (struct sigrecord *) checked_malloc(*size, sizeof(sigdb[0]));
		if (sigdb == NULL) return NULL;

		for (i = 0; i < *size; i++)
		{
			if( NULL == checked_fgets(&sigdb[i], in) )
			{
				free(sigdb);
				return NULL;

			}
		}
	}

	return sigdb;
}

#ifndef TEST
int main(int argc, char* argv[]) {
	size_t size, idx;
	char input[10];

	FILE *in;
	struct sigrecord *sigdb;

	if (argc != 2) {
		printf("Usage: %s data_base\n", argv[0]);
		return 0;
	}

	if ((in = datafile_open(argv[1]) )== NULL) {
		printf("Cannot open input file: %m\n");
		return 1;
	}

	sigdb = read_records(in, &size);
	if (sigdb == NULL) goto close_files;

	/* Loop until 'q' and print out signal information */
	while (NULL != fgets(input, sizeof(input), stdin))
	{
		if ( 0 == strcmp( input, "q\n" ) )
		{
			break;
		}

		if( 1 == sscanf(input, "%zu", &idx) )
		{
			if (idx < size)
			{
				printf("%d %s %s\n", sigdb[idx].signum, sigdb[idx].signame, sigdb[idx].sigdesc);
			}
			else
			{
				printf("Value out of range.\n");
			}
		}
		else
		{
			printf("Invalid argument.\n");
		}
	}

	free(sigdb);

close_files:
	fclose(in);
	return 0;
}
#endif
