#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define	LINELENGTH 40

char *decrypt(char*, int);
int usage(char*);

int main(int argc, char *argv[])
{
	char *inbuf, *keystr, *outbuf;
	char *errmsg;
	FILE *infile, *keyfile, *outfile;
	int oflag=0;
	
	if (argc != 3 && argc != 4) {
		errmsg = (char *)malloc(60); /* the error message won't be more than 60 chars */
		sprintf(errmsg, "sorry, %s\nusage: caesar secret_file keys_file [output_file]\n", getenv("USER"));
		usage(errmsg);
		free(errmsg);
		exit(1);
	}

	/* Key file should be readable only by root */
	if ((keyfile = fopen(argv[2], "r")) == NULL)
		errx(1, "Cannot open keys file.");
	struct stat keyst;
	int keyfd;
	if ((fstat( fileno( keyfile), &keyst) != 0) ||
			(keyst.st_mode != (S_IFREG | S_IRUSR)) ||
			(keyst.st_uid != 0))
		errx(1, "Keys file has been tampered with");

	gid_t rgid, egid, sgid;
	uid_t ruid, euid, suid;

	/* Get curret user & group IDs */
	if (getresuid( &ruid, &euid, &suid) != 0)
		errx(1, "getresuid() error");
	if (getresgid( &rgid, &egid, &sgid) != 0)
		errx(1, "getresgid() error");
	/* permanently drop privileges */
	if (setresgid( rgid, rgid, rgid) != 0)
		errx(1, "setresgid() error");
	if (setresuid( ruid, ruid, ruid) != 0)
		errx(1, "setresuid() error");

	/* No valiation for input file, other than ensuring it still exists */
	if ((infile = fopen(argv[1], "r")) == NULL)
		errx(1, "Cannot open input file");

	if (argc == 4) {
		int outfd;
		
		/* Only open output file if doesn't already exist, make it
		 * readable by owner only */
		if ((outfd = open( argv[3], O_WRONLY|O_CREAT|O_EXCL, S_IRUSR)) == -1)
			errx(1, "Cannot open output file.");
		if ((outfile = fdopen( outfd, "w")) == NULL)
			errx(1, "fdopen() error");
		oflag=1;
	}

	if (!(inbuf = malloc(LINELENGTH)))
		err(1, NULL);

	while (fgets(inbuf, LINELENGTH, infile) && !feof(infile)) {
		if (!(keystr = malloc(4*sizeof(char))))
			err(1, NULL);
		fgets(keystr, 4*sizeof(char), keyfile);
		outbuf = decrypt(inbuf, atoi(keystr));
		fputs(outbuf, (oflag ? outfile : stdout));
	}
}

char *decrypt(char *msg, int rot)
{
	int i;
	char ch, *outbuf;
	
	if ((rot < 0) || ( rot >= 26)) 
		errx(i, "bad rotation value");

	if(!(outbuf = malloc(LINELENGTH)))
		err(1, NULL);

	i = 0;
	while (ch = msg[i]) {
		outbuf[i] = isupper(ch) ? ('A' + (ch - 'A' + rot) % 26) : islower(ch) ? ('a' + (ch - 'a' + rot) % 26) : ch;
		++i;
	}
	return outbuf;
}

int usage(char *errmsg)
{
	fputs( errmsg, stderr);
	free(errmsg);
}
