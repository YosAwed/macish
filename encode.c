/*
  "encode.c" ish for Human68K
  ken Miyazaki & o.imaizumi
  88/02/06
  88/05/10 for UNIX (kondo)
  90/09/12 non-kana (keizo)

  93/08/23 -mv (aka)
  93/11/25 line number BUG FIX (aka)
  96/03/16 long filename (MicroCassetteMan)
  96/03/17 multi byte filename (MicroCassetteMan)
  12/09/17 for Linux (bko)
*/

#define __MACXCODE__
#include "ish.h"

extern void chksum(), print(), puttitle(), encd(),
	 hdprint(), sethed(), setwrd(), prtitle();

int	lnum;
long	tim;
extern int	 mline;
extern int	 errno;

unsigned int	fcrc16,vcrc16,vnum,vlin,vmax;
unsigned long	fcrc32,vsize;

int encode(int oflg)
{
	int	err;

	/* check multi volume.? */
	if (mline <= 80) {
		mline=0;
		return encode1(oflg);	/* single volume */
	}
	else {
		vnum=0;
		while ((err=mencode(oflg)) == 0) ;
		if(err == 100) return 0;
		else return err;
	}
}

int encode1(int oflg)
{
	int lin, j, rest;
	ushort crc;
	struct stat atrbuf;
	struct tm *tm;
	char *p,fname2[NAMLEN];

	if (oflg == 0) {
		strcpy(fname2,filename);
		if ((p=rindex(fname2,'.')) != NULL) strcpy(p,".ish");
		else strcat(fname2,".ish");
		if ((opath=fopen(fname2, "w")) == NULL) {
			fprintf(stderr, "cannot create \"%s\".", fname2);
			return errno;
		}
		fprintf(stderr, "< Create to %s >\n", fname2);
		clearerr(opath);
	}
	if (opath != stdout) fprintf(stderr, "%s ", filename);
	fstat(path, &atrbuf);
	lsize = atrbuf.st_size;
	if (opath != stdout) fprintf(stderr, "(%ld Bytes)", lsize);
/*  size = (lsize+jis-2)/(jis-3); *//* (lsize+2)+((jis-3)-1)/(jis-3) !! */
/*  size += (size+jis-4)/(jis-3)*2+4;
	if (title)
	size += (size+title-1)/title;
*/
	size=calcline(jis-3,lsize,0);

#ifdef __TURBOC__
	getftime(path, &tim);
#else
	tm = localtime(&atrbuf.st_mtime);
	tim = (tm->tm_year - 80)<<25;
	tim |= (tm->tm_mon + 1)<<21;
	tim |= tm->tm_mday<<16;
	tim |= tm->tm_hour<<11;
	tim |= tm->tm_min<<5;
	tim |= tm->tm_sec>>1;
#endif

	/* write header */

	hdprint();
	sethed();
	if (edmode == JIS8) {
		setcrc(buff, JIS8_L-2);
		enc_jis8(buff, JIS8_L);
	}
	else {
		setcrc(buff, JIS7_L-2);
		enc_jis7(buff, JIS7_L);
	}
	print(obuf);
	print(obuf);
	print(obuf);
	lnum = 5;

	/* encode file */

	for (rest = 0,crc = 0xffff, lin = 1; rest != -1; ) {
		if(ferror(opath)) return ERR;

		/* read file & add crc */
		buff[0] = lin++ & 0xff;
		if (rest) {
			buff[1] = crc;
			for(j = 2; j < jis; buff[j++] = '\0');
			rest = -1;
		}
		else {
			j = read(path, buff+1, jis-3);
			crc = calcrc(buff+1, j, crc);
			if ((rest = jis-3-j) != 0) {
				crc = ~crc;
				if (rest >= 2) {
					setwrd(buff+j+1, crc);
					for(j = j+3; j<jis; buff[j++] = '\0');
					rest = -1;
				}
				else {   /* rest == 1 */
					buff[j+1] = (crc >> 8);
				}
			}
		}
		if (lin == 2) { /* top of block */
			if (opath != stdout) {
				fprintf(stderr, ".");
				fflush(stderr);
			}
			for (j=0; j < LBUFLEN; ++j) {
				tatesum[j] = yokosum[j] = '\0';
			}
		}
		setcrc(buff, jis-2);
		encd(buff);
		chksum(lin-1, jis-2, buff);
		print(obuf);
		if ((lin == jis-2) || rest == -1) { /* end of block */
			tatesum[0] = jis-2;
			setcrc(tatesum, jis-2);
			encd(tatesum);
			print(obuf);
			chksum(jis-2, jis-2, tatesum);
			yokosum[0] = jis;
			if (lin != jis-2) {
				for(; lin < jis-2; lin++) {
					j = ((jis-2)-(lin-1))%(jis-2)+1;
					yokosum[j] = (yokosum[j]&0xff)-(lin&0xff);
				}
			}
			lin = 1;
			setcrc(yokosum, jis-2);
			encd(yokosum);
			print(obuf);
		} 
	}
	if (title) {
		if ((lnum-1) % title) prtitle();
	}
	if (opath != stdout) fprintf(stderr, " converted.\n");

	if (oflg == 0) fclose(opath);
	return 0;
}

