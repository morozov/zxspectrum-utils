//////////////////////////////////////////////////////////////////////////////////////////////////
// 27.08.2018 - 05.09.2018 // Verzia 1.00 // Busy soft: Kontrola datovej konzistencie MBD imagu //
//////////////////////////////////////////////////////////////////////////////////////////////////

// #define DBG

#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef WIN32
#include <io.h>
#endif

int _CRT_glob = 0;

#define KONIEC {perror(meno);if(ff>0)fclose(ff);exit(1);}
#define RDFAT(sec) (fat1[sec << 1] | (fat1[(sec << 1) + 1 ] << 8))
#define RDWORD(ptr) (*(ptr) | (*((ptr) + 1) << 8))

unsigned char boot[1024];
unsigned char dirs[1024];
unsigned char subs[1024];
unsigned char fat1[4096];
unsigned char fat2[4096];
unsigned char flag[2048];

FILE *ff;
char *meno;
int media_size_log = 0;
int media_size_fyz = 0;
unsigned char *subent,inf,att,legend;
unsigned int aa,bb,cc,dd,ee,mm;
unsigned int total,used,ssystem,undefined;
unsigned int errors,crcerr,rnferr,ineerr;
unsigned int sfree,virgin,deleted,tim,dat;
unsigned long useful,unusable;
unsigned long zacmbd,lenmbd,wrkmbd,len;

unsigned int GetVal(char *ss)
{
	unsigned int digit;
	unsigned int value = 0;
	unsigned int base = 10;
	if (*ss == '#') { base = 16; ss++; }
	else if ((*ss == '0') && ((ss[1] == 'X') || (ss[1] == 'x'))) { base = 16; ss += 2; }
	while (*ss)
	{
		int digit = *ss;
		if (digit < '0') break;
		else if (digit < ':') digit -= '0';
		else if (base == 10) break;
		else if (digit < 'A') break;
		else if (digit < 'G') digit -= 'A' - 10;
		else if (digit < 'a') break;
		else if (digit < 'g') digit -= 'a' - 10;
		else break;
		value *= base;
		value += digit;
		ss++;
	}
	return value;
}

void getsec(unsigned long baseadd, unsigned int sektor, unsigned char *buffer)
{
	#ifdef DBG
	printf("  Getsec:%04X (%X)\n", sektor, baseadd + (unsigned long)sektor * 1024);
	#endif

	int b = -1;
	if (fseek(ff, baseadd + (unsigned long)sektor * 1024, SEEK_SET) < 0)
		{fprintf(stderr, "Seek error %s\n", meno); exit(1);}
	if ((b = fread(buffer, 1, 1024, ff)) < 1024)
		{fprintf(stderr, "Error reading %s (%d)\n", meno, b); exit(1);}
}

void textdisp(int dlzka, unsigned char *text)
{
	unsigned char znak;
	while (dlzka)
	{
		znak = *text;
		if (znak < 32 || znak>127) znak = '.';
		putchar(znak); dlzka--; text++;
	}
}

int checkfat(unsigned char *fat,  int num)
{
	int i;
	int zle = 0;
	unsigned char checksum = 0x00;
	for (i = 2; i < 0x1000; i++) checksum += fat[i];
	if (checksum != fat[1])
	{
		zle = 1;
		printf("  Error: Bad checksum of FAT %u: 0x%02X instead of expected 0x%02X\n",
			num, checksum, fat[1]);
	}

	for (i = 1; i < 0x800; i++) if (fat[i << 1] == 0xFF && fat[(i << 1) + 1] == 0xFF) break;
	if (i == 0x800)
	{
		zle = 1;
		printf("  Error: Missing end mark 0xFFFF in FAT %u\n", num);
	}

	for (; i < 0x800; i++) if (fat[i << 1] != 0xFF || fat[(i << 1) + 1] != 0xFF) break;
	if (i != 0x800)
	{
		printf("  Error: Incorrect value 0x%02X%02X in FAT %u at item 0x%03X\n",
			fat[(i << 1) + 1], fat[i << 1], num, i);
	}

	return zle;
}

