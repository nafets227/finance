/***** fntxt2sql-aqb-tran.c *****/

#include "fntxt2sql.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure for Input from CSV-Files
typedef struct {
	char	ktonr[10+1];
	char	blz[8+1];
	char	part_ktonr[34+1];
	char	part_blz[11+1];
	char	name[1024+1];
	char	vzweck[12][140+1];
	char	betrag[20+1];
	char	waehrung[3+1];
	char	datum[10+1];
	char	valuta[10+1];
	//		char	gv_code[1024+1];
	//		char	primanota[1024+1];
} AqbBuchung;

static const char *SRC_ID(void)
{
	static char achID[256] = "";

	if(achID[0] == '\0')
		strcpy(achID, makeSourceDesc(
				"$RCSfile: fntxt2sql-aqb-tran.c,v $",
		"$Revision: 1.14 $)"));
	return achID;
}


//****************************************************************************
//***** Einen Satz einer CSV Datei parsen ************************************
//****************************************************************************
int parseAqb(char *pchBuffer, AqbBuchung *aqbBuchung)
{
	// jscpd:ignore-start
	int i, iSeps = 0, iAddVZweck = 0;
	char *pchTemp, *pchField, *pchSep;

	memset(aqbBuchung, '\0', sizeof(*aqbBuchung));

	if(convertCP(pchBuffer, "utf8", "latin1") != 0)
		return -1;

	pchTemp = pchBuffer;
	pchField = pchBuffer;
	for(i = 0; i < 29; i++)
	{
		while(*pchTemp != '\0' && *pchTemp != ';')
		{
			if(*pchTemp == '?')
				iSeps++;
			pchTemp++;
		}
		if(*pchTemp == '\0' && i < 29)
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
		case 0: // transactionId
		case 9: // localName
		case 11: // remoteName1
		case 24: // category
		case 25: // category2
		case 26: // category4
		case 27: // category5
		case 28: // category6
		case 29: // category7
			// Diese Felder werden ignoriert.
			break;
		case 1: // localBankCode
			debug_printf(dbg_fld, "BLZ: %s\n", pchField);
			strncpy(&aqbBuchung->blz[0], pchField, sizeof(aqbBuchung->blz)-1);
			break;
		case 2: // localAccountNumber
			debug_printf(dbg_fld, "KontoNr: %s\n", pchField);
			strncpy(&aqbBuchung->ktonr[0], pchField, sizeof(aqbBuchung->ktonr)-1);
			break;
		case 3: // remoteBankCode
			debug_printf(dbg_fld, "PartBLZ: %s\n", pchField);
			pchSep = strstr(pchField, "?31");
			if(pchSep != NULL) { // Special (erroneous) case: '?31' separates BLZ from ktonr.
				debug_printf(dbg_fld, "PartBLZ/Kto: %s\n", pchSep+3);
				strncpy(&aqbBuchung->part_ktonr[0], pchSep+3, sizeof(aqbBuchung->part_ktonr)-1);
			    *pchSep  = '\0';
			    iSeps--;
				}
			strncpy(&aqbBuchung->part_blz[0], pchField, sizeof(aqbBuchung->part_blz)-1);
			break;
		case 4: // remoteAccountNumber
			if(strlen(pchField) > 0)
				{
				debug_printf(dbg_fld, "PartKto: %s\n", pchField);
				pchSep = strstr(pchField, "?32");
				if(pchSep != NULL) // Special (erroneous) case: '?32' introduces "PartName"
					{
					debug_printf(dbg_fld, "PartKto/PartName: %s\n", pchSep+3);
					strncpy(&aqbBuchung->name[0], pchSep+3, sizeof(aqbBuchung->name)-2);
					strcat(&aqbBuchung->name[0], " ");
				    *pchSep  = '\0';
				    iSeps--;
					}
				strncpy(&aqbBuchung->part_ktonr[0], pchField, sizeof(aqbBuchung->part_ktonr)-1);
				}
			else
				debug_printf(dbg_fld, "PartKto (null): %s\n", aqbBuchung->part_ktonr);
			break;
		case 5: // date
			debug_printf(dbg_fld, "Datum: %s\n", pchField);
			strncpy(&aqbBuchung->datum[0], pchField, sizeof(aqbBuchung->datum)-1);
			break;
		case 6: // valutadate
			debug_printf(dbg_fld, "Valuta: %s\n", pchField);
			strncpy(&aqbBuchung->valuta[0], pchField, sizeof(aqbBuchung->valuta)-1);
			break;
		case 7: // value_value
			debug_printf(dbg_fld, "Betrag: %s\n", pchField);
			strncpy(&aqbBuchung->betrag[0], pchField, sizeof(aqbBuchung->betrag)-1);
			break;
		case 8: // value_currency
			debug_printf(dbg_fld, "Waehrung: %s\n", pchField);
			strncpy(&aqbBuchung->waehrung[0], pchField, sizeof(aqbBuchung->waehrung)-1);
			break;
		case 10: // remoteName
			debug_printf(dbg_fld, "Name: %s\n", pchField);
			pchSep = strstr(pchField, "?34");
			if(pchSep != NULL) // Special (erroneous) case: '?34' introduces "Textschluesselergaenzung"
				{
				debug_printf(dbg_fld, "Name/TxtSL: %s\n", pchSep+3);
			    *pchSep  = '\0';
			    iSeps--;
				}
			strncat(&aqbBuchung->name[0], pchField,
					sizeof(aqbBuchung->name)-1-strlen(aqbBuchung->name));
			break;
		case 12: // purpose
		case 13: // purpose1
		case 14: // purpose2
		case 15: // purpose3
		case 16: // purpose4
		case 17: // purpose5
		case 18: // purpose6
		case 19: // purpose7
		case 20: // purpose8
		case 21: // purpose9
		case 22: // purpose10
		case 23: // purpose11
			debug_printf(dbg_fld, "VZweck: %s\n", pchField);
			pchSep = strstr(pchField, "?");
			if(i+iAddVZweck-12 >= sizeof(aqbBuchung->vzweck)/sizeof(aqbBuchung->vzweck[0]))
				{
				if(*pchField != 0)
					fprintf(stderr, "Warning: VZweck(%d) \"%s\" ignored.\n", (int)i-12, pchField);
				}
			else
				{
				if(pchSep != NULL) // Erroneous field contains ?
					{
					*pchSep = '\0';
					strncpy(aqbBuchung->vzweck[i+iAddVZweck-12], pchField, sizeof(aqbBuchung->vzweck[0])-1);
					iAddVZweck++;
					pchSep += 3;
					while(*pchSep == ' ')
						pchSep++;
					strncpy(aqbBuchung->vzweck[i+iAddVZweck-12], pchSep, sizeof(aqbBuchung->vzweck[0])-1);
					iSeps--;
					}
				else
					strncpy(aqbBuchung->vzweck[i+iAddVZweck-12], pchField, sizeof(aqbBuchung->vzweck[0])-1);
				}
			break;
		default:
			fprintf(stderr, "parsing error: Invalid FieldNr %d\n", i);
			return -5;

		}

		pchTemp++;
		pchField = pchTemp;
	}

	if(iSeps != 0)
		{
		fprintf(stderr, "Error in field: %d Separators \'?\' not processed.\n", (int)iSeps);
		return -5;
		}

	return 0;
	// jscpd:ignore-end
}

