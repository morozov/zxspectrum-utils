//////////////////////////////////////////////////////////////////
// Busy // Create empty MBD image // Version 1.01 // 09.11.2022 //
//////////////////////////////////////////////////////////////////

#define DBG 0

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int _CRT_glob = 0;

#define LENIMG 0x200000

#ifndef BYTE
#define BYTE unsigned char
#endif

BYTE efekt[] = {
	0xAF,0xD3,0xFE,0x26,0x5B,0x77,0x2B,0xCB,0x5C,0x20,0xFA,0x2C,0x01,
	0x7E,0x03,0x3E,0x06,0x25,0x2D,0x71,0x20,0xFC,0x3D,0x20,0xF8,0x25,
	0x25,0x10,0xF2,0x44,0x4D,0x29,0x29,0x29,0x09,0x7C,0xE6,0x03,0xF6,
	0x58,0x67,0xED,0x5F,0xE6,0x7F,0x77,0x23,0x44,0x10,0xFE,0x18,0xE9};

#include "mbdsys.h"

BYTE image[LENIMG];

unsigned int GetVal(char *ss)
{
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

void WriteChainToFat(BYTE * fattab, int fatidx, int fatlen)
{
	while (fatlen > 0x400)
	{
		int nxtidx = fatidx + 1;
		fattab[(fatidx << 1) + 0] = nxtidx;
		fattab[(fatidx << 1) + 1] = 0xC0;
		fatidx = nxtidx;
		fatlen -= 0x400;
	}
	fattab[(fatidx << 1) + 0] = fatlen & 0xFF;
	fattab[(fatidx << 1) + 1] = (fatlen >> 8) | 0x80;
}

int main(int argc, char ** argv)
{
	puts("\nBusy soft: Create empty MB-02 disk image 1.01\n");

	if ((argc < 5) || (argc > 6))
	{
		printf("  Use:  %s  NewImageFile  NumTrk  NumSec  DiskName  <-s>\n\n"
			"DiskName can be up to 26 characters long.\n"
			"NumTrk = number of tracks  1..255\n"
			"NumSec = sectors per track 1..127\n"
			"-s = optional switch to make bootable image with system\n"
			"Both numbers can be decimal or hexadecimal with prefix '#' or '0x'\n", *argv);
		exit(1);
	}

	srand(clock());

	int i;
	char *file_name = argv[1];
	char *disk_name = argv[4];
	int numtrk = GetVal(argv[2]);
	int numsec = GetVal(argv[3]);
	int disk_size = (numtrk * numsec) << 1;

	int system_disk = (argc == 6 && !strcasecmp(argv[5], "-s"));

	if (DBG) printf("NumTrk:%u  NumSec:%u  DiskSize:%u kB%s\n", numtrk, numsec, disk_size, system_disk ? "  (bootable system disk)":"");

	if (!numtrk || (numtrk > 255)) { printf("Wrong number of tracks %u.\nNumTrk must be 1..255\n",numtrk); exit(1); }
	if (!numsec || (numsec > 127)) { printf("Wrong number of sectors %u.\nNumSec must be 1..127\n", numsec); exit(1); }
	if ((disk_size < 5) || (disk_size > 2048)) {printf("Wrong disk size %u kB.\nCapacity of disk (2 * NumTrk * NumSec) must be 6..2048 kB.\n", disk_size); exit(1); }
	if (system_disk && (numsec != 11)) { printf("Wrong sector per track %u. System disk must have 11 sectors per track.\n", numsec); exit(1); }
	if (system_disk && (disk_size < 154)) { printf("Wrong disk size %u kB.\nCapacity of system disk disk (2 * NumTrk * NumSec) must be 154..2046 kB.\n", disk_size); exit(1); }

	memset(image, 0x00, LENIMG);
	BYTE * boot = image;

	boot[0x00] = 0x18;	// MB02 disk identification and relative jump to system loader
	boot[0x01] = 0x5E;	// Argument for relative jump instruction
	boot[0x02] = 0x80;	// High byte of address of system loader (not used)
	boot[0x03] = 0x02;	// MB02 disk identification
	boot[0x04] = numtrk;// Total number of tracks
	boot[0x06] = numsec;// Clusters per track
	boot[0x08] = 0x02;	// Number of sides
	boot[0x0A] = 0x01;	// Sectors per cluster
	boot[0x0C] = 0x02;	// DIRS sector

	// Date & time of creation //
	time_t now;
	time(&now);
	struct tm *loctim = localtime(&now);
	if (DBG)
		printf("%02u.%02u.%04u  %02u:%02u:%02u ... %u\n",
			loctim->tm_mday,
			loctim->tm_mon + 1,
			loctim->tm_year + 1900,
			loctim->tm_hour,
			loctim->tm_min,
			loctim->tm_sec,
			(unsigned int)now);

	boot[0x21] = (loctim->tm_sec >> 1) | (loctim->tm_min << 5);
	boot[0x22] = (loctim->tm_min >> 3) | (loctim->tm_hour << 3);
	boot[0x23] = ((loctim->tm_mon+1) << 5) | loctim->tm_mday;
	boot[0x24] = ((loctim->tm_mon + 1) >> 3) | ((loctim->tm_year - 80) << 1);

	if (DBG)
	{
		unsigned int dat = boot[0x23] | (boot[0x24] << 8);
		unsigned int tim = boot[0x21] | (boot[0x22] << 8);
		printf("%02u.%02u.%04u  %02u:%02u:%02u\n",
			(dat & 0x1F), (dat & 0x1E0) >> 5, ((dat & 0xFE00) >> 9) + 1980,	// Date
			(tim & 0xF800) >> 11, (tim & 0x7E0) >> 5, (tim & 0x1F) << 1);	// Time
	}

	// Disk name //
	int size_name = strlen(disk_name);
	if (size_name > 0x1A) { size_name = 0x1A; disk_name[0x1A] = 0; }
	memset(boot + 0x26, 0x20, 0x1A);
	memcpy(boot + 0x26, disk_name, size_name);

	// Random disk identification ///
	unsigned char chksum = 0x00;
	for (i = 0; i < 0x20; i++) boot[0x40 + i] = boot[0x20 + i] ^ (boot[0x21 + (i & 0x03)]) ^ (rand() & 0xFF);
	for (i = 0; i < 0x20; i++) chksum ^= boot[0x40 + i];
	boot[0x16] = chksum;

	if (DBG) { printf("Disk ID: "); for (i = 0; i < 0x20; i++) printf("%02X", boot[0x40 + i]); putchar('\n');	}

	// FAT tables //
	BYTE * tabfat = image + 0xC00;				// Begin of FATs
	tabfat[0x03] = 0xFF;						// Backup sector
	tabfat[0x05] = 0x84;						// DIRS   sector
	int secfat = ((disk_size - 1) >> 9) + 1;	// FAT size in sectors
	int fatsiz = secfat << 10;					// FAT size in bytes
	boot[0x0E] = secfat;
	boot[0x11] = secfat << 2;
	int fat1sec = 3;							// FAT1 first sector
	int fat2sec = 3 + secfat;					// FAT2 first sector
	if (DBG) {printf("FAT Length: %u  FAT1 start: %u  FAT2 start: %u\n", secfat, fat1sec, fat2sec);}
	boot[0x12] = fat1sec;
	boot[0x14] = fat2sec;
	WriteChainToFat(tabfat, fat1sec, fatsiz);
	WriteChainToFat(tabfat, fat2sec, fatsiz);
	for (i = disk_size << 1; i < fatsiz; i ++) tabfat[i] = 0xFF;
	tabfat[fatsiz - 2] = 0xFF;
	tabfat[fatsiz - 1] = 0xFF;
	for (i = 1; i < secfat; i++)
	{
		boot[0x18 + (i << 1)] = fat1sec + i;
		boot[0x19 + (i << 1)] = fat2sec + i;
	}

	if (system_disk)
	{
		// Boot code and system dir + files //
		boot[0x01] = 0x6E;
		int subsec = 3 + (secfat << 1);
		int subofs = subsec << 10;
		tabfat[(subsec << 1) + 1] = 0x84;
		if (DBG) printf("Directory sector %u at 0x%02X\n", subsec, subofs);
		image[0x800] = 0x80;
		image[0x801] = 0x00;
		image[0x802] = subsec;
		image[0x803] = 0x80;
		image[subofs] = 0x80;
		memcpy(image + 0x60, boot_code, sizeof(boot_code));
		memcpy(image + subofs + 0x20, system_dir, sizeof(system_dir));
		memcpy(image + 0x20000, system_files, sizeof(system_files));
		WriteChainToFat(tabfat, 0x80, 0x800);  // Masterboot //
		WriteChainToFat(tabfat, 0x82, 0x5C21); // BSdos308   //
	}
	else
	{
		// Well known boot loader effect - blinking squares //
		memcpy(boot + 0x60, efekt, sizeof(efekt));
	}

	chksum = 0x00;
	for (i = 2; i < fatsiz; i++) chksum += tabfat[i];
	for (; i < 0x1000; i++) chksum--;
	tabfat[1] = chksum;
	memcpy(tabfat + fatsiz, tabfat, fatsiz);

	if (DBG) { printf("FAT:"); for (i = 0; i < 0x10; i++) printf(" %02X%02X", tabfat[(i << 1) + 1], tabfat[i << 1]); puts("\n"); }

	// Write image to output file //
	int disk_length = disk_size << 10;
	FILE * file = fopen(file_name, "wb");
	if (file == NULL) {perror(file_name); exit(1); }
	if (fwrite(image, 1, disk_length, file) < disk_length) fprintf(stderr, "Error writing %s\n", file_name);
	if (fclose(file)) perror(file_name);

	int free_space = 0;
	for (i = 3; i < fatsiz; i += 2) if (tabfat[i]<0x80) free_space++;

	printf("   System disk: %s\n", system_disk ? "Yes":"No");
	printf("  Name of disk: %.26s\n", boot + 0x26);
	printf("   Output file: %s\n", file_name);
	printf("      Geometry: %u tracks  %u sec/trk\n", numtrk, numsec);
	printf("    Image size: %u kB  %u bytes\n", disk_size, disk_length);
	printf("    Free space: %u kB  %u bytes\n", free_space, free_space << 10);
}
 