void checkchain(char * object, unsigned int sector, int length)
{
	#ifdef DBG
	printf("Start check chain in %s  Sector: 0x%03X  Length=0x%04X\n", object, sector, length);
	#endif

	sector &= 0x3FFF;
	// if (!sector) return;
	if (sector < 0x0002) { printf("  Error: %s  Wrong first chain sector 0x%03X\n", object, sector); return; }
	if (sector > media_size_log) { printf("  Error: %s  First chain sector 0x%03X out of media\n", object, sector); return; }

	while (1)
	{
		if (flag[sector]) { printf("  Error: %s  Chain sector 0x%03X in cross\n", object, sector); break; }
		flag[sector] = 1;
		int nxtsec = RDFAT(sector);

		#ifdef DBG
		printf("Chain check: %s  FAT[%03X]=%04X  Rest length=%04X\n", object, sector, nxtsec, length);
		#endif

		if (nxtsec < 0x8000)
			{printf("  Error: %s  Chain sector FAT[%03X]=%03X not allocated\n", object, sector, nxtsec); break; }

		if ((nxtsec > 0xFEFF)||
			(nxtsec == 0xC000)||
			(nxtsec == 0xC001))
				{ printf("  Error: %s  Wrong chain sector FAT[%03X]=%03X\n", object, sector, nxtsec); break; }

		if (nxtsec > 0xC000 + media_size_log)
			{ printf("  Error: %s  Chain sector FAT[%03X]=%03X out of media\n", object, sector, nxtsec); break; }

		if (nxtsec < 0xC000)
		{
			if ((length > 0) && (nxtsec - 0x8000 != length))
				printf("  Error: %s  Bad chain end  FAT[%03X]=%04X  Len=0x%04X\n", object, sector, nxtsec, length);
			break;
		}
		int newlen = length - 0x400;
		if ((length > 0) && (newlen <= 0))
		{
			printf("  Error: %s  Incorrect length in chain FAT[%03X]=%04X  Len=%d\n", object, sector, nxtsec, length);
			newlen = 0;
		}
		length = newlen;
		sector = nxtsec & 0x3FFF;
	}
}