//****************************************************************************
//***** Eine Buchung einer CSV Datei behandeln *******************************
//****************************************************************************
int processAqbBuchung(const AqbBuchung aqbBuchung)
{
	const char *pc = (const char*)0;
	int i = 0;
	int iRc;
	Buchung buchung;

	debug_printf(dbg_in, "processAqbBuchung.\n");

	memset(&buchung, '\0', sizeof(buchung));

	buchung.buchart = 'B';
	buchung.betrag = atof(aqbBuchung.betrag);
	strcpy(buchung.orig_ktonr,	aqbBuchung.ktonr);
	strcpy(buchung.orig_blz,	aqbBuchung.blz);
	strcpy(buchung.waehrung, 	aqbBuchung.waehrung);
	strcpy(buchung.datum, makeDatum(aqbBuchung.datum));
	strcpy(buchung.valuta, makeDatum(aqbBuchung.valuta));
	strcpy(buchung.part_name1,      aqbBuchung.name);
	strcpy(buchung.part_blz,        aqbBuchung.part_blz);
	strcpy(buchung.part_ktonr,      aqbBuchung.part_ktonr);

	for(i=0; i < sizeof(buchung.vzweck)/sizeof(buchung.vzweck[0]); i++)
	{
		pc = aqbBuchung.vzweck[i];
		if(strlen(pc) > sizeof(buchung.vzweck[0])-1)
			strncpy(buchung.vzweck[i], pc, sizeof(buchung.vzweck[0])-1);
		else
			strcpy(buchung.vzweck[i], pc);
	}

	strcpy(buchung.source, makeSourceId(SRC_ID()));
	debug_printf(dbg_fld, "Buchung %+9.2f %s %s %s %s\n",
			buchung.betrag, buchung.waehrung, buchung.datum,
			buchung.gv_code, buchung.primanota);

	iRc = writeRecord(buchung);

	return iRc;
}

//****************************************************************************
//***** Einen Satz einer CSV Datei behandeln *********************************
//****************************************************************************
int processAqbTranRecord(char * pchBuffer)
{
	int iRc = 0;
	AqbBuchung aqbBuchung;

	debug_printf(dbg_in, "processAqbRecord: %s\n", pchBuffer);
	debug_printf(dbg_in, "\n");

	if(!strncmp(pchBuffer, "\"transactionId\"", 15))
	{
		debug_printf(dbg_in, "Ignoring Header-Line\n");
		return 0;
	}

	iRc = parseAqb(pchBuffer, &aqbBuchung);
	if(iRc != 0)
		return iRc;

	iRc = processAqbBuchung(aqbBuchung);
	if(iRc != 0)
		return iRc;

	return 0;
}

