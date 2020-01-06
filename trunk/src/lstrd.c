/*
* This file is part of LSTRD.
*
* Copyright (C) 2003 Pavel Cejka <pavel.cejka at kapsa.club.cz>
*
* Modifications (C) 2015 ub880d <ub880d@users.sf.net> (mainly for pedantic compile)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at
* your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
* USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "trdos_structure.h"

#define VERSION			"1.0"

int lstrdos (char *filename, int listmode)
{
	unsigned char directory [9*SEC_SIZE];
	int inputfile;
	const char *trdosfiletype;
	char *trdosfilename;
	char *trdosdiskname;
	char *trdospassword;
	int len1, len2;
	int freesectors;
	int i;
/*	char *diskformat; //not used yet */
	int trdosfile;
	
	inputfile=open (filename,0);
	if (!inputfile)
	{	
		printf ("Error: Can't open input file %s.\n",filename);
		close (inputfile);
		return (-1);
	}
	
	
	if (read (inputfile, directory,9*SEC_SIZE)==9*SEC_SIZE)
	{	
		/* soubor otevøen a naèten v pamìti, teï by nemìlo být mo¾né, aby nastala chyba */
		trdosdiskname=(char*)malloc (11);
		strncpy(trdosdiskname, directory + INFO_SEC*SEC_SIZE + OFFSET_DISKNAME, 10);
		trdosdiskname[10]='\0';

		trdospassword=(char*)malloc (10);
		strncpy(trdospassword, directory + INFO_SEC*SEC_SIZE + OFFSET_PASSWORD, 9);
		trdospassword[9]='\0';
		
		freesectors=directory[INFO_SEC*SEC_SIZE + OFFSET_FREESECTORS]+256*directory[INFO_SEC*SEC_SIZE + OFFSET_FREESECTORS+1];
		trdosfilename=(char*)malloc (9);
			
		/* Výpis */
		switch (listmode)
		{
			case 2:
				/* 
				   re¾im EXTRA SHORT (haven't equivalent in real TRDOS, but may be usefull for
				   *nix operating systems for batch operations with files
				*/
				for (trdosfile=0; trdosfile<directory[INFO_SEC*SEC_SIZE + OFFSET_FILES]; trdosfile++)
				{
					/* jméno souboru */
					strncpy(trdosfilename, directory+16*trdosfile, 8);
					trdosfilename[8]='\0';
					printf ("%s.%c\n",trdosfilename,directory[trdosfile*16+8]);
				}
				break;
			case 1:
				/* re¾im CAT */
				printf ("Title:  %s\n",trdosdiskname);
				printf ("%i File(s)\n",directory[INFO_SEC*SEC_SIZE + OFFSET_FILES]);
				printf ("%i Deleted File(s)\n\n",directory[INFO_SEC*SEC_SIZE + OFFSET_DELETEDFILES]);

				for (trdosfile=0; trdosfile<directory[INFO_SEC*SEC_SIZE + OFFSET_FILES]; trdosfile++)
				{
					/* jméno souboru */
					strncpy(trdosfilename, directory+16*trdosfile, 8);
					trdosfilename[8]='\0';
					printf ("%s <%c> %3d",trdosfilename,directory[trdosfile*16+8], /* délka v sektorech */ directory[trdosfile*16+13]);
					
					if ((trdosfile%2)==1) printf ("\n");
					else printf ("  ");
				}

				if ((trdosfile%2)==1) printf ("\n");
				printf ("\n%i Free\n",directory[INFO_SEC*SEC_SIZE + OFFSET_FREESECTORS]+256*directory[INFO_SEC*SEC_SIZE + OFFSET_FREESECTORS+1]);
				break;
			case 0:
			default:
				/* re¾im LIST */
				printf ("Diskname: %s\n",trdosdiskname);
    				printf ("Password: %s\n",trdospassword);
				switch (directory[INFO_SEC*SEC_SIZE + OFFSET_DISKFORMAT])
				{
		    			case 22:
						printf("80 Tracks, Double Side, capacity 640kB\n");
		    				break;
		    			case 23:
	    		    			printf("40 Tracks, Double Side, capacity 320kB\n");
		    				break;
		    			case 24:
						printf("80 Tracks, Single Side, capacity 320kB\n");
		    				break;
		    			case 25:
						printf("40 Tracks, Single Side, capacity 160kB\n");
		    				break;
					default:
						printf("UNKNOWN FORMAT!\n");
						break;
				}
				printf ("Number of files/deleted: %i/%i\n", directory[INFO_SEC*SEC_SIZE + OFFSET_FILES], directory[INFO_SEC*SEC_SIZE + OFFSET_DELETEDFILES]);
				printf ("Free sectors/bytes:      %i/%i\n", freesectors, freesectors*256);
				printf ("First free sector/track: %i/%i\n\n", directory[INFO_SEC*SEC_SIZE + OFFSET_FIRSTSECTOR], directory[INFO_SEC*SEC_SIZE + OFFSET_FIRSTTRACK]);
				
				printf ("FILENAME      TYPE         SECTORS ADDRESS LENGTH TRACK SECTOR \n");
				printf ("--------------------------------------------------------------\n");
				for (trdosfile=0; trdosfile<directory[INFO_SEC*SEC_SIZE + OFFSET_FILES]; trdosfile++)
				{
					strncpy(trdosfilename, directory+16*trdosfile, 8);
					trdosfilename[8]='\0';
					switch (directory[trdosfile*16+8])
					{
						case 'b':
						case 'B':
							trdosfiletype = "BASIC PROGRAM";
							break;
						case 'c':
						case 'C':
							trdosfiletype = "CODE (BYTES)";
							break;
						case 'd':
						case 'D':
							trdosfiletype = "DATA";
							break;
						case 'F':
							trdosfiletype = "VPL ANIMATION";
							break;
						case 'G':
							trdosfiletype = "CYGNUS PLUGIN";
							break;
						case 'M':
							trdosfiletype = "SOUNDTRACKER";
							break;
						case '#':
							trdosfiletype = "STREAM";
							break;
						default:
							trdosfiletype = "UNKNOWN";
							break;
					}
					switch (directory[trdosfile*16+8])
					{
						case 'b':
						case 'B':
							len1 = /* délka basicu */ directory[trdosfile*16+11]+256*directory[trdosfile*16+12];
							len2 = /* délka basicu bez promìnných */ directory[trdosfile*16+9]+256*directory[trdosfile*16+10];
							break;
						default:
							len1 = /* adresa kam se soubor nahraje do RAM, není-li explicitnì urèeno jinak */
								directory[trdosfile*16+9]+256*directory[trdosfile*16+10];
							len2 = /* délka v bytech */ directory[trdosfile*16+11]+256*directory[trdosfile*16+12];
					}
					printf ("%s <%c>  %-13s%5i  %6i%7i   %3i    %3i\n"
							, trdosfilename			/* jméno souboru */
							, directory[trdosfile*16+8]	/* typ souboru */
							, trdosfiletype			/* typ souboru */
							, directory[trdosfile*16+13]	/* délka v sektorech */
							, len1				/* délka basicu / adresa */
							, len2				/* délka basicu bez promìnných / délka v bytech */
							, directory[trdosfile*16+15]	/* první stopa zabraná souborem */
							, directory[trdosfile*16+14]	/* první sektor zabraný souborem */
						);
				}
				break;
		}
		free(trdosdiskname);
		free(trdospassword);
		free(trdosfilename);
	}
	else
	{
		printf ("Error: Reading from file %s failed.\n",filename);
		close (inputfile);
		return (-1);
	}
	close (inputfile);
	return (0);
}

