/***** fntxt2sql-aqm.c *****/

#include "fntxt2sql.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure for Input from CSV-Files
typedef struct {
	char	ktonr[10+1];
	char	blz[8+1];
	char	name[1024+1];
	char	vzweck[1024+1];
	char	betrag[20+1];
	char	waehrung[3+1];
	char	datum[10+1];
	char	gv_code[1024+1];
	char	primanota[1024+1];
} AqmBuchung;

static const char *SRC_ID(void)
{
	static char achID[256] = "";

	if(achID[0] == '\0')
		strcpy(achID, makeSourceDesc(
				"$RCSfile: fntxt2sql-aqm.c,v $",
		"$Revision: 1.8 $)"));
	return achID;
}

//****************************************************************************
//***** Einen Satz einer CSV Datei parsen ************************************
//****************************************************************************
int parseAqm(char *pchBuffer, AqmBuchung *aqmBuchung)
{
	int i;
	char *pchTemp, *pchField;

	pchTemp = pchBuffer;
	pchField = pchBuffer;
	for(i = 0; i < 9; i++)
	{
		while(*pchTemp != '\0' && *pchTemp != ';')
		{
			pchTemp++;
		}
		if(*pchTemp == '\0' && i < 8)
		{
			fprintf(stderr, "Parsing error: Record too short\n");
			return -5;
		}

		*pchTemp = '\0';

		// Hochkommas entfernen
		if(*pchField == '\"' && *(pchField + strlen(pchField) - 1) == '\"')
		{
			pchField++;
			*(pchField + strlen(pchField) - 1) = '\0';
		}
		switch(i)
		{
		case 0:
			debug_printf(dbg_fld, "BLZ: %s\n", pchField);
			strcpy(&aqmBuchung->blz[0], pchField);
			break;
		case 1:
			debug_printf(dbg_fld, "KontoNr: %s\n", pchField);
			strcpy(&aqmBuchung->ktonr[0], pchField);
			break;
		case 2:
			// Im ersten Satz ist aufgrund eines Bugs in AQMoney
			// immer noch ein Hochkomma am Anfang des Namensfelds.
			// Deshalb hier die spezielle Abfrage:
			if(*pchField == '\"')
				pchField++;	

			debug_printf(dbg_fld, "Name: %s\n", pchField);
			strcpy(&aqmBuchung->name[0], pchField);
			break;
		case 3:
			debug_printf(dbg_fld, "VZweck: %s\n", pchField);
			strcpy(&aqmBuchung->vzweck[0], pchField);
			break;
		case 4:
			debug_printf(dbg_fld, "Betrag: %s\n", pchField);
			strcpy(&aqmBuchung->betrag[0], pchField);
			break;
		case 5:
			debug_printf(dbg_fld, "Waehrung: %s\n", pchField);
			strcpy(&aqmBuchung->waehrung[0], pchField);
			break;
		case 6:
			debug_printf(dbg_fld, "Datum: %s\n", pchField);
			strcpy(&aqmBuchung->datum[0], pchField);
			break;
		case 7:
			debug_printf(dbg_fld, "GV-Code: %s\n", pchField);
			strcpy(&aqmBuchung->gv_code[0], pchField);
			break;
		case 8:
			debug_printf(dbg_fld, "Primanota: %s\n", pchField);
			strcpy(&aqmBuchung->primanota[0], pchField);
			break;
		default:
			fprintf(stderr, "parsing error: Invalid FieldNr %d\n", i);
			return -5;

		}

		pchTemp++;
		pchField = pchTemp;
	}

	return 0;
}

//****************************************************************************
//***** Eine Buchung einer CSV Datei behandeln *******************************
//****************************************************************************
int processAqmBuchung(const AqmBuchung aqmBuchung)
{
	char *pc = (char*)aqmBuchung.vzweck;
	int i = 0;
	Buchung buchung;

	debug_printf(dbg_in, "processAqmBuchung.\n");

	memset(&buchung, '\0', sizeof(buchung));
	strcpy(buchung.source, makeSourceId(SRC_ID()));


	buchung.buchart = 'B';
	buchung.betrag = makeBetrag(aqmBuchung.betrag);
	strcpy(buchung.orig_ktonr,	aqmBuchung.ktonr);
	strcpy(buchung.orig_blz,	aqmBuchung.blz);
	strcpy(buchung.waehrung, 	aqmBuchung.waehrung);
	strcpy(buchung.datum, makePktDatum(aqmBuchung.datum));
	strcpy(buchung.valuta, makePktDatum(aqmBuchung.datum));
	strcpy(buchung.part_name1, aqmBuchung.name);
	strcpy(buchung.butext, aqmBuchung.gv_code);
	strcpy(buchung.primanota, aqmBuchung.primanota);

	while(	strlen(pc) > 0 &&
			i < sizeof(buchung.vzweck)/sizeof(buchung.vzweck[0]))
	{
		if(strlen(pc) > 27)
		{
			strncpy(buchung.vzweck[i], pc, 27);
			pc += 27;
		}
		else
		{
			strcpy(buchung.vzweck[i], pc);
			pc += strlen(pc);
		}
		i++;
	}

	debug_printf(dbg_fld, "Buchung %+9.2f %s %s %s %s\n",
			buchung.betrag, buchung.waehrung, buchung.datum,
			buchung.gv_code, buchung.primanota);

	writeRecord(buchung);

	return 0;
}

//****************************************************************************
//***** Einen Satz einer CSV Datei behandeln *********************************
//****************************************************************************
int processAqmRecord(char * pchBuffer)
{
	int iRc = 0;
	AqmBuchung aqmBuchung;

	debug_printf(dbg_in, "processAqmRecord: %s\n", pchBuffer);
	debug_printf(dbg_in, "\n");

	if(!strncmp(pchBuffer+20, "\"Name\"", 6))
	{
		debug_printf(dbg_in, "Ignoring Header-Line\n");
		return 0;
	}

	iRc = parseAqm(pchBuffer, &aqmBuchung);
	if(iRc != 0)
		return iRc;

	iRc = processAqmBuchung(aqmBuchung);
	if(iRc != 0)
		return iRc;

	return 0;
}

