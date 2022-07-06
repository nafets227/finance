/***** fntxt2sql-csv.c *****/

#include "fntxt2sql.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure for Input from CSV-Files
typedef struct {
	char	ktonr[10+1];
	char	blz[8+1];
	char	auszDatum[10+1];
	char    saldo[20+1];
	char	saldoWaehrung[3+1];
	char	buDatum[10+1];
	char	valuta[10+1];
	char	primanota[4+1];
	char	vzweck[1024+1];
	char	betrag[20+1];
	char	waehrung[3+1];
} CsvBuchung;

static const char *SRC_ID(void)
{
	static char achID[256] = "";

	if(achID[0] == '\0')
		strcpy(achID, makeSourceDesc(
				"$RCSfile: fntxt2sql-csv.c,v $",
		"$Revision: 1.10 $)"));
	return achID;
}


//****************************************************************************
//***** Einen Satz einer CSV Datei parsen ************************************
//****************************************************************************
int parseCsv(char *pchBuffer, CsvBuchung *csvBuchung)
{
	int i;
	char *pchTemp, *pchField;

	pchTemp = pchBuffer;
	pchField = pchBuffer;
	for(i = 0; i < 11; i++)
	{
		while(*pchTemp != '\0' && *pchTemp != ';')
		{
			if(*pchTemp == '\"')
			{
				for(pchTemp++; *pchTemp != '\"'; pchTemp++)
					if(*pchTemp == '\0')
					{
						fprintf(stderr, "Parsing error 1\n");
						return -5;
					}
			}
			pchTemp++;
		}
		if(*pchTemp == '\0' && i < 10)
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
			debug_printf(dbg_fld, "KontoNr: %s\n", pchField);
			strcpy(&csvBuchung->ktonr[0], pchField);
			break;
		case 1:
			debug_printf(dbg_fld, "BLZ: %s\n", pchField);
			strcpy(&csvBuchung->blz[0], pchField);
			break;
		case 2:
			debug_printf(dbg_fld, "Auszugsdatum: %s\n", pchField);
			strcpy(&csvBuchung->auszDatum[0], pchField);
			break;
		case 3:
			debug_printf(dbg_fld, "Saldo: %s\n", pchField);
			strcpy(&csvBuchung->saldo[0], pchField);
			break;
		case 4:
			debug_printf(dbg_fld, "Waehrung: %s\n", pchField);
			strcpy(&csvBuchung->saldoWaehrung[0], pchField);
			break;
		case 5:
			debug_printf(dbg_fld, "Buchungsdatum: %s\n", pchField);
			strcpy(&csvBuchung->buDatum[0], pchField);
			break;
		case 6:
			debug_printf(dbg_fld, "Valuta-Datum: %s\n", pchField);
			strcpy(&csvBuchung->valuta[0], pchField);
			break;
		case 7:
			debug_printf(dbg_fld, "PrimaNota: %s\n", pchField);
			strcpy(&csvBuchung->primanota[0], pchField);
			break;
		case 8:
			debug_printf(dbg_fld, "Buchungtstext: %s\n", pchField);
			strcpy(&csvBuchung->vzweck[0], pchField);
			break;
		case 9:
			debug_printf(dbg_fld, "Betrag: %s\n", pchField);
			strcpy(&csvBuchung->betrag[0], pchField);
			break;
		case 10:
			debug_printf(dbg_fld, "Waehrung: %s (len=%d)\n", pchField, (int)strlen(pchField));
			strcpy(&csvBuchung->waehrung[0], pchField);
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
//***** Einen Saldo einer CSV Datei behandeln ********************************
//****************************************************************************
int processCsvSaldo(const CsvBuchung csvBuchung)
{
	Buchung buchung;
	int iRc;

	memset(&buchung, '\0', sizeof(buchung));

	debug_printf(dbg_in,"processCsvSaldo: %s %s %s\n", csvBuchung.auszDatum,
			csvBuchung.saldo, csvBuchung.saldoWaehrung);

	buchung.buchart = 'E';
	buchung.betrag = makeBetrag(csvBuchung.saldo);
	strcpy(buchung.orig_ktonr,	csvBuchung.ktonr);
	strcpy(buchung.orig_blz,	csvBuchung.blz);
	strcpy(buchung.waehrung, 	csvBuchung.saldoWaehrung);
	strcpy(buchung.datum, makeDatum(csvBuchung.auszDatum));
	strcpy(buchung.source, makeSourceId(SRC_ID()));

	debug_printf(dbg_fld, "Schlussssaldo %+9.2f %s %s\n",
			buchung.betrag, buchung.waehrung, buchung.datum);

	iRc = writeRecord(buchung);

	return iRc;
}

//****************************************************************************
//***** Den Buchungstext einer Buchung verarbeiten ***************************
//****************************************************************************
int processCsvText(const char * pchVZweck, Buchung *pBuchung)
{
	int i = 0;

	static const struct _confFixText
	{ char * pchVZweck; char *pchPrimaNota; } confFixText[] = {
			{"DAUERAUFTR ", "333"},
			{"LASTSCHR. " , "222"},
			{"AUTOM.AUSZ ", "111"},
			{"ABSCHLUSS " , "915"},
	};

	debug_printf(dbg_in, "processCsvText(\"%40s\")\n", pchVZweck);

	for(i = 0; i < sizeof(confFixText) / sizeof(confFixText[0]); i++)
		if (!strcmp(pBuchung->primanota, confFixText[i].pchPrimaNota) &&
				!memcmp(pchVZweck, confFixText[i].pchVZweck,
						strlen(confFixText[i].pchVZweck)))
			// Primanota and VZweck fit to vonfFixText[i]
			pchVZweck += strlen(confFixText[i].pchVZweck);

	for(i= 0;
	strlen(pchVZweck) > 0 &&
	i < sizeof(pBuchung->vzweck)/sizeof(pBuchung->vzweck[0]);
	i++)
	{
		if(i==1 && *pchVZweck == ' ')  pchVZweck++;
		if(strlen(pchVZweck) > 27)
		{
			strncpy(pBuchung->vzweck[i], pchVZweck, 27);
			pchVZweck += 27;
		}
		else
		{
			strcpy(pBuchung->vzweck[i], pchVZweck);
			pchVZweck += strlen(pchVZweck);
		}
	}

	return 0;
}


//****************************************************************************
//***** Eine Buchung einer CSV Datei behandeln *******************************
//****************************************************************************
int processCsvBuchung(const CsvBuchung csvBuchung)
{
	Buchung buchung;
	int iRc;

	debug_printf(dbg_in, "processCsvBuchung.\n");

	memset(&buchung, '\0', sizeof(buchung));

	buchung.buchart = 'B';
	buchung.betrag = makeBetrag(csvBuchung.betrag);
	strcpy(buchung.orig_ktonr,	csvBuchung.ktonr);
	strcpy(buchung.orig_blz,	csvBuchung.blz);
	strcpy(buchung.waehrung, 	csvBuchung.waehrung);
	strcpy(buchung.datum, makeDatum(csvBuchung.buDatum));
	strcpy(buchung.valuta, makeDatum(csvBuchung.valuta));
	strcpy(buchung.primanota, csvBuchung.primanota);
	strcpy(buchung.source, makeSourceId(SRC_ID()));

	processCsvText(csvBuchung.vzweck, &buchung);

	debug_printf(dbg_fld, "Buchung %+9.2f %s %s %s\n",
			buchung.betrag, buchung.waehrung, buchung.datum, buchung.vzweck[0]);

	iRc = writeRecord(buchung);

	return iRc;
}

//****************************************************************************
//***** Einen Satz einer CSV Datei behandeln *********************************
//****************************************************************************
int processCsvRecord(char * pchBuffer)
{
	int iRc = 0;
	static char achAuszDatumOld[10+1] = "          ";
	CsvBuchung csvBuchung;

	debug_printf(dbg_in, "processCsvRecord: %s\n", pchBuffer);
	debug_printf(dbg_in, "\n");

	if(!strncmp(pchBuffer, "Konto", 5))
	{
		debug_printf(dbg_in, "Ignoring Header-Line\n");
		return 0;
	}

	iRc = parseCsv(pchBuffer, &csvBuchung);
	if(iRc != 0)
		return iRc;

	if(strcmp(csvBuchung.auszDatum, achAuszDatumOld))
	{
		iRc = processCsvSaldo(csvBuchung);
		if(iRc != 0)
			return iRc;
		strcpy(achAuszDatumOld, csvBuchung.auszDatum);
	}

	iRc = processCsvBuchung(csvBuchung);
	if(iRc != 0)
		return iRc;

	return 0;
}

