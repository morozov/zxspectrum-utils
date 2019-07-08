/////////////////////////////////////////////////////////////////////////////
// 16.03.2010 - 24.08.2018 // Busy soft: Zmena roku 1980-1999 => 2000-2019 //
/////////////////////////////////////////////////////////////////////////////

#define DBG

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
unsigned char menodisk[10];
unsigned char menodirs[10];

FILE *ff;
int zmena,rok;
char *meno;
unsigned char *subent,inf,att,legend;
unsigned int aa,bb,cc,dd,ee,tim,dat;
unsigned long zacmbd,lenmbd,wrkmbd;
unsigned long celkovo,zmenene;
long len;

int __gxx_personality_v0;

unsigned gethex(char *ss)
{
	unsigned digit;
	unsigned value = 0;
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

unsigned getnum(char *ss)
{
	int len = strlen(ss);
	if (!len) return 0l;
	if (*ss == '#') return gethex(ss + 1);
	if ((len > 2) && (ss[0] == '0') && (tolower(ss[1]) == 'x')) return gethex(ss + 2);
	return atol(ss);
}

void getsec(unsigned baseadd, unsigned int sektor, unsigned char *buffer)
{
	#ifdef DBG
	printf(" Getsec:%X:%04X ", baseadd, sektor);
	#endif

	zmena = 0;

	if (fseek(ff, baseadd + (sektor << 10), SEEK_SET) < 0)
		{fprintf(stderr, "Seek error %s\n", meno); exit(1);}
	if (fread(buffer, 1, 1024, ff) < 1024)
		{fprintf(stderr, "Error reading %s\n", meno); exit(1);}
}

void putsec(unsigned baseadd, unsigned int sektor, unsigned char *buffer)
{
	if (!zmena) return;
	zmena = 0;

	#ifdef DBG
	printf(" Putsec:%X:%04X ", baseadd, sektor);
	#endif

	if (fseek(ff, baseadd + (sektor << 10), SEEK_SET) < 0)
		{fprintf(stderr, "Seek error %s\n", meno); exit(1);}
	if (fwrite(buffer, 1, 1024, ff) < 1024)
		{fprintf(stderr, "Error writing %s\n", meno); exit(1);}
}

void textdisp(unsigned char *text)
{
	char znak;
	int dlzka = 10;
	while (dlzka)
	{
		znak = *text;
		if ((!znak) || (znak == 7) || (znak == 9) || (znak == 10) || (znak == 13)) znak = '.';
		putchar(znak); dlzka--; text++;
	}
}

void datedisp(unsigned char * timdat)
{
	dat = timdat[2] | (timdat[3] << 8);
	tim = timdat[0] | (timdat[1] << 8);

	printf("  %02d.%02d.%04d %02d:%02d:%02d",
		(dat & 0x1F), (dat & 0x1E0) >> 5, ((dat & 0xFE00) >> 9) + 1980,	// Datum
		(tim & 0xF800) >> 11, (tim & 0x7E0) >> 5, (tim & 0x1F) << 1);	// Cas
}

int correction(unsigned char * kdecas)
{
	/* int den, mes; */

	celkovo++;

	if (!kdecas[3] && !kdecas[2]) return 0;
	if ((kdecas[3]) < 32)
	{
		zmena = 1;
		zmenene++;
		kdecas[3] += 40;
		return ((kdecas[3]) >> 1) + 1980;
	}
	return 0;
}

int main(int pocet, char **parametre)
{
	unsigned char * tmp;

	puts("\nBusy soft: MB-02 Year-bug fixer 1.01");

	if (pocet < 2)
	{
		puts("\nUse: MbRokFix mb02imagefile [lenMBD [offset]] [> outputfile]\n"
			" Fix bad year format in all dates on disk(s).\n"
			"   lenMBD = length of image in case of more images in the one file\n"
			"   offset = offset of image in case of image is not at begin of file\n"
			" Both numbers can be decimal or hexadecimal with prefix '#' or '0x'\n");
		exit(1);
	}

	ff = NULL;
	meno = parametre[1];
	lenmbd = (pocet < 3) ? 0 : getnum(parametre[2]);
	zacmbd = (pocet < 4) ? 0 : getnum(parametre[3]);

	printf("File: %s  lenmbd: 0x%lX %lu  offset: 0x%lX %lu\n\n", meno, lenmbd, lenmbd, zacmbd, zacmbd);

	if (!lenmbd) lenmbd = 2000000000;

	ff = fopen(meno, "rb+"); if (ff == NULL) KONIEC
	fseek(ff, 0L, SEEK_END);
	len = ftell(ff);           if (len < 0) KONIEC

	celkovo = 0;
	zmenene = 0;

	for (wrkmbd = zacmbd; wrkmbd + 1024 < len; wrkmbd += lenmbd)
	{
		memset(fats, 255, 4096);
		getsec(wrkmbd, 0, boot);
		memcpy(menodisk, boot + 0x26, 10);

		if ((boot[0x00] != 0x18)
			|| (boot[0x03] != 0x02)
			|| (boot[0x08] != 0x02)
			|| (boot[0x0A] != 0x01))
		{
			printf("%8lX:", wrkmbd);
			textdisp(menodisk);
			puts(" => Unknown format !!!");
			continue;
		}

		bb = boot[BootHHxor];
		for (aa = BootIdent; aa < BootXXend; aa++) bb ^= boot[aa];
		if (bb)
		{
			printf("%8lX:", wrkmbd);
			textdisp(menodisk);
			puts(" => Data integrity error !!!");
			continue;
		}

		// Boot sektor //
		printf("%8lX:", wrkmbd);
		textdisp(menodisk);
		datedisp(boot + 0x21);
		rok = correction(boot + 0x21);
		if (rok) printf(" => %d", rok);
		putchar('\n');
		putsec(wrkmbd, 0, boot);

		tmp = boot + 0x12;
		aa = tmp[0] | (tmp[1]<<8) | 0xC000;
		for (bb = 0; (aa >= 0xC000) && (bb < 4096); bb += 1024)
		{
			getsec(wrkmbd, aa & 0x3FFF, fats + bb);
			#ifdef DBG
			printf(" Fat[%04X]", aa);
			#endif
			tmp = fats + 2 * (aa & 0x3FFF);
			aa = tmp[0] | (tmp[1]<<8);
			#ifdef DBG
			printf("=%04X ", aa);
			#endif
		}
		#ifdef DBG
		putchar('\n');
		#endif

		tmp = boot + 0x0C;
		getsec(wrkmbd, tmp[0] | (tmp[1] << 8), dirs);

		for (aa = 0; aa < 256; aa++) if ((dirs[4 * aa]) & 0x80)
		{
			legend = 1;
			bb = 0;
			tmp = dirs + 4 * aa + 2;
			cc = tmp[0] | (tmp[1]<<8) | 0xC000;
			getsec(wrkmbd, cc & 0x3FFF, subs);
			memcpy(menodirs, subs + 0x06, 10);

			while ((cc >= 0xC000) && (bb < 0xFF00))
			{
				getsec(wrkmbd, cc & 0x3FFF, subs);
				#ifdef DBG
				putchar('\n');
				#endif
				for (dd = 0; dd < 1024; dd += 32)
				{
					subent = subs + dd;
					printf("%8lX:", wrkmbd);
					textdisp(menodisk);
					printf("%4u:", aa);
					textdisp(menodirs);
					printf("%6u:", bb);
					textdisp(subent + 6);
					datedisp(subent + 0x01);
					rok = correction(subent + 0x01);
					if (rok) printf(" => %d", rok);
					putchar('\n');
					bb++;
				}
				putsec(wrkmbd, cc & 0x3FFF, subs);
				#ifdef DBG
				printf("\n Fat[%04X]", cc);
				#endif
				tmp = fats + 2 * (cc & 0x3FFF);
				cc = tmp[0] | (tmp[1]<<8);
				#ifdef DBG
				printf("=%04X ", cc);
				#endif
			}
		}
		putchar('\n');
	}
	if (!fclose(ff)) perror(meno);
	printf("\nTotal: %lu  Corrected: %lu\n", celkovo, zmenene);
	return 0;
}

