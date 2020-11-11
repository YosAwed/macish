/*
  "ish.c"	    Version 1.11

  ish file converter for Human68K

       (C) Pekin
      Public Domain Software

  1987.11.11 ken Miyazaki(ken)
  1987.11.28 O Imaizumi(Gigo)
  1987.12.05 Bug Fix
  1987.12.06 OS9Ext support.
  1988.01.31 V.1.11
  1988.02.06 for Human68K(Gigo)
  1988.05.10 for UNIX (kondo)
  1990.07.12 non-kana (keizo)

  1993.09.25 -mv & -lf (aka)
  1996.01.28 for MS-DOS(MicroCassetteMan)
  1996.03.16 long filename
  1996.03.17 multi byte filename
  2012.09.17 for Linux (bko)
  2020.10.29 for MacOS/XCode (Awed)

  special thanks for M.ishizuka(ish)
*/

#define MAIN
#define __MACXCODE__
#include "ish.h"

// extern int errono;

enum EncDecMode edmode = JIS7;
short int jis = JIS7_L,  esc = OFF, title = 50, mode = RESNEW;
short int os = UNIX;
int  path = 0, size;
long lsize;
char *filename = "file.ish";
/* CEX */
int mline = 0;
int lfflag = 0;

FILE *opath;
FILE *ipath;
char name[NAMLEN+16];
#ifdef __WIN32__
uchar sname[NAMLEN];
uchar *sfilename;
#endif

uchar buff[LBUFLEN];
uchar obuf[LBUFLEN];
uchar tatesum[LBUFLEN];
uchar yokosum[LBUFLEN];
uchar atrb;
uchar myID[96];			// multi volume temporary file ID

static int ishargc=0;

int main(int argc, char **argv)
{
	int i, oflg = 0, n = 0, fc = 0;
	char *p;
//    FILE *fopen(void);
	int ac;
	char **av;

	srand(time(NULL));
	for (i=0; i<96; ++i) {
		do {
			myID[i] = (uchar)rand();
		} while (myID[i] == 0);
	}

	opath = stdout;
#ifdef __WIN32__
	os = MS_DOS;
#endif
#ifdef __LINUX__
	os = LINUX;
#endif

	if (argc < 2) help();
	ac = argc -1;
	av = argv;
	av++;
	while(--argc > 0) {
		if (**++argv == '-') {
			switch(toupper(argv[0][1])) {
			case 'S' :
				switch(toupper(argv[0][2])) {
				case '\0':
				case '7':
					jis = JIS7_L;
					edmode = JIS7;
					break;
				case '8':
					jis = JIS8_L;
					edmode = JIS8;
					break;
				case 'S':
					jis = SJIS_L;
					edmode = SJIS;
					break;
				case 'N':
					jis = NJIS_L;
					edmode = NJIS;
					break;
				default:
					help();
				}
				mode = CREISH;
				break;
			case 'O':
				jis = OJIS;
				fprintf(stderr, "\nSorry, -o not supported.\n\n");
				__exit(225); /* bad para err */
			case 'M':
				/* CEX */
				if (toupper(argv[0][2]) == 'V') {
					mline = atoi(&argv[0][3]);	
				}
				else if(toupper(argv[0][2]) == 'A') os = MAC;
				else os = MS_DOS;
				break;
			case 'L':
				if (toupper(argv[0][2]) == 'F') lfflag=1;
				break;
			case 'C':
				os = CP_M;
				break;
			case '9':
				os = OS_9;
				break;
			case 'K':
				os = OS_9EXT;
				break;
			case 'U':
				os = UNIX;
				break;
			case 'X':
				os = LINUX;
			case '?':
				os = OTHER;
				break;
			case '*':
				os = ALL_OS;
				break;
			case 'N':
				esc = OFF;
				break;
			case 'E':
				esc = ON;
				break;
			case 'T':
				title = atoi(&argv[0][2]);
				break;
			case 'R':
				mode = RESNEW;
				break;
			case 'A':
				mode = RESALL;
				break;
			case 'Q':
				mode = RESASK;
				break;
			case 'F':
				if (argv[0][2] == '=') {
					oflg = -1;
					if (strlen(&argv[0][3]) >= NAMLEN) {
						fprintf(stderr, "specified filename is too long.\n\n");
						__exit(225);
					}
					strcpy(name, &argv[0][3]);
				}
				else oflg = 1;		// output to stdout
				break;
			default:
				help();
			}
		}
		else ishargc++;
	}
/*
	if ((p = rindex(filename, '.')) != NULL)
	if (tolower(*++p) == 'i' && tolower(*++p) == 's' 
	&& tolower(*++p) == 'h' && *++p == '\0' && mode == CREISH)
		mode = RESNEW;
*/
	if (mode == CREISH) {
		puttitle();
		n = 0;
		if (oflg == -1) {
			// if (ishargc == 0) "None of terget file(s)" error will be generated
			if (ishargc) {
				if ((filename = rindex(name, PATHSP)) == 0) filename = name;
				else filename++;
				if ((p = rindex(filename, '.')) == NULL) strcat(name, ".ish");
				opath = fopen(name, "w");
				if (opath == NULL) {
					fprintf(stderr, "cannot create \"%s\".", name);
					__exit(errno);
				}
				fprintf(stderr, "< Create to %s >\n", name);
				if (ishargc >= 2) {
					fprintf(stderr, "Warning - -f= option is used with multi inpit file.\n"
						"          All output will be concatenated to the output file!!!\n\n");
				}
			}
		}
		clearerr(opath);

		int openmode = O_RDONLY | O_BINARY;
		for (; ac > 0; --ac, ++n) {
			if (av[n][0] == '-') continue;
			if ((path = open(av[n], openmode)) == ERR) {
				fprintf(stderr, "cannot open \"%s\".\n", av[n]);
				continue;
			}
#ifdef __WIN32__
			if (GetShortPathName(av[n], sname, NAMLEN) != 0) {
				if ((sfilename = rindex(sname, PATHSP)) == 0) sfilename = sname;
				else sfilename++;
			}
			else sname[0] = 0;
#endif
			if ((filename = rindex(av[n],PATHSP)) == 0) filename = av[n];
			else filename++;
			if (encode(oflg) == ERR) __exit(errno);
			close(path);
			fc++;
		}
		if (fc == 0) fprintf(stderr, "\nNone of terget file(s)\n\n");
	}
	else {		/* Restore */
		if (oflg == 1) {
			fprintf(stderr, "missing output filename in -f option.\n");
			__exit(225);
		}
		if (ishargc == 0) {
			/* from stdin */
			ipath = stdin;
			puttitle();
			fprintf(stderr, "< Restore from stdin >\n");
			if (decode(oflg) == 0) {
				fprintf(stderr,"cannot find ish header.\n");
			}
		}
		else {
			/* restore from file */
			for (n = 0; ac > 0; --ac, ++n) {
				if (av[n][0] == '-') continue;
				filename = av[n];
				if ((ipath = fopen(filename, "r")) == NULL) {
					// name must be retained for -f option
					//strcpy(name, filename);
					//strcat(name, ".ish");
					//if ((ipath = fopen(name, "r")) == NULL) {
						fprintf(stderr, "cannot open \"%s\".\n", filename);
						__exit(errno);
					//}
				}
				if (fc == 0) puttitle();
				fprintf(stderr, "< Restore from %s >\n", filename);
				if (decode(oflg) == 0) {
					fprintf(stderr,"cannot find ish header.\n");
				}
				fclose(ipath);
				++fc;
			}
		}
	}
	__exit(0);
}