int main(int pocet, char **parametre)
{
	puts("\nBusy soft: MB-02 disk image checker 1.01");

	if (pocet < 2)
	{
		printf("\nUse: %s mb02imagefile [lenmbd [offset]] [> outputfile]\n"
			" You can check a set of disks in one big file.\n"
			"   lenmbd = length of image in case of more images in the one file\n"
			"   offset = offset of image in case of image is not at begin of file\n"
			" Both numbers can be decimal or hexadecimal with prefix '#' or '0x'\n"
			" All informations will be written to stdout,\n"
			" or output can be redirect to outputfile, if present.\n\n", parametre[0]);
		exit(1);
	}

	ff = NULL;
	meno = parametre[1];
	lenmbd = (pocet < 3) ? 0l : GetVal(parametre[2]);
	zacmbd = (pocet < 4) ? 0l : GetVal(parametre[3]);

	printf("File: %s  length: 0x%X %u  offset: 0x%X %u\n\n", meno, lenmbd, lenmbd, zacmbd, zacmbd);

	if (!lenmbd) lenmbd = 2000000000;

	ff = fopen(meno, "rb"); if (ff == NULL) KONIEC;
	#ifdef WIN32
	len = filelength(fileno(ff));  if (len < 0) KONIEC;
	#else
	fseek(ff, 0, SEEK_END);
	len = ftell(ff);               if (len < 0) KONIEC;
	fseek(ff, 0, SEEK_SET);
	#endif

	for (wrkmbd = zacmbd; wrkmbd + 1024 < len; wrkmbd += lenmbd)
	{
		memset(fat1, 0xFF, 0x1000);
		memset(fat2, 0xFF, 0x1000);
		memset(flag, 0x00, 2048);
		flag[0] = 1;
		flag[1] = 1;

		getsec(wrkmbd, 0, boot);

		printf("Tested image: ", wrkmbd);
		textdisp(0x0A, boot + 0x26); putchar(' ');
		textdisp(0x10, boot + 0x30); putchar('\n');

		if (boot[0x00] != 0x18) { printf("  Unknown format error: Byte 0x%02X instead of expected 0x18 at boot+00\n", boot[0x00]); continue; }
		if (boot[0x03] != 0x02) { printf("  Unknown format error: Byte 0x%02X instead of expected 0x02 at boot+03\n", boot[0x03]); continue; }
		if (boot[0x08] != 0x02) { printf("  Unknown format error: Byte 0x%02X instead of expected 0x02 at boot+08\n", boot[0x08]); continue; }
		if (boot[0x0A] != 0x01) { printf("  Unknown format error: Byte 0x%02X instead of expected 0x01 at boot+0A\n", boot[0x0A]); continue; }

		if (RDWORD(boot + 0x0E) << 10 != RDWORD(boot + 0x10))
		printf("  Error: Incorrect FAT length: Sectors: %u  Bytes: %u\n", RDWORD(boot + 0x0E), RDWORD(boot + 0x10));

		unsigned char chksum = 0x00;
		for (aa = 0x40; aa < 0x60; aa++) chksum ^= boot[aa];
		if (chksum != boot[0x16])
			printf("  Error: Bad checksum of disk ID: 0x%02X instead of expected 0x%02X at boot+0x16\n",
				boot[0x16], chksum);

		int i;
		int secfat = RDWORD(boot + 0x0E);
		if (secfat > 4) { printf("  Error: Too many FAT sectors: %u\n", secfat); continue; }

		int fat1sec = RDWORD(boot + 0x12);
		int fat2sec = RDWORD(boot + 0x14);
		getsec(wrkmbd, fat1sec, fat1);
		getsec(wrkmbd, fat2sec, fat2);
		for (i = 1; i < secfat; i++)
		{
			getsec(wrkmbd, boot[0x18 + (i<<1)], fat1 + (i<<10));
			getsec(wrkmbd, boot[0x19 + (i<<1)], fat2 + (i<<10));
		}

		if (memcmp(fat1, fat2, 0x1000))
			printf("  Error: FAT tables are different. FAT1: %02X %02X  FAT2: %02X %02X\n",
				fat1[0], fat1[1], fat2[0], fat2[1]);

		int zle1 = checkfat(fat1, 1);
		int zle2 = checkfat(fat2, 2);
		if (zle1 && !zle2) memcpy(fat1, fat2, 0x1000);

		for (i = 1; i < 0x800; i++) if (RDFAT(i) == 0xFFFF) break;
		media_size_log = i;
		media_size_fyz = RDWORD(boot + 0x04) * RDWORD(boot + 0x06) * RDWORD(boot + 0x08);
		#ifdef DBG
		printf("Physical size of disk: %u kB\n", media_size_fyz);
		printf("Logical  size of disk: %u kB\n", media_size_log);
		#endif
		if (media_size_fyz != media_size_log)
			printf("  Error: Inconsistent disk size. Physical: %u kB  Logical: %u kB\n",
				media_size_fyz, media_size_log);

		checkchain((char*)"FAT1", fat1sec, secfat << 10);
		checkchain((char*)"FAT2", fat2sec, secfat << 10);

		int dirsec = RDWORD(boot + 0x0C);
		if (dirsec > media_size_log) { printf("  Error: DIRS sector out of disk (boot[0x0C]=0x%04X)\n", dirsec); continue; }

		if (flag[dirsec]) printf("  Error: DIRS sector %03X in chain cross\n", dirsec);
		flag[dirsec] = 1;

		getsec(wrkmbd, dirsec, dirs);

		int dir;
		for (dir = 0; dir < 256; dir++)
		{
			char buffer[64];

			int offs = dir << 2;
			if ((dirs[offs] ^ dirs[offs + 3]) & 0xC0)
				printf("  Error: Directory %u  Integrity error in DIRS:  Id[0]=%02X  Id[3]=%02X\n", dir, dirs[offs], dirs[offs + 3]);

			if (!((dirs[offs]) & 0x80)) continue;

			#ifdef DBG
			printf("\nStart checking directory %u\n", dir);
			#endif

			int subsec = RDWORD(dirs + offs + 2) & 0x3FFF;
			if (subsec < 2) { printf("  Error: Directory %u  Incorrect first sector 0x%04X\n", dir, subsec); continue; }
			if (subsec > media_size_log) { printf("  Error: Directory %u  First sector 0x%04X out of disk\n", dir, subsec); continue; }

			sprintf(buffer, "Directory %u", dir);
			checkchain(buffer, subsec, -1);

			getsec(wrkmbd, subsec, subs);
			chksum = 0x00;
			for (i = 6; i < 16; i++) chksum ^= subs[i];
			if (chksum != dirs[offs + 1])
				printf("  Error: Bad checksum of directory %u name: 0x%02X instead of 0x%02X at DIRS+0x%02X.\n",
					dir, chksum, dirs[offs + 1], offs + 1);

			int secnum = 0;
			while (1)
			{
				int subnum;
				for (subnum = 0; subnum < 32; subnum++)
				{
					char name[12];
					int file_number = (secnum << 5) | subnum;
					if (!file_number) continue;
					unsigned char * subadd = subs + (subnum << 5);
					for (i = 0; i < 10; i++) { char znak = subadd[i + 6]; name[i] = (znak < 32) || (znak > 126) ? '.' : znak; }
					name[10] = 0; sprintf(buffer, "Dir: %u File: %u Name: %s", dir, file_number, name);

					if (!(*subadd & 0x80)) continue;

					#ifdef DBG
					printf("\nStart checking %s\n", buffer);
					#endif

					if (((subadd[0x00] ^ subadd[0x1D]) & 0x40) || ((subadd[0x00] ^ subadd[0x1F]) & 0x40))
						printf("  Error: %s  ID integrity error.  Id=%02X Att=%02X Sec=%02X\n",
							buffer, subadd[0x00], subadd[0x1D], subadd[0x1F]);

					if (!(*subadd & 0x60))
						printf("  Error: %s  File item without header and body\n", buffer);

					if (*subadd & 0x20)
					{
						int length = subadd[0x18] | (subadd[0x19] << 8) | (subadd[0x1A] << 16) | (subadd[0x1B] << 24);
						int sec1st = RDWORD(subadd + 0x1E) & 0x3FFF;
						if ((!length && sec1st) || (length && !sec1st))
							printf("  Error: %s  Length integrity error.  Length=%u  1st sector=0x%X\n",
								buffer, length, sec1st);

						if (sec1st) checkchain(buffer, sec1st, length);
					}
				}

				int nxtsec = RDFAT(subsec);
				#ifdef DBG
				printf("Next sector in directory %u: FAT[%03X]=0x%03X\n", dir, subsec, nxtsec);
				#endif
				if ((nxtsec > 0xC002) && (nxtsec < 0xC7FF))
				{
					subsec = nxtsec & 0x3FFF;
					if (dirsec > media_size_log) { printf("  Error: SUB sector out of disk. FAT[%03X]=0x%03X\n", subsec, nxtsec); break; }
					getsec(wrkmbd, subsec, subs);
					secnum++;
					continue;
				}
				else if ((nxtsec < 0x8002) || (nxtsec > 0x8400))
					printf("  Error: Incorrect chain of SUB sectors. FAT[%03X]=0x%03X\n", subsec, nxtsec);
				#ifdef DBG
				else
				printf("End checking directory %u\n", dir);
				#endif

				break;
			}
		}
	}

	if (fclose(ff)) perror(meno);

	int i, first = 0, last = 0;
	for (i = 0; i < 2048; i++) if ((fat1[(i << 1) + 1] >= 0x80) && (fat1[(i << 1) + 1] < 0xFF) && !flag[i])
	{
		last = i;
		if (!first) first = i;
	}
	if (last) printf("  Error: Lost (allocated but unused) sectors: %03X..%03X\n", first, last);

	#ifdef DBG
	putchar('\n');
	puts("  OK");
	#endif
	return 0;
}
