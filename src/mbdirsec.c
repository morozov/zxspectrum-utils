/////////////////////////////////////////////////////////////////////////
// 25.03.2000 - 24.08.2018 // Busy soft: Vypis adresarov MB-02 diskety //
/////////////////////////////////////////////////////////////////////////

// #define DBG

//#include<io.h>
#include<stdio.h>
#include<fcntl.h>
#include<ctype.h>
//#include<conio.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>

#define KONIEC {perror(meno);if(ff!=NULL)fclose(ff);exit(1);}

#define BootHHxor 0x16
#define BootIdent 0x40
#define BootPasss 0x50
#define BootXXend 0x60

unsigned char boot[1024];
unsigned char dirs[1024];
unsigned char subs[1024];
unsigned char fats[4096];

FILE *ff;
char *meno;
unsigned char *subent,inf,att,legend;
unsigned int aa,bb,cc,dd,ee,mm;
unsigned int total,used,ssystem,undefined;
unsigned int errors,crcerr,rnferr,ineerr;
unsigned int sfree,virgin,deleted,tim,dat;
unsigned long useful,unusable;
unsigned long zacmbd,lenmbd,wrkmbd;
long len;

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

void getsec(unsigned long baseadd, unsigned int sektor, unsigned char *buffer)
{
	#ifdef DBG
	printf(" Getsec:%04X ", sektor);
	#endif

	if (fseek(ff, baseadd + (unsigned long)sektor * 1024, SEEK_SET) < 0)
		{fprintf(stderr, "Seek error %s\n", meno); exit(1);}
	if (fread(buffer, 1, 1024, ff) < 1024)
		{fprintf(stderr, "Error reading %s\n", meno); exit(1);}
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

void chain(unsigned int sector)
{
	sector |= 0xC000;
	if (sector & 0x3FFF) while (1)
	{
		int secnxt = sector & 0x3FFF;
		printf(" %03X", secnxt);
		if (sector < 0xC000 || sector >= 0xFF00) { printf(" (error)"); break; }
		sector = fats[secnxt << 1] | (fats[(secnxt << 1) + 1] << 8);
		#ifdef DBG
		printf(":%04X ", sector);
		#endif
		if (sector < 0xC000) break;
	}
	putchar('\n');
}

int main(int pocet, char **parametre)
{
	puts("\nBusy soft: MB-02 disk image analyser 1.04");

	if (pocet < 2)
	{
		printf("\nUse: %s mb02imagefile [lenmbd [offset]] [> outputfile]\n"
			" You can display a set of disks in one big file.\n"
			"   lenmbd = length of image in case of more images in the one file\n"
			"   offset = offset of image in case of image is not at begin of file\n"
			" Both numbers can be decimal or hexadecimal with prefix '#' or '0x'\n"
			" All informations will be written to stdout,\n"
			" or output can be redirect to outputfile, if present.\n\n", parametre[0]);
		exit(1);
	}

	ff = NULL;
	meno = parametre[1];
	lenmbd = (pocet < 3) ? 0l : getnum(parametre[2]);
	zacmbd = (pocet < 4) ? 0l : getnum(parametre[3]);

	printf("File: %s  length: 0x%lX %lu  offset: 0x%lX %lu\n", meno, lenmbd, lenmbd, zacmbd, zacmbd);

	if (!lenmbd) lenmbd = 2000000000l;

	ff = fopen(meno, "rb"); if (ff == NULL) KONIEC
	fseek(ff, 0L, SEEK_END);
	len = ftell(ff);               if (len < 0) KONIEC

		for (wrkmbd = zacmbd; wrkmbd + 1024 < len; wrkmbd += lenmbd)
		{
			memset(fats, 255, 4096);

			getsec(wrkmbd, 0, boot);

			printf("\n\nDisk: "); textdisp(0x0A, boot + 0x26);
			putchar(' ');         textdisp(0x10, boot + 0x30);
			printf("    Position in file: 0x%lX %lu\n", wrkmbd, wrkmbd);

			if ((boot[0x00] != 0x18)
				|| (boot[0x03] != 0x02)
				|| (boot[0x08] != 0x02)
				|| (boot[0x0A] != 0x01)) {
				puts("Data: Unknown format !!!"); continue;
			}

			bb = boot[BootHHxor];
			for (aa = BootIdent; aa < BootXXend; aa++) bb ^= boot[aa];
			if (bb) { puts("Data: Data integrity error !!!"); continue; }

			dat = boot[0x23] | (boot[0x24] << 8);
			tim = boot[0x21] | (boot[0x22] << 8);

			printf("Time: %02d.%02d.%04d %02d:%02d:%02d\n"
				"Data: %u tracks, %u sec/track, %u surfaces,"
				" %u sec/cluster, %u sec/FAT\n\n",
				(dat & 0x1F), (dat & 0x1E0) >> 5, ((dat & 0xFE00) >> 9) + 1980,	// Datum
				(tim & 0xF800) >> 11, (tim & 0x7E0) >> 5, (tim & 0x1F) << 1,	// Cas
				boot[0x04] | (boot[0x05] << 8),
				boot[0x06] | (boot[0x07] << 8),
				boot[0x08] | (boot[0x09] << 8),
				boot[0x0A] | (boot[0x0B] << 8),
				boot[0x0E] | (boot[0x0F] << 8));

			aa = boot[0x12] | (boot[0x13] << 8) | 0xC000;
			for (bb = 0; (aa >= 0xC000) && (bb < 4096); bb += 1024)
			{
				getsec(wrkmbd, aa & 0x3FFF, fats + bb);
				#ifdef DBG
				printf(" Fat[%04X]", aa);
				#endif
				aa = fats[2 * (aa & 0x3FFF)] | (fats[2 * (aa & 0x3FFF) + 1] << 8);
				#ifdef DBG
				printf("=%04X ", aa);
				#endif
			}
			#ifdef DBG
			putchar('\n');
			putchar('\n');
			#endif

			printf("  FAT 1 sectors: "); chain(boot[0x12] | (boot[0x13] << 8));
			printf("  FAT 2 sectors: "); chain(boot[0x14] | (boot[0x15] << 8));

			total = 1; used = 0; sfree = 0; ssystem = 1; undefined = 0;
			errors = 0; crcerr = 0; rnferr = 0; ineerr = 0; virgin = 0; deleted = 0;
			useful = 0; unusable = 0;
			for (aa = 2; aa < 4096; aa += 2)
			{
				bb = fats[aa] | (fats[aa + 1] << 8);
				if (bb >= 0xFFFE) continue;
				total++;
				if (fats[aa + 1] == 0xFF)
				{
					bb = fats[aa];
					if (!bb) ssystem++; else
						if (bb < 0xF0) undefined++; else
						{
							switch (bb)
							{
							case 0xFC: crcerr++; break;
							case 0xFD: rnferr++; break;
							default: ineerr++;
							}
							errors++;
						}
					continue;
				}
				if (!bb) { virgin++; sfree++; }
				else
					if (bb < 0x8000) { deleted++; sfree++; }
					else
						if (bb >= 0xC000) { used++; useful += 1024; }
						else
						{
							used++;
							useful += bb - 0x8000;
							unusable += 0x8400 - bb;
						}
			}
			puts(
				"\n  Capacity              kB         bytes"
				"\n  ********              **         *****");
			printf("  Total ............ %5u    %10u\n", total, total * 1024);
			printf("  System ........... %5u    %10u\n", ssystem, ssystem * 1024);
			printf("  Undefined ........ %5u    %10u\n", undefined, undefined * 1024);
			printf("  Errors ........... %5u    %10u\n", errors, errors * 1024);
			printf("    CRC errors ..... %5u    %10u\n", crcerr, crcerr * 1024);
			printf("    RNF errors ..... %5u    %10u\n", rnferr, rnferr * 1024);
			printf("    Other errros ... %5u    %10u\n", rnferr, rnferr * 1024);
			printf("  Free ............. %5u    %10u\n", sfree, sfree * 1024);
			printf("    Virgin ......... %5u    %10u\n", virgin, virgin * 1024);
			printf("    Deleted ........ %5u    %10u\n", deleted, deleted * 1024);
			printf("  Used ............. %5u    %10u\n", used, used * 1024);
			printf("    Useful ......... %5lu.%02lu %10lu\n", useful / 1024, (useful & 0x3FF) * 100 / 1024, useful);
			printf("    Unusable ....... %5lu.%02lu %10lu\n", unusable / 1024, (unusable & 0x3FF) * 100 / 1024, unusable);

			#ifdef DBG
			putchar('\n');
			#endif
			getsec(wrkmbd, boot[0x0C] | (boot[0x0D]<<8), dirs);
			#ifdef DBG
			putchar('\n');
			#endif
			puts("\n Directories");
			for (aa = 0; aa < 256; aa++) if ((dirs[4 * aa]) & 0x80)
			{
				getsec(wrkmbd, dirs[(aa << 2) + 2] | ((dirs[(aa << 2) + 3] << 8) & 0x3FFF), subs);
				dat = subs[0x03] | (subs[0x04] << 8);
				tim = subs[0x01] | (subs[0x02] << 8);
				printf("    %3u: ", aa);
				textdisp(0x0A, subs + 0x06);
				putchar(' ');
				textdisp(0x10, subs + 0x10);
				printf("  %02d.%02d.%04d %02d:%02d:%02d\n",
					(dat & 0x1F), (dat & 0x1E0) >> 5, ((dat & 0xFE00) >> 9) + 1980,	// Datum
					(tim & 0xF800) >> 11, (tim & 0x7E0) >> 5, (tim & 0x1F) << 1);	// Cas
			}

			for (aa = 0; aa < 256; aa++) if ((dirs[4 * aa]) & 0x80)
			{
				#ifdef DBG
				putchar('\n');
				#endif
				legend = 1;
				bb = 0;
				cc = dirs[(aa << 2) + 2] | (dirs[(aa << 2) + 3] << 8) | 0xC000;
				getsec(wrkmbd, cc & 0x3FFF, subs);
				dat = subs[0x03] | (subs[0x04] << 8);
				tim = subs[0x01] | (subs[0x02] << 8);
				printf("\n Directory: %u   Updir: %u   Sectors: ", aa, subs[0x05]);
				chain(cc);
				printf("      Name: ");
				textdisp(0x0A, subs + 0x06); putchar(' ');
				textdisp(0x10, subs + 0x10); putchar('\n');
				printf("      Time: %02d.%02d.%04d %02d:%02d:%02d\n",
					(dat & 0x1F), (dat & 0x1E0) >> 5, ((dat & 0xFE00) >> 9) + 1980,	// Datum
					(tim & 0xF800) >> 11, (tim & 0x7E0) >> 5, (tim & 0x1F) << 1);	// Cas

				while ((cc >= 0xC000) && (bb < 0xFF00))
				{
					getsec(wrkmbd, cc & 0x3FFF, subs);
					#ifdef DBG
					putchar('\n');
					#endif
					for (dd = 0; dd < 1024; dd += 32)
					{
						subent = subs + dd;
						if (*subent > 0x80)
						{
							if (legend)
							{
								legend = 0;
								puts("\n   Number:What  T:Name......:Len..:Add..:Bas..  DD.MM.YYYY HH:MM:SS  Atribute:Adres:Length     Sectors");
							}
							inf = *subent & 0x30;
							dat = subent[0x03] | (subent[0x04] << 8);
							tim = subent[0x01] | (subent[0x02] << 8);

							printf("    %5u:%s  %u:",
								bb,				// Info bajt
								inf == 0x30 ? "file" :
								inf == 0x20 ? "body" :
								inf == 0x10 ? "head" : "file",
								subent[0x05]);			// Hlavicka typ

							textdisp(0x0A, subent + 0x06);		// Hlavicka meno

							printf(":%05u:%05u:%05u  %02d.%02d.%04d %02d:%02d:%02d %c ",
								subent[0x10] | (subent[0x11] << 8),	// Hlavicka dlzka
								subent[0x12] | (subent[0x13] << 8),	// Hlavicka adresa
								subent[0x14] | (subent[0x15] << 8),	// Hlavicka basic
								(dat & 0x1F), (dat & 0x1E0) >> 5, ((dat & 0xFE00) >> 9) + 1980,	// Datum
								(tim & 0xF800) >> 11, (tim & 0x7E0) >> 5, (tim & 0x1F) << 1,	// Cas

								att = subent[0x1D]);			// Atributy
							for (ee = 0; ee < 8; ee++) { putchar(((att >> 7) & 0x01) | 0x30); att <<= 1; }

							printf(":%05u:%-10u",
								subent[0x16] | (subent[0x16] << 8),
								subent[0x18] | (subent[0x19] << 8) | (subent[0x1A] << 16) | (subent[0x1B] << 24));

							chain(subent[0x1E] | (subent[0x1F] << 8));
						}
						bb++;
					}
					#ifdef DBG
					printf("\n Fat[%04X]", cc);
					#endif
					cc = fats[(cc & 0x3FFF) << 1] | (fats[((cc & 0x3FFF) << 1) + 1] << 8);
					#ifdef DBG
					printf("=%04X ", cc);
					#endif
				}
			}
		}
	#ifdef DBG
	putchar('\n');
	#endif
	if (!fclose(ff)) perror(meno);
}
