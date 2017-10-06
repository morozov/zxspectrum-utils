/*
* This file is part of LSTRD.
*
* Copyright (C) 2003 Pavel Cejka <pavel.cejka at kapsa.club.cz>
*
* Modifications (C) 2015 ub880d <ub880d@users.sf.net>
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

#define SEC_SIZE 256
#define TRK_SIZE 16
#define INFO_SEC 8

/* TRDOS INFO SECTOR */
#define OFFSET_FIRSTSECTOR	225		/* První volný sektor pouľitelný k uloľení souboru. */
#define OFFSET_FIRSTTRACK	226		/* První volný track pouľitelný k uloľení souboru. */
#define OFFSET_DISKFORMAT	227		/* Formát 22=80tr/DS, 23=40tr/DS, 24=80tr/SS, 25=40tr/SS */
#define OFFSET_FILES   		228		/* Počet souborů včetně smazaných, které nebyly poslední. */
#define OFFSET_FREESECTORS	229		/* Počet volných sektorů - 2 byty */
#define OFFSET_TRDOSIDENT	231		/* TRDOS identifikace, musí být vľdy 16 */
#define OFFSET_PASSWORD		234		/* OBSOLETE!! Zastaralé, pouľíval TRDOS 3.x a 4.x */
#define OFFSET_DELETEDFILES	244		/* Počet smazaných souborů (těch co nebyly poslední...) */
#define OFFSET_DISKNAME		245		/* Novějąí verze TRDOSu z Brna pouľívá 10 znaků, starąí jen 8 */

typedef struct {
	unsigned char byte[SEC_SIZE];
} trdos_sector;
	
typedef struct {
	trdos_sector sector[TRK_SIZE];
} trdos_track;