void encd(char *argv)
{
	switch(edmode) {
	case JIS7:
		enc_jis7(argv,jis);
		break;
	case JIS8:
		enc_jis8(argv,jis);
		break;
	case SJIS:
		enc_sjis(argv,jis);
		break;
	case NJIS:
		enc_njis(argv,jis);
		break;
	}
}

void hdprint(void)
{
//	int	bnum;

	if (esc == ON) fprintf(opath, "\033[32m");
	fprintf(opath, "<<< %s for ", filename);
	switch(os) {
	case MS_DOS:
		fprintf(opath, "MS-DOS");
		break;
	case CP_M:
		fprintf(opath, "CP/M or MSX-DOS");
		break;
	case OS_9:
		fprintf(opath, "OS-9");
		break;
	case OS_9EXT:
		fprintf(opath, "OS-9Ext");
		break;
	case UNIX:
		fprintf(opath, "UNIX");
		break;
	case LINUX:
		fprintf(opath, "Linux");
		break;
	case OTHER:
		fprintf(opath, "OTHER");
		break;
	case ALL_OS:
		fprintf(opath, "ALL-OS");
		break;
	case MAC:
		fprintf(opath, "Mac");
		break;
	}
	fprintf(opath, " ( use ");
	switch(edmode) {
	case JIS7:
		fprintf(opath, "jis7");
		break;
	case JIS8:
		fprintf(opath, "jis8");
		break;
	case SJIS:
		fprintf(opath, "shift_jis");
		break;
	case NJIS:
		fprintf(opath, "non-kana");
		break;
	}
	fprintf(opath, " ish ) [ %d lines ] >>>",size);
	if(mline) {
		fprintf(opath,"\n<<< Part %d/%d [ %d lines ] >>>",vnum,vmax,vlin);
	}
	if (esc == ON) fprintf(opath, "\033[m\n");
	else fprintf(opath, "\n");
}