void version ()
{
	printf ("%s \n", VERSION);
}

void help ()
{
	printf ("LSTRD is a special utility for examine TR-DOS image files.\n");
	printf ("Usage: lstrd [OPTIONS]... [FILENAME]...\n");
	printf ("If the filename missing or any option is incorrect, this help will printed.\n");
	printf ("  -c, --cat     use a short listing format, same as command CAT in original TRDOS\n");
	printf ("  -h, --help    display this help and exit\n");
	printf ("  -l, --list    use a long listing format, similar to command LIST in original TRDOS\n");
	printf ("  -v, --version    version\n");
}

int main(int argc, char *argv[])
{
	int counter;
	int listmode=0;
	char *filename;
	int switches;
	int correct;

	/* vstupní soubor je to co není jiným parametrem, výstupní soubor není */
	/* printf ("DEBUG: argc - %i\n", argc); */

	switches=1;
	for (counter=1; counter<argc; counter++)
	{
/*		printf ("++ %i ++ %s \n",counter, argv[counter]); */
		correct=0;
		
		if (!strcmp(argv[counter],"-h") || (!strcmp(argv[counter],"--help")))
		{
			help ();
			switches++;
			correct=1;
			return (0);
		}
		if (!strcmp(argv[counter],"-v") || (!strcmp(argv[counter],"--version")))
		{
			version ();
			switches++;
			correct=1;
			return (0);
		}
		if (!strcmp(argv[counter],"-l") || (!strcmp(argv[counter],"--list")))
		{
			listmode=0;
			switches++;
			correct=1;
		}
		if (!strcmp(argv[counter],"-c") || (!strcmp(argv[counter],"--cat")))
		{
			listmode=1;
			switches++;
			correct=1;
		}
		if (!strcmp(argv[counter],"-s") || (!strcmp(argv[counter],"--short")))
		{
			listmode=2;
			switches++;
			correct=1;
		}
		if ((correct==0) && (counter!=(argc-1)))
		{
			/*
			   nepochopitelný parametr je tolerován jen jako poslední, to toti¾ musí být
			   jméno souboru, jestli je platné, to se uká¾e pozdìji, pokud by byl nepochopitelný 
			   parametr nalezen døíve ne¾ na posledním místì, bude zobrazena nápovìda
			*/
			printf ("ERROR: incorrect parametr - \"%s\"\n", argv[counter]);
			printf ("Hint: Filename must be last parameter.\n");
			help ();
			return (-1);
		}
	}

	filename=argv[argc-1];

	/* printf ("DEBUG: switches - %i \n", switches); */

	if (switches!=argc-1)
	{
		printf ("ERROR: Filename missing.\n");
		return (-1);
	}
	else
	{
		lstrdos (filename, listmode);
		return (0);
	}
}
