//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Verzia 02 // 09.03.2009 - 24.08.2018 // MBD Id Fix = Put random ID into MB2 image(s) file(s) // Busy //
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//#include <io.h>
//#include <dos.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KONIEC {perror(meno);if(ff>0)fclose(ff);exit(1);}
#define BootHHxor 0x16
#define BootIdent 0x40
#define BootPasss 0x50
#define BootXXend 0x60

unsigned char nahoda[8];
unsigned char bootbuffer[0x60];

unsigned long gethex(char *ss)
{
	unsigned digit;
	unsigned long value = 0;
	while (*ss)
	{
		digit = toupper(*ss);
		if (digit < '0') break;
		if (digit > 'F') break;
		if (digit < ':') digit -= '0';
		else
		{
			if (digit < 'A') break;
			digit -= 'A' - ':';
		}
		value <<= 4;
		value |= digit & 0x0F;
		ss++;
	}
	return value;
}

unsigned long getnum(char *ss)
{
	int len = strlen(ss);
	if (!len) return 0l;
	if (*ss == '#') return gethex(ss + 1);
	if ((len > 2) && (ss[0] == '0') && (tolower(ss[1]) == 'x')) return gethex(ss + 2);
	return atol(ss);
}

void main(int argc, char **argv)
{
	srand(clock()); rand();

	puts("\nBusy: MBD-Id-Fix 02");
	if (argc < 2)
	{
		puts("Sets random disk ID for MBD disk image file\n"
			"Use: MbdIdFix filename.mbd [lenmbd [offset]]\n"
			"  length = length of pure image in case of more images in the file.mbd\n"
			"  offset = offset of image in case of image is not at begin of file.mbd\n"
			"Numbers can be decimal or hexadecimal with prefix '#' or '0x'");
		exit(1);
	}

	char * meno = argv[1];
	int lenmbd = (argc < 3) ? 0l : getnum(argv[2]);
	int zacmbd = (argc < 4) ? 0l : getnum(argv[3]);

	printf("File: %s  length: 0x%X %u  offset: 0x%X %u\n", meno, lenmbd, lenmbd, zacmbd, zacmbd);

	if (!lenmbd) lenmbd = 2000000000l;

	FILE * ff = fopen(meno, "rb+"); if (ff == NULL) KONIEC
	fseek(ff, 0L, SEEK_END);
	long len = ftell(ff);        if (len < 0) KONIEC

	int cislo1 = time(NULL);
	int cislo2 = clock();
	int cislo3 = 0x55555555 ^ rand() ^ (rand() << 16);   rand();
	int cislo4 = 0xAAAAAAAA ^ rand() ^ (rand() << 16);

	nahoda[0] = 0xFF & ((cislo1 >>  0) ^ (cislo3 >> 16));
	nahoda[1] = 0xFF & ((cislo1 >>  8) ^ (cislo3 >> 24));
	nahoda[2] = 0xFF & ((cislo1 >> 16) ^ (cislo3 >>  0));
	nahoda[3] = 0xFF & ((cislo1 >> 24) ^ (cislo3 >>  8));
	nahoda[4] = 0xFF & ((cislo2 >> 24) ^ (cislo4 >> 16));
	nahoda[5] = 0xFF & ((cislo2 >> 16) ^ (cislo4 >>  0));
	nahoda[6] = 0xFF & ((cislo2 >>  8) ^ (cislo4 >> 24));
	nahoda[7] = 0xFF & ((cislo2 >>  0) ^ (cislo4 >>  8));

	int cntimg = 0;
	int aa;
	int wrkmbd;
	for (wrkmbd = zacmbd; wrkmbd + 1024 < len; wrkmbd += lenmbd)
	{
		cntimg++;
		int ee = fseek(ff, wrkmbd, SEEK_SET); if (ee < 0) KONIEC
		size_t see = fread(bootbuffer, 1, 0x60, ff); if (see == 0) KONIEC
		printf("%8X: Image %u: ", wrkmbd, cntimg);
		if (see < 0x60) { puts("not completed"); break; }
		if ((bootbuffer[0x00] != 0x18)
			|| (bootbuffer[0x03] != 0x02)
			|| (bootbuffer[0x08] != 0x02)
			|| (bootbuffer[0x0A] != 0x01)) {
			puts("=> Unknown format"); continue;
		}
		printf("%02ux%02u \"", bootbuffer[0x04], bootbuffer[0x06]);
		for (aa = 0x26; aa < 0x30; aa++)
		{
			unsigned char bb = bootbuffer[aa];
			putchar(((bb > 31) && (bb < 128)) ? bb : '.');
		}
		printf("\" ");
		unsigned char bb = bootbuffer[BootHHxor];
		for (aa = BootIdent; aa < BootXXend; aa++) bb ^= bootbuffer[aa];
		if (bb) { puts("=> Data integrity error"); continue; }
		////////////////////////////////////////////////////
		for (aa = 0x00; aa < 0x08; aa++)
		{
			bootbuffer[aa + BootIdent + 0] ^= nahoda[aa];
			bootbuffer[aa + BootIdent + 8] ^= nahoda[aa];
			bootbuffer[aa + BootPasss + 0] ^= nahoda[aa];
			bootbuffer[aa + BootPasss + 8] ^= nahoda[aa];
		}
		////////////////////////////////////////////////////
		ee = fseek(ff, wrkmbd, SEEK_SET); if (ee < 0) KONIEC
		see = fwrite(bootbuffer, 1, 0x60, ff); if (see == 0) { puts("=> write error"); KONIEC }
		if (see < 0x60) { puts("=> not complete written"); break; }
		puts("=> Fixed");
		nahoda[0] = 5 * nahoda[0] + 7;
		nahoda[1] = 9 * nahoda[1] + 13;
		nahoda[2] ^= 0xFF & rand();
		nahoda[3] ^= 0xFF & rand();
		nahoda[4] ^= 0xFF & rand();
	}

	int ee = fclose(ff);
	if (ee) perror(meno); else puts("All done.");
}

