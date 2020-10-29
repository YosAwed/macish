/*
  "encode.c" ish for Human68K
  ken Miyazaki & o.imaizumi
  88/02/06
  88/05/10 for UNIX (kondo)
  90/09/12 non-kana (keizo)

  93/08/23 -mv (aka)
  93/11/25 -mv BUG FIX (aka)
  93/11/29 fclose BUG FIX (aka)
  96/01/28 for MS-DOS (MicroCassetteMan)
  96/03/17 multi byte filename (MicroCassetteMan)
*/

#define __MACXCODE__
#include "ish.h"

void chksum(), print(), puttitle(), encd(),
	hdprint(), sethed(), setwrd(), prtitle();

extern int     lnum;
extern long    tim;
extern int     mline;
extern int     errno;

extern unsigned int     fcrc16,vcrc16,vnum,vlin,vmax;
extern unsigned long    fcrc32,vsize;

int mencode(int oflg)
{
	int lin, j, rest;
	unsigned short crc;
	struct stat atrbuf;
	struct tm *tm;
	unsigned long xlen2,vsize2,vstart,vtmp;
	char fname2[NAMLEN],*p;

	if(vnum == 0) {
		fstat(path, &atrbuf);
		lsize = atrbuf.st_size;

		size = calcline(jis-3,lsize,0);
		if (size <= mline) {
			mline = 0;
			encode1(oflg);
			return 1;
		}

		fcrc16 = 0xffff;
		fcrc32 = 0xffffffff;
		for (j=LBUFLEN; j == LBUFLEN; ) {
			j = read(path,buff,LBUFLEN);
			fcrc16 = calcrc(buff,j,fcrc16);
			fcrc32 = calcrc32(buff,j,fcrc32);
		}
		fcrc16 = (~fcrc16 & 0xffff);
		fcrc32 = ~fcrc32;
		lseek(path,0,0);

		xlen2 = mline/jis;
		j = jis-3;
		while (mline < calcline(j,xlen2*(j*j+2),1)) xlen2 -= 1;
		vsize = xlen2*j*j-2;
		vmax = lsize/vsize;
		if (lsize % vsize) vmax += 1;
		size = calcline(j,vsize,1)*(vmax-1);
		size += calcline(j,lsize-vsize*(vmax-1),1);
	}

	if (vmax >= 100) {
		fprintf(stderr, "too many multi volume.\n");
		return ERR;
	}
	vnum += 1;

	if (oflg == 0) {
		strcpy(fname2,filename);
#ifdef LONGNAME
		if ((p = rindex(fname2, '.')) != NULL) strcpy(p, ".ish.#00");
		else strcat(fname2, ".ish.#00");
#else
		if ((p = rindex(fname2, '.')) != NULL) strcpy(p, ".ish");
		else strcat(fname2, ".ish");
#endif
		j = strlen(fname2);
		fname2[j-3] += vnum/100;
		fname2[j-2] =  (vnum % 100)/10+'0';
		fname2[j-1] =  (vnum % 10)+'0';

	if ((opath=fopen(fname2, "w")) == NULL) return errno;
		fprintf(stderr, "< Create to %s >\n", fname2);
		clearerr(opath);
	}

	if(opath != stdout) fprintf(stderr, "%s ", filename);
	fprintf(stderr, "(%ld Bytes) Volume %d", lsize,vnum);

	vstart = vsize*(long)(vnum -1);
	vsize2 = vsize;
	if (lsize-vstart < vsize) vsize2=lsize-vstart;
	vlin = calcline(jis-3,vsize2,1);

	vcrc16=0xffff;
	for (vtmp=vsize2; vtmp != 0; ) {
		j = LBUFLEN;
		if (vtmp < LBUFLEN) j=vtmp;
		j = read(path,buff,j);
		vcrc16 = calcrc(buff,j,vcrc16);
		vtmp -= j;
	}
	vcrc16 = ~vcrc16;
	lseek(path,vstart,0);

	tm = localtime(&atrbuf.st_mtime);
	tim = (tm->tm_year - 80)<<25;
	tim |= (tm->tm_mon + 1)<<21;
	tim |= tm->tm_mday<<16;
	tim |= tm->tm_hour<<11;
	tim |= tm->tm_min<<5;
	tim |= tm->tm_sec>>1;

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
	lnum = 3;	/* add by taka@exsys.prm.prug.or.jp */
	print(obuf);
	print(obuf);
	print(obuf);
	lnum = 6;

	/* encode file */
	vtmp=vsize2;
	for (rest = 0,crc = 0xffff, lin = 1; rest != -1; ) {
		if (ferror(opath)) return ERR;

		/* read file & add crc */
		buff[0] = lin++ & 0xff;
		if (rest) {
			buff[1] = crc;
			for(j = 2; j < jis; buff[j++] = '\0');
			rest = -1;
		}
		else {
			if (vtmp < jis-3) j=read(path,buff+1,vtmp);
			else j = read(path, buff+1, jis-3);

			vtmp -= j;	
			crc = calcrc(buff+1, j, crc);
			if ((rest = jis-3-j) != 0) {
				crc = ~crc;
				if (rest >= 2) {
					setwrd(buff+j+1, crc);
					for (j = j+3; j<jis; buff[j++] = '\0');
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
			for (j=0; j < LBUFLEN; ++j) tatesum[j] = yokosum[j] = '\0';
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
				for (; lin < jis-2; lin++) {
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

	if(oflg == 0) fclose(opath);
	if(vmax == vnum) return 100;
	return 0;
}

long calcline(int max,long len,long sw)
{
	long line,block;

	len += 2;
	line=len/max;
	if(len % max) line += 1l;
	block=line/max;
	if(line % max) block += 1l;
	line=line+block*2l+4l+sw;
	if(title) block=line/(title-1);
	else block=0;
	line=line+block+1l;
	return line;
}
