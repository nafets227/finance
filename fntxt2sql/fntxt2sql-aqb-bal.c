/***** fntxt2sql-aqb-bal.c *****/

#include "fntxt2sql.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int RC_EMPTYLINE = 100;

static const char *SRC_ID(void)
{
	static char achID[256] = "";

	if(achID[0] == '\0')
		strcpy(achID, makeSourceDesc(
				"$RCSfile: fntxt2sql-aqb-bal.c,v $",
		"$Revision: 1.2 $)"));
	return achID;
}


//****************************************************************************
//***** Einen Satz einer Aqb-Bal Datei parsen ********************************
//****************************************************************************
static int parseAqbBal(char *pchBuffer, Buchung *pBuchung)
{
	int i;
	char *pchTemp, *pchField;

	// Initialize pBuchung
	memset(pBuchung, '\0', sizeof(*pBuchung));
	pBuchung->buchart = 'E';
	strncpy(pBuchung->source, makeSourceId(SRC_ID()), sizeof(pBuchung->source)-1);

	// Codepage Conversion
	if(convertCP(pchBuffer, "utf8", "latin1") != 0)
		return -1;

	// Now process fields
	pchTemp = pchBuffer;
	pchField = pchBuffer;
	for(i = 0; i < 13; i++)
	{
		while(*pchTemp != '\0' && *pchTemp != '\t')
			pchTemp++;

		if(*pchTemp == '\0' && i < 13)
		{
			fprintf(stderr, "Parsing error: Record too short\n");
			return -5;
		}

		*pchTemp = '\0';

		/* Fields of AqBanking Output
		 * See chapter 10.4.7 of AqBanking Handbook at
		 * https://www.aquamaniac.de/sites/download/
				download.php?package=09&release=09&file=01&
				dummy=aqbanking4-handbook-20091231.pdf
			1 "Account" (fester String)
			2 Bankleitzahl des Kontos
			3 Kontonummer
			4 Name der Bank (falls bekannt)
			5 Name des Kontos (falls bekannt)
			6 Datum des gebuchten Kontostandes (DD.MM.JJJJ)
			7 Uhrzeit des gebuchten Kontostandes (hh:mm)
			8 Betrag des gebuchten Kontostandes
			9 Waehrung des gebuchten Kontostandes
			10 Datum des notierten Kontostandes (DD.MM.JJJJ)
			11 Uhrzeit des notierten Kontostandes (hh:mm)
			12 Betrag des notierten Kontostandes
			13 Waehrung des notierten Kontostandes
		 */

		switch(i)
		{
		case 0: // "Account"
		case 3: // Bank-Name
		case 4: // Account-name (if available)
		case 6: // Time booked
		case 10: // Time noted
			// Diese Felder werden ignoriert.
			break;

		case 1: // localBankCode
			debug_printf(dbg_fld, "BLZ: %s\n", pchField);
			strncpy(&pBuchung->orig_blz[0], pchField, sizeof(pBuchung->orig_blz)-1);
			break;

		case 2: // localAccountNumber
			debug_printf(dbg_fld, "KontoNr: %s\n", pchField);
			strncpy(&pBuchung->orig_ktonr[0], pchField, sizeof(pBuchung->orig_ktonr)-1);
			break;

		case 9: // Date noted
			if(pBuchung->datum[0] != '\0') // got data already from date booked
				break;
			// no break!!!
		case 5: // date booked
			debug_printf(dbg_fld, "Datum: %s\n", pchField);
			if(*pchField)
				strncpy(&pBuchung->datum[0], makeDatum(pchField),
					sizeof(pBuchung->datum)-1);
			break;

		case 11: // Value noted
			if(pBuchung->betrag != 0) // got data already from value booked
				break;
			// no break!!!
		case 7: // value booked
			debug_printf(dbg_fld, "Betrag: %s\n", pchField);
			pBuchung->betrag = atof(pchField);
			break;

		case 12: // Currency noted
			if(pBuchung->waehrung[0] != '\0') // got data already from currency booked
				break;
			// no break!!!
		case 8: // currency booked
			debug_printf(dbg_fld, "Waehrung: %s\n", pchField);
			strncpy(&pBuchung->waehrung[0], pchField, sizeof(pBuchung->waehrung)-1);
			break;

		default:
			fprintf(stderr, "parsing error: Invalid FieldNr %d (\"%s\")\n", i, pchField);
			return -5;
		}

		pchTemp++;
		pchField = pchTemp;
	}

	if(*pBuchung->datum == '\0') // empty date means to ignore the line
		{
		debug_printf(dbg_fld, "Ignoring Line with empty Datum\n")
		memset(pBuchung, '\0', sizeof(*pBuchung)); // just for safety
		return RC_EMPTYLINE;
		}

	return 0;
}

//****************************************************************************
//***** Eine Buchung einer CSV Datei behandeln *******************************
//****************************************************************************
static int processAqbBal(const Buchung buchung)
{
	int iRc;

	debug_printf(dbg_fld, "Saldo %+9.2f %s %s %s %s\n",
			buchung.betrag, buchung.waehrung, buchung.datum,
			buchung.gv_code, buchung.primanota);

	iRc = writeRecord(buchung);

	return iRc;

}

//****************************************************************************
//***** Einen Satz einer CSV Datei behandeln *********************************
//****************************************************************************
int processAqbBalRecord(char * pchBuffer)
{
	int iRc = 0;
	Buchung buchung;

	debug_printf(dbg_in, "processAqbBalRecord: %s\n", pchBuffer);
	debug_printf(dbg_in, "\n");

	iRc = parseAqbBal(pchBuffer, &buchung);
	if(iRc == RC_EMPTYLINE)
		;	// Ignore Line with Datum not set
	else if(iRc == 0)
		{
		iRc = processAqbBal(buchung);
		if(iRc != 0)
			return iRc;
		}
	else
		return iRc;

	return 0;
}