void sethed(void)
{
	short i, j;
	uchar *s0, *s, *p;
	/* CEX */
	long	tzone;

	buff[0] = '\0';
	buff[1] = '\0';
	buff[2] = lsize & 0xff;
	buff[3] = (lsize >> 8) & 0xff;
	buff[4] = (lsize >> 16) & 0xff;
	buff[5] = (lsize >> 24) & 0xff;
	buff[6] = jis & 0xff;
	buff[7] = (jis >> 8) & 0xff;
	i=jis-1;
	buff[8] = i & 0xff;
	buff[9] = (i >> 8) & 0xff;
	switch(edmode) {
	case JIS7:
		buff[10] = 16;
		buff[11] = 13;
		break;
	case JIS8:
		buff[10] = 8;
		buff[11] = 8;
		break;
	case SJIS:
		buff[10] = 16;
		buff[11] = 15;
		break;
	case NJIS:
		buff[10] = 16;
		buff[11] = 14;
		break;
	default:
		fprintf(stderr, "unknown!\n");
	}
#ifdef __WIN32__
	if(sname[0] != 0) s0 = sfilename;
	else s0 = (uchar *)filename;
#else
	s0 = (uchar *)filename;
#endif
	for (p = buff+12, i = 0; i < 11; ++i) *p++ = '_';
	for (s = s0, p = buff+12, i = j = 0; i < 8; ) {
		if (*s == '\0' || *s == '.') {*p++ =' '; ++i;}
		// skip white space
		else if (*s == ' ') {++s; ++j;}
		// replace a too long char.
		else if (ismbcwc(s0, j) >= (8-i)) break;
		else {*p++ = *s++; ++i; ++j;}
	}
	s = rindex(s0, '.');
	if (s == NULL) s = rindex(s0,'\0');
	else ++s;
	j = s - s0;
	for (p = buff+20, i = 0; i < 3; ) {
		if (*s == '\0') {*p++ =' '; ++i;}
		// skip white space
		else if (*s == ' ') {++s; ++j;}
		// replace a too long char.
		else if (ismbcwc(s0, j) >= (3-i)) break;
		else {*p++ = *s++; ++i; ++j;}
	}
	buff[23] = '\0';

	buff[24] = os;
	buff[25] = 1;
	buff[26] = 3;

	buff[27] = tim & 0xff;
	buff[28] = (tim >> 8) & 0xff;
	buff[29] = (tim >> 16) & 0xff;
	buff[30] = (tim >> 24) & 0xff;

	/* CEX */
#ifdef	TZSET
	{
		/* tzset() */
		/* extern long timezone; */
		tzset();
		tzone = timezone;
	}
#else
#ifdef FTIME
	{
		/* ftime() */
		struct timeb buf;
		ftime(&buf);
		tzone = buf.timezone * 60L;
	}
#else
	{
		/* gettimeofday */
		struct timeval	tp;
		struct timezone	tzp;
		gettimeofday(&tp, &tzp);
		tzone = tzp.tz_minuteswest * 60L;
	}
#endif
#endif
	buff[31] = tzone & 0xff;
	buff[32] = (tzone >> 8) & 0xff;
	buff[33] = (tzone >> 16) & 0xff;

	for ( i=34 ; i < jis ; ++i ) buff[i]='\0';

	if (mline) {
		buff[1]=vnum;

		buff[26] = 0x1f;

		buff[35] = fcrc16 & 0xff;
		buff[34] = (fcrc16 >> 8) & 0xff;

		buff[36] = fcrc32 & 0xff;
		buff[37] = (fcrc32 >> 8) & 0xff;
		buff[38] = (fcrc32 >> 16) & 0xff;
		buff[39] = (fcrc32 >> 24) & 0xff;

		buff[40] = vsize & 0xff;
		buff[41] = (vsize >> 8) & 0xff;
		buff[42] = (vsize >> 16) & 0xff;
		buff[43] = (vsize >> 24) & 0xff;

		buff[45] = vcrc16 & 0xff;
		buff[44] = (vcrc16 >> 8) & 0xff;
	}
}

void setwrd(uchar *adr,int dat)
{
	adr[0] = (dat>>8)&0xff;
	adr[1] = dat&0xff;
}

void print(char *argv)
{
	fprintf(opath, "%s\n", argv);
	if (title != 0 ) {
		if ((++lnum % title) == 0) prtitle();
	}
}

void prtitle(void)
{
	if (esc == ON) fprintf(opath, "\033[32m");
	if(mline) {
		fprintf(opath, "--- %s (%d/%d) ---", filename, lnum++, vlin);
	}
	else {
		fprintf(opath, "--- %s (%d/%d) ---", filename, lnum++, size);
	}
	if (esc == ON) fprintf(opath, "\033[m\n");
	else fprintf(opath, "\n");
}

/*  accumlate checksum */
void chksum(int no, int len, uchar *argv)
{
	register int i;

	if (no < jis-2) {
		for (i = 1; i < len; i++) tatesum[i] -= argv[i];
	}

	if (no < jis-1) {
		for(i = 0; i < len; i++) {
			yokosum[(len-(no-1)+i)%len+1] -= argv[i];
		}
	}
}
