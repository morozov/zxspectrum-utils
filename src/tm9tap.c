/////////////////////////////////////////////////////////////////////////
// 26.05.2022 // tm9tap // Busy soft: TapeManager09 => TAP konvertor 1 //
/////////////////////////////////////////////////////////////////////////

//#include<io.h>
#include<ctype.h>
#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string.h>

#define lenbuf 65538
unsigned char buffer[lenbuf];
char * inpfile;
char * outfile;
FILE * inphandle = NULL;
FILE * outhandle = NULL;

void readdata(unsigned char * address, int length)
{
	size_t readed = fread((void*)address, 1, length, inphandle);
	if (readed == length) return;
	if (outhandle) fclose(outhandle);
	printf("\nError reading %s (%zu from %d at %ld)\n", inpfile, readed, length, ftell(inphandle));
	exit(1);
}

int main(int argc, char *argv[])
{
	int cnt;
	puts("\nBusy soft: TapeManager09 => TAP format convertor 1\n");
	if (argc != 3)
	{
		puts(
			"  Usage: Tm9tap input_tm9_file output_tap_file\n"
			"  Converts memory image from TapeManager09 into TAP file"
		); exit(-1);
	}

	inpfile = argv[1];
	outfile = argv[2];

	inphandle = fopen(inpfile, "rb"); if (!inphandle) { perror(inpfile); exit(1); }
	outhandle = fopen(outfile, "wb"); if (!outhandle) { perror(outfile); exit(1); }

	printf("      Input file: %s\n", inpfile);
	printf("     Output file: %s\n", outfile);

	readdata(buffer, 2);
	int number_of_banks = buffer[0] + 1;
	int number_of_files = buffer[1];

	printf(" Number of banks: %u\n", number_of_banks);
	printf(" Number of files: %u\n", number_of_files);

	putchar('\n');
	int total_size = 0;

	for (cnt = 1; cnt <= number_of_files; cnt++)
	{
		if (fread(buffer, 1, 6, inphandle) < 6) break;

		int memo_size = buffer[0] | (buffer[1] << 8);
		int file_size = buffer[2] | (buffer[3] << 8);
		int info_byte = buffer[4];
		int checksum = buffer[5];

		total_size += file_size;

		printf("Cnt:%-3u  Mem:%-5u  Size:%-5u  Beep:%s  Wait:%-3u  Parity:%-3u  ",
			cnt,
			memo_size,
			file_size - 2,
			info_byte & 0x80 ? "Yes" : "No ",
			info_byte & 0x0F,
			checksum);

		int rest = file_size;

		buffer[0] = file_size & 0xFF;
		buffer[1] = (file_size >> 8) & 0xFF;
		unsigned char * load = buffer + 2;

		while (rest > 0)
		{
			unsigned char control_byte = 0;
			readdata(&control_byte, 1);
			int len = control_byte & 0x7F;
			if (!len) len = 0x80;
			if (control_byte & 0x80)
			{
				// Packed the same bytes
				unsigned char data_byte = 0x55;
				readdata(&data_byte, 1);
				memset(load, data_byte, len);
			}
			else
			{
				// Non-packed block
				readdata(load, len);
			}
			load += len;
			rest -= len;
		}
		if (file_size != load - buffer - 2) { printf("Internal error (%d %ld)\n", file_size, load - buffer - 2); exit(1); }

		int writed = fwrite(buffer, 1, file_size + 2, outhandle);
		if (writed < file_size + 2) { printf("Write error %s (Write %u bytes only)\n", outfile, writed); exit(1); }

		puts("Ok");
	}
	putchar('\n');

	printf("Written %u files and total %u bytes into tap file.\n", cnt - 1, total_size);
	
	fclose(inphandle);
	fclose(outhandle);
	return 0;
}
