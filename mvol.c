/* multi volume sapport
	mvol.c (C) 1992 by taka@exsys.prm.prug.or.jp */
/*    920701    support multi volume file restore (taka) */
/*    930823    modified (aka)		 */
/*    960128    for MS-DOS (MicroCassetteMan) */
/*    960305    unlink bug fix */
/*    960317    support multi byte filename */
//    120917    for Linux (bko)
//    201029    for Mac XCode (Awed)

#define __MACXCODE__
#include "ish.h"

extern unsigned long    fsize;
extern unsigned long    v_offset;
extern unsigned short   max_vol;
extern int    delvol;

struct IDBLOCK    cur_idb;

tmpchain *root = NULL;

void init_mhead(char *head)
{
	char *p;
	int i;

	/* setup init data */
	cur_idb.id_block_size[0] = 0x00;
	cur_idb.id_block_size[1] = 0x01;
	cur_idb.id_block_version_number[0] = 0x00;
	cur_idb.id_block_version_number[1] = 0x02;
	cur_idb.id_block_id_offset[0] = 0x20;
	cur_idb.id_block_id_offset[1] = 0x00;
	cur_idb.id_block_id_size[0] = 0x60;
	cur_idb.id_block_id_size[1] = 0x00;
	cur_idb.header_copy_offset[0] = 0x80;
	cur_idb.header_copy_offset[1] = 0x00;
	cur_idb.header_copy_size[0] = 0x60;
	cur_idb.header_copy_size[1] = 0x00;
	cur_idb.sequence_number_table_offset[0] = 0xe0;
	cur_idb.sequence_number_table_offset[1] = 0x00;
	cur_idb.sequence_number_table_size[0] = 0x20;
	cur_idb.sequence_number_table_size[1] = 0x00;
	/* clear */
	p = (char *)cur_idb.reserved;
	for (i = 0; i < (16+96+96+32) ; i++) *p++ = 0;
	/* set id block id */
	for (i=0; i<96; ++i) {
		cur_idb.id_block_id[i] = myID[i];
	};

	/* copy header */
	bcopy(head+2, &cur_idb.header_copy, 42);
	/* set file size */
	bcopy(head+2, cur_idb.id_block_offset, 4);
}

int mvol_name(char *mvname)
{
	char *p;
	int  fd, n;
	char tmptmp[NAMLEN+16];

	tmpchain *t = search_tmpo(mvname);
	if (t != NULL) {
		fd = open(t->tmpo, O_RDWR | O_BINARY);
		if (fd < 0) {
			fprintf(stderr, "cannot open temp file \"%s\".\n", t->tmpo);
			__exit(0);
		}
//		long sk = lseek(fd, -sizeof(cur_idb), SEEK_END);
		read(fd,&cur_idb, sizeof(cur_idb));
#ifdef DISPISH
		int j;
		fprintf(stderr, "read:%s\n",t->tmpo);
		fprintf(stderr, "read:seek=%ld\n",sk);
		for (j = 0; j < 4; j++) {
			fprintf(stderr, "read:seq[%d]=%x\n",j,cur_idb.sequence_number_table[j]);
		}
#endif
		delvol=0;
		lseek(fd, v_offset, SEEK_SET);
		return fd;
	}

#ifdef __LINUX__
	strcpy(tmptmp, mvname);
	if ((p = rindex(tmptmp, '.')) != NULL) strcpy(p, ".XXXXXX");
	else strcat(tmptmp,".XXXXXX");

	if ((fd = mkstemp(tmptmp)) < 0) {
		fprintf(stderr,"cannot create temporary file (%d).\n", errno);
		return -1;
	}
#else
#ifdef __WIN32__
	if (GetTempFileName(".","ish",0,tmptmp) == 0) {
		fprintf(stderr,"cannot create temporary file (%d).\n", errno);
		return -1;
	}
	if ((fd = open(tmptmp, O_WRONLY | O_BINARY)) == ERR) {
		fprintf(stderr,"cannot open temporary file (%d).\n", errno);
		return -1;
	}
#else
	/* serch new file name */
	for (n = 0; n < 100; n++) {
		strcpy(tmptmp, mvname);
		if ((p = rindex(tmptmp, '.')) != NULL) {
			strcpy(p, ".#01");
			p += 2;
		}
		else {
			strcat(tmptmp,".#01");
			p = rindex(tmptmp, '.') + 2;
		}
		sprintf(p, "%02d", n);
	    if (access(tmptmp, 0) != 0) break;
	}
	if (n >= 100) {
		fprintf(stderr,"cannot create temporary file.\n");
		return -1;
	}
	if ((fd = open(tmptmp, O_CREAT | O_WRONLY | O_BINARY, 0660)) == ERR) {
		fprintf(stderr,"cannot create temporary file (%d).\n", errno);
		return -1;
	}
#endif
#endif
	tmpchain *t2 = malloc(sizeof(tmpchain));
	strcpy(t2->target, mvname);
	strcpy(t2->tmpo, tmptmp);
	t2->next = NULL;

	if (root == NULL) root = t2;
	else {
		for (t = root; t->next != NULL; t = t->next);
		t->next = t2;
	}
	lseek(fd, fsize, SEEK_SET);
	write(fd, &cur_idb, sizeof(cur_idb));
	lseek(fd, v_offset, SEEK_SET);
#ifdef DISPISH
	int j;
	fprintf(stderr, "create:%s\n",t2->tmpo);
	fprintf(stderr, "create:seek=%ld\n",fsize);
	fprintf(stderr, "create:idb_size=%d\n",sizeof(cur_idb));
	for (j = 0; j < 4; j++) {
		fprintf(stderr, "create:seq[%d]=%x\n",j,cur_idb.sequence_number_table[j]);
	}
#endif
	delvol=0;
	return fd;
}

