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


#define    LINELENGTH 40

char *decrypt(char*, int);
int usage(char*);

int main(int argc, char *argv[]) {
    char *inbuf, *keystr, *outbuf;
    char *errmsg;

    FILE *infile, *keyfile, *outfile; 
    int infd, keyfd, outfd;  

    gid_t rgid, egid, sgid;
    uid_t ruid, euid, suid;

    int oflag=0;
   
    if (argc != 3 && argc != 4) {
        errmsg = (char *)malloc(60); /* the error message won't be more than 60 chars */
        sprintf(errmsg, "sorry, %s\nusage: caesar secret_file keys_file [output_file]\n", getenv("USER"));
        usage(errmsg);
        free(errmsg);
        exit(1);
    }

    /* key file must be opened first with root privileges */

    if ((keyfd = open(argv[2], O_RDONLY)) == -1) {
      errx(1, "Cannot open keys file.");
    }

    keyfile = fdopen(keyfd,"r");
    if (keyfile == NULL) {
      errx(1, "Cannot open convert key file descriptor to file pointer."); 
    }

    /* temporarily drop privileges */
        

    /* Get current UID's */
    if (getresuid( &ruid, &euid, &suid) != 0)
      errx( 1, "getresuid error");
    if (getresgid( &rgid, &egid, &sgid) != 0)
      errx( 1, "getresgid error");

    /* drop privileges temporarily */
    if (setresgid( rgid, rgid, egid) != 0)
      errx( 1, "setresgid error");
    if (setresuid( ruid, ruid, euid) != 0)
      errx( 1, "setresuid error");

    /* remaining file I/O should be performed as privileged */
    if ((infd = open(argv[1], O_RDONLY)) == -1) {
        errx(1, "Cannot open input file.");
    }

    infile = fdopen(infd,"r");
    if (infile == NULL) {
      errx(1, "Cannot open convert in file descriptor to file pointer."); 
    }

    if (argc == 4) {
      if ((outfd = open(argv[3], O_WRONLY | O_CREAT | O_EXCL, S_IRUSR) ) == -1) {
            errx(1, "Cannot open output file.");
      }
      oflag=1;
    }

    outfile = fdopen(outfd,"w");
    if (outfile == NULL) {
      errx(1, "Cannot open convert out file descriptor to file pointer."); 
    }

    if (!(inbuf = malloc(LINELENGTH)))
        err(1, NULL);

    while (fgets(inbuf, LINELENGTH, infile) && !feof(infile)) {
        if (!(keystr = malloc(4*sizeof(char))))
            err(1, NULL);

	/* regain dropped privileges */
	if (setresuid( ruid, euid, ruid) != 0)
	  errx( 1, "setresuid error"); 
	if (setresgid( rgid, egid, rgid) != 0) 
	  errx( 1, "setresgid error");	 

        fgets(keystr, 4*sizeof(char), keyfile); 

	/* drop privileges temporarily */
	if (setresgid( rgid, rgid, egid) != 0)
	  errx( 1, "setresgid error");
	if (setresuid( ruid, ruid, euid) != 0)
	  errx( 1, "setresuid error");


        outbuf = decrypt(inbuf, atoi(keystr));
        fputs(outbuf, (oflag ? outfile : stdout));
    }
} // end main()

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
        outbuf[i] = isupper(ch) ? ('A' + (ch - 'A' + rot) % 26) :
islower(ch) ? ('a' + (ch - 'a' + rot) % 26) : ch;
        ++i;
    }
    return outbuf;
}

int usage(char *errmsg)
{
    fprintf(stderr, errmsg);
    free(errmsg);
}