void __exit(int rc)
{
	tmpchain *t = root;

	while (t !=NULL) {
		unlink(t->tmpo);
		tmpchain *p = t->next;
		free(t);
		t = p;
	}
	exit(rc);
}

void help(void)
{
	printf("ish %s (%s)\n",VERSION,CCODE);
	printf(
	"Restore : ish <path> [-{r|a|q}] [-{m|c|9|k|u|x|?|*}] [-lf]\n"
	"Store   : ish <path> {-s[8|s|n|7]} [-{m|c|9|k|u|x|?|*}] [-{n|e}] [-tn] [-mvn]\n\n"
	"-r : restore new file(s)                -a : restore all file(s)\n"
	"-q : restore with question\n"
	"-s : store a JIS7 format ish file       -s8: store a JIS8 format ish file\n"
	"-ss: store a SJIS format ish file       -s7: same as -s\n"
	"-sn: store a SJIS non-kana format ish file\n"
	"-m : MS-DOS & Human68K                  -? : other OS(s)\n"
	"-c : CP/M and/or MSX-DOS                -9 : OS-9\n"
	"-k : OS-9/68000 and/or OS-9Ext          -u : UNIX\n"
	"-x : Linix                              -ma: Macintosh\n"
	"-* : all OS(s)\n"
	"-n : no ESC sequence in TITLE line      -e : ESC sequence in TITLE line\n"
	"-tn: title in n line(default 50 line)   -f=<path> : Output to <path>\n"
	"-mvn: multi volume limit n lines        -lf: ignore CR and LF\n"
	);
	__exit(0);
}

void puttitle(void)
{
	fprintf(stderr, "ish file converter for Linux %s\n",VERSION);
	fprintf(stderr, "original sources are Human68K Ver1.11\n");
	fprintf(stderr, "original idea By M.ishizuka(for MS-DOS)\n");
	fprintf(stderr, "copyright 1988 - Pekin - (by Ken+Gigo)\n");
	fprintf(stderr, "    non-kana by keizo   July 12, 1990\n");
	fprintf(stderr, "multi-vol restore supported by taka@exsys.prm.prug.or.jp 1992\n");
	fprintf(stderr, "multi-vol store   supported by aka@redcat.niiza.saitama.jp 1993\n");
	fprintf(stderr, "ported back to MS-DOS and to ComWin by MicroCassetteMan 1996\n");
	fprintf(stderr, "brushup for Linux and MinGW by bko 2012\n\n");
	fprintf(stderr, "ported for Mac XCode by Awed 2020\n\n");
}