int mvol_ok(char *mvname,int n)
{
	int fd;
	int i,j,m,k;

	/* set done */
	--n;
	i = n >> 4;
	j = n & 0x0f;
	cur_idb.sequence_number_table[i] |= (1 << j);
	tmpchain *t = search_tmpo(mvname);
	if ((t == NULL) || ((fd = open(t->tmpo, O_RDWR | O_BINARY)) < 0)) {
		fprintf(stderr,"temporary file was broken.\n");
		return 0;
	}
//	long sk = lseek(fd, -sizeof(cur_idb), SEEK_END);
	write(fd, &cur_idb, sizeof(cur_idb));
	close(fd);
#ifdef DISPISH
	fprintf(stderr, "write:%s\n",t->tmpo);
	fprintf(stderr, "write:seek=%ld\n",sk);
	for(j = 0; j < 4; j++) {
		fprintf(stderr, "write:seq[%d]=%x\n",j,cur_idb.sequence_number_table[j]);
	}
#endif
	/* check all done */
	m = max_vol -1;
	i = m >> 4;
	for(j = 0; j < i; j++) {
		if (cur_idb.sequence_number_table[j] != 0x0f) return 0;
	}
	i = m & 0x0f;
	k = 0;
	for (m = 0; m <= i; m++) {
		k |= (1 << m);
	}
	if (cur_idb.sequence_number_table[j] != k) return 0;
	/* all dane */
	restore_done(mvname, fsize);
	return 1;
}

tmpchain *search_tmpo(char *mvname)
{
	tmpchain *t;
	int fd;
	struct IDBLOCK tmp;

	for (t = root; t != NULL; t = t->next) {
		if (strcmp(t->target, mvname) == 0) break;
	}
	if (t == NULL) return NULL;

	fd = open(t->tmpo, O_RDONLY | O_BINARY);
	if (fd < 0) {
#ifdef DISPISH
		fprintf(stderr,"fatal!! \"%s\" open error.",t->tmpo);
#endif
		return NULL;
	}
	if (lseek(fd, -sizeof(cur_idb), SEEK_END) == -1) {
#ifdef DISPISH
		fprintf(stderr,"fatal!! \"%s\" seek error.",t->tmpo);
#endif
		close(fd);
		return NULL;
	}
	read(fd, &tmp, sizeof(cur_idb));
	close(fd);
	if (strncmp(tmp.id_block_id, myID, MVIDLEN) != 0) {
#ifdef DISPISH
		int i;
		fprintf(stderr,"fatal!! \"%s\" ID error.",t->tmpo);
		fprintf(stderr,"file    ID = ");
		for (i = 0; i < MVIDLEN; ++i) {
			fprintf(stderr,"%02x ",tmp.id_block_id[i]);
		}
		fprintf(stderr,"correct ID = ");
		for (i = 0; i < MVIDLEN; ++i) {
			fprintf(stderr,"%02x ",myID[i]);
		}
		fprintf(stderr,"\n");
#endif
		return NULL;
	}
	if (bcmp(&tmp.header_copy, &cur_idb.header_copy, sizeof(HEADER_COPY)) != 0) {
#ifdef DISPISH
		fprintf(stderr,"fatal!! \"%s\" header_copy error.",t->tmpo);
#endif
		return NULL;
	}
	return t;
}

#define    COPY_BUF    1024

void restore_done(char *mvname,long sz)
{
	int     id;
	int     od;
	char    buf[COPY_BUF];
	int     l;
	ulong    crc32;
	ushort   crc16;
	ulong    ncrc32;
	ushort   ncrc16;
	uchar    *p;

	tmpchain *t = search_tmpo(mvname);
	if (t == NULL) {
		fprintf(stderr,"temporary file was broken.\n");
		return;
	}
	if (check_overwrite(mvname) != 0) return;

	char *tmpname = t->tmpo;
	fprintf(stderr, "%s CRC checking.\n", tmpname);
	p = cur_idb.header_copy.fcrc32;
	ncrc32 = (long)p[0] + ((long)p[1] << 8) + ((long)p[2] << 16) +
		((long)p[3] << 24);
	p = cur_idb.header_copy.fcrc16;
	ncrc16 = (long)p[1] + ((long)p[0] << 8);
	id = open(tmpname, O_RDONLY | O_BINARY);
	od = open(mvname, O_WRONLY | O_CREAT | O_BINARY , 0666);
	crc32 = 0xffffffff;
	crc16 = 0xffff;
	while (sz > 0) {
		if (sz < COPY_BUF) l = read(id, buf, sz);
		else l = read(id, buf, COPY_BUF);
		crc32 = calcrc32(buf, l, crc32);
		crc16 = calcrc(buf, l, crc16);
		write(od, buf, l);
		sz -= l;
	}
	close(id);
	close(od);
	crc32 = ~crc32;
	if (sizeof(long) > 4) {		// 64bit OS
		crc32 &= (ulong)0xffffffffL;
	}
	crc16 = ~crc16;
	if ((crc32 == ncrc32) && (crc16 == ncrc16)) {
		/* checksum OK */
		fprintf(stderr,"Checksum CRC-16 & CRC-32 OK.\n");
		fprintf(stderr, "all volume restored to \"%s\"\n", name);
	}
	else {
		fprintf(stderr,"Checksum error.\n");
		unlink(mvname);
	}
}
