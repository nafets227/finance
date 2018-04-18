/***** fntxt2sql-btx.c *****/

#include "fntxt2sql.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


static int fTranPending = 0;
static Buchung buchung;

static const char *SRC_ID(void)
{
	static char achID[256] = "";

	if(achID[0] == '\0')
		strcpy(achID, makeSourceDesc(
				"$RCSfile: fntxt2sql-btx.c,v $",
		"$Revision: 1.9 $)"));
	return achID;
}

//****************************************************************************
//***** Puffer Zugriffs-Funktionen *******************************************
//****************************************************************************
static const char *pchBtxBuffer = NULL;
static int        iBtxBufferLen = 0; 
static int		  iBtxBufferPos = 0;

static const char * const getLine()
{

	char const *pch = NULL;

	if(iBtxBufferPos > iBtxBufferLen)
		return NULL;	// EOF

	pch = pchBtxBuffer + iBtxBufferPos;
	iBtxBufferPos += strlen(pch) + 1;

	return pch;
}

static void rewindLine()
{
	if(iBtxBufferPos > 0)
		iBtxBufferPos--;
	
	while(*(pchBtxBuffer + iBtxBufferPos - 1) != '\0' &&
			iBtxBufferPos > 0)
		iBtxBufferPos--;

	return;
}

//****************************************************************************
//***** Satz mit Auftragsreferenznummer bearbeiten ***************************
//****************************************************************************
int processAuftragsRef(const char * const pchBuffer)
{
	// Dieses feld wird ignoriert.
	debug_printf(dbg_fld, "AufRef %s\n", pchBuffer);
	return 0;
}

//****************************************************************************
//***** Satz mit Auftragsreferenznummer bearbeiten ***************************
//****************************************************************************
int processBezugsRef(const char * const pchBuffer)
{
	// Diese feld wird ignoriert.
	debug_printf(dbg_fld, "BezRef %s\n", pchBuffer);
	return 0;
}

//****************************************************************************
//***** Satz mit Auftragsreferenznummer bearbeiten ***************************
//****************************************************************************
int processKontobezeichnung(const char * const pchBuffer)
{
	if(pchBuffer[8] == '/')	// Durch / getrennte BLZ und KtoNr
	{
		// BLZ speichern:
		memcpy(buchung.orig_blz, pchBuffer, sizeof(buchung.orig_blz)-1);
		buchung.orig_blz[sizeof(buchung.orig_blz)-1] = '\0';

		// Ktonr speichern:
		if(strlen(pchBuffer + 9) >= sizeof(buchung.orig_ktonr))
		{
			printf("Format %s von Feld 25 nicht unterstuetzt (2)\n", pchBuffer);
			return -1;
		}
		strcpy(buchung.orig_ktonr, pchBuffer + 9);
	}
	else
	{
		// Ktonr speichern:
		if(strlen(pchBuffer) >= sizeof(buchung.orig_ktonr))
		{
			printf("Format %s von Feld 25 nicht unterstuetzt (3)\n", pchBuffer);
			return -1;
		}
		strcpy(buchung.orig_ktonr, pchBuffer);
	}

	debug_printf(dbg_fld, "Kto %s, BLZ %s\n",
			buchung.orig_ktonr, buchung.orig_blz);

	return 0;
}

//****************************************************************************
//***** Satz mit Auftragsreferenznummer bearbeiten ***************************
//****************************************************************************
int processAuszugsnummer(const char * const pchBuffer)
{
	debug_printf(dbg_fld, "AuszNr %s\n", pchBuffer);
	return 0;
}

//****************************************************************************
//***** Satz mit Auftragsreferenznummer bearbeiten ***************************
//****************************************************************************
int processAnfangssaldo(const char * const pchBuffer)
{
	char achBuffer[32];
	int rc;
	int i;

	strcpy(buchung.source, makeSourceId(SRC_ID())); // Origin belegen
	if(fTranPending)
	{rc = writeRecord(buchung); resetRecord(&buchung); fTranPending = 0; if(rc != 0) return rc;}

	buchung.buchart = 'A';

	strcpy(achBuffer, pchBuffer+10);
	for(i = 0; i < strlen(achBuffer); i++)
		if(achBuffer[i] == ',')
			achBuffer[i] = '.';
	buchung.betrag = atof(achBuffer);

	memcpy(buchung.waehrung, pchBuffer+7, 3);
	buchung.waehrung[4] = '\0';


	memcpy(achBuffer, pchBuffer+1, 6);
	achBuffer[6] = '\0';
	strcpy(buchung.datum, makeDatum(achBuffer));

	switch(*pchBuffer)
	{
	case 'D': // Debit
		buchung.betrag = -buchung.betrag;
		break;

	case 'C': // Credit
		break;

	default:
		printf("Credit / Debit Zeichen %c unbekannt.\n", *pchBuffer);
		return -1;
	}

	debug_printf(dbg_fld, "Anfangssaldo %+9.2f %s %s\n",
			buchung.betrag, buchung.waehrung, buchung.datum);


	fTranPending = 1;

	return 0;
}

//****************************************************************************
//***** Satz mit Auftragsreferenznummer bearbeiten ***************************
//****************************************************************************
int processUmsatz(const char * const pchBuffer)
{
	/* 	960902
	0902
	D
	82,80
	NSTO
	NONREF
oder
	960916
	0916
	D
	300,
	NCHK
	0000080843246 */

	char achTemp[100];
	char const *pchSource; 
	char *pchDest;
	int rc;

	//********** Vorherige Buchung schreiben ******************************
	strcpy(buchung.source, makeSourceId(SRC_ID())); // Origin belegen
	if(fTranPending)
	{rc = writeRecord(buchung); resetRecord(&buchung); fTranPending = 0; if (rc != 0) return rc;}

	buchung.buchart = 'B';

	pchSource = pchBuffer;

	//********** Valuta ***************************************************
	memcpy(achTemp, pchSource, 6);
	achTemp[6] = '\0';
	strcpy(buchung.valuta, makeDatum(achTemp));
	pchSource += 6;

	//********** Datum ****************************************************
	if('0' <= *pchSource && *pchSource <= '9')
	{
		memcpy(achTemp, pchSource, 4);
		achTemp[4] = '\0';
		strcpy(buchung.datum, makeDatum(achTemp));
		pchSource += 4;
	}
	else
		strcpy(buchung.datum, buchung.valuta);

	//********** Creadit / Debit ******************************************
	if(*pchSource == 'D')		// Debit
		buchung.betrag = -1;
	else if(*pchSource == 'C')	// Credtt
		buchung.betrag = +1;
	else if(*pchSource == 'R' && *(pchSource+1) == 'D') // Debit Storno
	{
		buchung.betrag = +1;
		pchSource++;
	}
	else if(*pchSource == 'R' && *(pchSource+1) == 'C') // Credit Storno
	{
		buchung.betrag = -1;
		pchSource++;
	}
	else
	{
		printf("Unbekanntes credit / debit-zeichen %c\n", *pchSource);
		return -3;
	}

	pchSource ++;
	if(*pchSource == ' ')
		pchSource++;

	//********** Waehrung *************************************************
	if(*pchSource < '0' || *pchSource > '9')
	{
		if(*pchSource=='M')
			strcpy(buchung.waehrung, "DEM");
		else if(*pchSource=='R')
			strcpy(buchung.waehrung, "EUR");
		else
		{
			fprintf(stderr, "Waehrung %c nicht unterstuetzt.\n",
					*pchSource);
			return -14;
		}

		pchSource++;
	}

	//********** Betrag ***************************************************
	pchDest = achTemp;

	while(('0' <= *pchSource && *pchSource <= '9') ||
			*pchSource == ',')
	{
		if(*pchSource == ',')
			*pchDest = '.';
		else
			*pchDest = *pchSource;

		pchDest++;
		pchSource++;
	}
	*pchDest = '\0';
	buchung.betrag *= atof(achTemp);


	//********** Buchungs-Schluessel ***************************************
	memcpy(buchung.buchungs_sl, pchSource+1, 3);
	buchung.buchungs_sl[3] = '\0';
	pchSource += 4;

	//********** Referenz *************************************************
	if(strcmp(pchSource, "NONREF"))
		strcpy(buchung.referenz, pchSource);
	else
		buchung.referenz[0] = '\0';

	//********** Buchung vormerken ****************************************
	fTranPending = 1;

	//*********************************************************************
	debug_printf(dbg_fld, "Ums %.2f %s %s (Valuta %s) %s\n",
			buchung.betrag, buchung.buchungs_sl, buchung.datum,
			buchung.valuta, buchung.referenz);

	return 0;
}

//****************************************************************************
//***** Einzelnes Mehrzweck-Feld bearbeiten **********************************
//****************************************************************************
int processMehrzweckPart(const char * const pchBuffer)
{
	int iFeldNr = 	(pchBuffer[0] - '0') * 10 +
	(pchBuffer[1] - '0') * 1;
	char const *pchSource = pchBuffer + 2;

	switch(iFeldNr)
	{
	case 0:		// 00 Buchungstext
		strncpy(buchung.butext, pchSource,
				sizeof(buchung.butext)-1);
		debug_printf(dbg_fld, "Buchungs-Text %s\n",
				buchung.butext);
		break;

	case 10:		// 10 Primanoten-Nr
		strncpy(buchung.primanota, pchSource,
				sizeof(buchung.primanota)-1);
		debug_printf(dbg_fld, "PrimaNota %s\n",
				buchung.primanota);
		break;

	case 20:		// 20 Verwendungszweck 1
		strncpy(buchung.vzweck[0], pchSource,
				sizeof(buchung.vzweck[0])-1);
		debug_printf(dbg_fld, "VZweck 1 %s\n",
				buchung.vzweck[0]);
		break;

	case 21:		// 21 Verwendungszweck 2
		strncpy(buchung.vzweck[1], pchSource,
				sizeof(buchung.vzweck[1])-1);
		debug_printf(dbg_fld, "VZweck 2 %s\n",
				buchung.vzweck[1]);
		break;

	case 22:		// 22 Verwendungszweck 3
		strncpy(buchung.vzweck[2], pchSource,
				sizeof(buchung.vzweck[2])-1);
		debug_printf(dbg_fld, "VZweck 3 %s\n",
				buchung.vzweck[2]);
		break;

	case 23:		// 23 Verwendungszweck 4
		strncpy(buchung.vzweck[3], pchSource,
				sizeof(buchung.vzweck[3])-1);
		debug_printf(dbg_fld, "VZweck 4 %s\n",
				buchung.vzweck[3]);
		break;

	case 24:		// 24 Verwendungszweck 5
		strncpy(buchung.vzweck[4], pchSource,
				sizeof(buchung.vzweck[4])-1);
		debug_printf(dbg_fld, "VZweck 5 %s\n",
				buchung.vzweck[4]);
		break;

	case 25:		// 25 Verwendungszweck 6
		strncpy(buchung.vzweck[5], pchSource,
				sizeof(buchung.vzweck[5])-1);
		debug_printf(dbg_fld, "VZweck 6 %s\n",
				buchung.vzweck[5]);
		break;

	case 26:		// 26 Verwendungszweck 7
		strncpy(buchung.vzweck[6], pchSource,
				sizeof(buchung.vzweck[6])-1);
		debug_printf(dbg_fld, "VZweck 7 %s\n",
				buchung.vzweck[6]);
		break;

	case 30:		// 30 BLZ Partner
		strncpy(buchung.part_blz, pchSource,
				sizeof(buchung.part_blz)-1);
		debug_printf(dbg_fld, "Partner BLZ %s\n",
				buchung.part_blz);
		break;

	case 31:		// 31 Kontonummer Partner
		strncpy(buchung.part_ktonr, pchSource,
				sizeof(buchung.part_ktonr)-1);
		debug_printf(dbg_fld, "Partner KtoNr %s\n",
				buchung.part_ktonr);
		break;

	case 32:		// 32 Name1 Partner
		strncpy(buchung.part_name1, pchSource,
				sizeof(buchung.part_name1)-1);
		debug_printf(dbg_fld, "Partner Name1 %s\n",
				buchung.part_name1);
		break;

	case 33:		// 33 Name2 Partner
		strncpy(buchung.part_name2, pchSource,
				sizeof(buchung.part_name2)-1);
		debug_printf(dbg_fld, "Partner Name2 %s\n",
				buchung.part_name2);
		break;

	case 34:		// 34 Textschluessel-Ergaenzung
		//		strncpy(buchung.??, pchSource, sizeof(buchung.??)-1);
		debug_printf(dbg_fld, "Mzw34 %s\n", pchBuffer);
		break;

	default:
		printf("Unknown Mehrzweck-Part-No %d\n", iFeldNr);
		return -4;
	}

	return 0;
}
//****************************************************************************
//***** Satz mit Mehrzweck-Feld bearbeiten ***********************************
//****************************************************************************
int processMehrzweck(const char * const pchBuffer)
{
	char achMzwBuffer[350 + 1];
	char *pchSource;
	char const *pch;

	strcpy(achMzwBuffer, pchBuffer);

	do
	{
		pch = getLine();
		if(pch == NULL)
			return -100;

		if(*pch == ':')
		{
			rewindLine();
			break;
		}
		else
		{
			strcat(achMzwBuffer, pch);
		}

	} while(1);

	//********** GV-Code **************************************************
	memcpy(buchung.gv_code, achMzwBuffer, 3);
	buchung.gv_code[3] = '\0';

	debug_printf(dbg_fld, "GV-Code %s\n", buchung.gv_code);

	//********** Weitere Felder *******************************************
	for(	pchSource = strtok(&achMzwBuffer[3], "?>");
	pchSource != NULL;
	pchSource = strtok(NULL, "?"))
		processMehrzweckPart(pchSource);

	return 0;
}
//****************************************************************************
//***** Einzelnes ComDirect-Mehrzweck-Feld bearbeiten ************************
//****************************************************************************
int processComDirPart(const char * const pchBuffer)
{
	int iFeldNr = 	(pchBuffer[0] - '0') * 10 +
	(pchBuffer[1] - '0') * 1;
	char const *pchSource = pchBuffer + 2;

	switch(iFeldNr)
	{
	case 1:		// 01 Verwendungszweck 1
		strncpy(buchung.vzweck[0], pchSource,
				sizeof(buchung.vzweck[0])-1);
		debug_printf(dbg_fld, "VZweck 1 %s\n", buchung.vzweck[0]);
		return sizeof(buchung.vzweck[0])-1 + 2;

	case 2:		// 02 Verwendungszweck 2
		strncpy(buchung.vzweck[1], pchSource,
				sizeof(buchung.vzweck[1])-1);
		debug_printf(dbg_fld, "VZweck 2 %s\n", buchung.vzweck[1]);
		return sizeof(buchung.vzweck[1])-1 + 2;

	case 3:		// 03 Verwendungszweck 3
		strncpy(buchung.vzweck[2], pchSource,
				sizeof(buchung.vzweck[2])-1);
		debug_printf(dbg_fld, "VZweck 3 %s\n", buchung.vzweck[2]);
		return sizeof(buchung.vzweck[2])-1 + 2;

	case 4:		// 04 Verwendungszweck 4
		strncpy(buchung.vzweck[3], pchSource,
				sizeof(buchung.vzweck[3])-1);
		debug_printf(dbg_fld, "VZweck 4 %s\n", buchung.vzweck[3]);
		return sizeof(buchung.vzweck[3])-1 + 2;

	case 5:		// 05 Verwendungszweck 5
		strncpy(buchung.vzweck[4], pchSource,
				sizeof(buchung.vzweck[4])-1);
		debug_printf(dbg_fld, "VZweck 5 %s\n", buchung.vzweck[4]);
		return sizeof(buchung.vzweck[4])-1 + 2;

	case 6:		// 06 Verwendungszweck 6
		strncpy(buchung.vzweck[5], pchSource,
				sizeof(buchung.vzweck[5])-1);
		debug_printf(dbg_fld, "VZweck 6 %s\n", buchung.vzweck[5]);
		return sizeof(buchung.vzweck[5])-1 + 2;

	case 7:		// 07 Verwendungszweck 7
		strncpy(buchung.vzweck[6], pchSource,
				sizeof(buchung.vzweck[6])-1);
		debug_printf(dbg_fld, "VZweck 7 %s\n", buchung.vzweck[6]);
		return sizeof(buchung.vzweck[6])-1 + 2;

	case 15:		// 15 Buchungstext
		strncpy(buchung.butext, pchSource,
				sizeof(buchung.butext)-1);
		debug_printf(dbg_fld, "Buchungs-Text %s\n", buchung.butext);
		return sizeof(buchung.butext)-1 + 2;

	default:
		printf("Unknown ComDirect-Part-No %d\n", iFeldNr);
		return -4;
	}

	return -1;	// we should never reach this!
}

//****************************************************************************
//***** Satz mit ComDirect Buchungstext bearbeiten ***************************
//****************************************************************************
int processComDirText(const char * const pchBuffer)
{
	char achCDirBuffer[350 + 1];
	char const *pch;
	int  iRc;

	strcpy(achCDirBuffer, pchBuffer);

	do
	{
		pch = getLine();
		if(pch == NULL)
			return -100;

		if(*pch == '\0' || *pch == ' ')
			// Empty Line indicates end of CDirText
			break;
		else
			strcat(achCDirBuffer, pch);

	} while(1);
	
	// Nun haben wir alle CDir Texte eingelesen.
	// Jetzt folgt die Abarbeitung
	for(pch = achCDirBuffer; *pch != '\0'; pch += iRc)
	{
		iRc = processComDirPart(pch);
		if(iRc < 0)	// Error
			return iRc;
	}

	debug_printf(dbg_fld, "ComDirectText %s\n", achCDirBuffer);

	return 0;
}

//****************************************************************************
//***** Satz mit Schlusssalso bearbeiten *************************************
//****************************************************************************
int processSchlusssaldo(const char * const pchBuffer)
{
	char achBuffer[32];
	int rc;
	int i;

	strcpy(buchung.source, makeSourceId(SRC_ID())); // Origin belegen
	if(fTranPending)
	{rc = writeRecord(buchung); resetRecord(&buchung); fTranPending = 0; if(rc != 0) return rc;}

	buchung.buchart = 'E';

	strcpy(achBuffer, pchBuffer+10);
	for(i = 0; i < strlen(achBuffer); i++)
		if(achBuffer[i] == ',')
			achBuffer[i] = '.';
	buchung.betrag = atof(achBuffer);

	memcpy(buchung.waehrung, pchBuffer+7, 3);
	buchung.waehrung[4] = '\0';

	memcpy(achBuffer, pchBuffer+1, 6);
	achBuffer[6] = '\0';
	strcpy(buchung.datum, makeDatum(achBuffer));

	switch(*pchBuffer)
	{
	case 'D': // Debit
		buchung.betrag = -buchung.betrag;
		break;

	case 'C': // Credit
		break;

	default:
		printf("Credit / Debit Zeichen %c unbekannt.\n", *pchBuffer);
		return -1;
	}

	debug_printf(dbg_fld, "Schlussssaldo %+9.2f %s %s\n",
			buchung.betrag, buchung.waehrung, buchung.datum);


	fTranPending = 1;

	return 0;
}

//****************************************************************************
//***** Satz mit Zwischensaldo bearbeiten ************************************
//****************************************************************************
int processZwischensaldo(const char * const pchBuffer)
{
	char achBuffer[32];
	int rc;
	int i;

	strcpy(buchung.source, makeSourceId(SRC_ID())); // Origin belegen
	if(fTranPending)
	{rc = writeRecord(buchung); resetRecord(&buchung); fTranPending = 0; if(rc != 0) return rc;}

	buchung.buchart = 'Z';

	strcpy(achBuffer, pchBuffer+10);
	for(i = 0; i < strlen(achBuffer); i++)
		if(achBuffer[i] == ',')
			achBuffer[i] = '.';
	buchung.betrag = atof(achBuffer);

	memcpy(buchung.waehrung, pchBuffer+7, 3);
	buchung.waehrung[4] = '\0';

	memcpy(achBuffer, pchBuffer+1, 6);
	achBuffer[6] = '\0';
	strcpy(buchung.datum, makeDatum(achBuffer));

	switch(*pchBuffer)
	{
	case 'D': // Debit
		buchung.betrag = -buchung.betrag;
		break;

	case 'C': // Credit
		break;

	default:
		printf("Credit / Debit Zeichen %c unbekannt.\n", *pchBuffer);
		return -1;
	}

	debug_printf(dbg_fld, "Zwischensaldo %+9.2f %s %s\n",
			buchung.betrag, buchung.waehrung, buchung.datum);


	fTranPending = 1;

	return 0;
}

//****************************************************************************
//***** Satz mit Auftragsreferenznummer bearbeiten ***************************
//****************************************************************************
int processValSaldo(const char * const pchBuffer)
{
	//	strncpy(buchung.origkto, pchBuffer, sizeof(buchung.origkto)-1);
	debug_printf(dbg_fld, "ValSaldo %s\n", pchBuffer);
	return 0;
}

//****************************************************************************
//***** Satz mit Auftragsreferenznummer bearbeiten ***************************
//****************************************************************************
int processEndBuchung()
{
	int rc;

	debug_printf(dbg_fld, "EndBuchung \n\n");

	if(fTranPending)
	{
		strcpy(buchung.source, makeSourceId(SRC_ID())); // Origin belegen
		rc = writeRecord(buchung); 
		resetRecord(&buchung); 
		fTranPending = 0; 
		if(rc != 0) 
			return rc;
	}

	return 0;
}

//****************************************************************************
//***** Einen Satz einer Btx-Txt Datei behandeln *****************************
//****************************************************************************
static int processBtxRecord(const char * const pchBuffer)
{
	debug_printf(dbg_in, "processBtxRecord(\"%s\")\n", pchBuffer);

	strcpy(buchung.source, makeSourceId(SRC_ID())); // Origin belegen

	if     (!memcmp(pchBuffer, ":20:", 4))	// Auftragsreferenznummer
		return processAuftragsRef(pchBuffer+4);
	else if(!memcmp(pchBuffer, ":21:", 4))	// Bezugsreferenznummer
		return processBezugsRef(pchBuffer+4);
	else if(!memcmp(pchBuffer, ":25:", 4))	// Kontobezeichnung
		return processKontobezeichnung(pchBuffer+4);
	else if(!memcmp(pchBuffer, ":28:", 4))	// Auszugsnummer
		return processAuszugsnummer(pchBuffer+4);
	else if(!memcmp(pchBuffer, ":28C:", 4))	// Auszugsnummer
		return processAuszugsnummer(pchBuffer+4);
	else if(!memcmp(pchBuffer, ":60F:", 5))	// Anfangssaldo
		return processAnfangssaldo(pchBuffer+5);
	else if(!memcmp(pchBuffer, ":60M:", 5))	// Zwischensaldo
		return processAnfangssaldo(pchBuffer+5);
	else if(!memcmp(pchBuffer, ":61:", 4))	// Umsatzzeile
		return processUmsatz(pchBuffer+4);
	else if(!memcmp(pchBuffer, ":86:", 4))	// Mehrzweckfeld
		return processMehrzweck(pchBuffer+4);
	else if(!memcmp(pchBuffer, ":NS:", 4))	// ComDirect Text
		return processComDirText(pchBuffer+4);
	else if(!memcmp(pchBuffer, ":62F:" , 5))// Schlusssaldo
		return processSchlusssaldo(pchBuffer+5);
	else if(!memcmp(pchBuffer, ":62M:" , 5))// Zwischensaldo
		return processZwischensaldo(pchBuffer+5);
	else if(!memcmp(pchBuffer, ":64:", 4))	// aktueller Valutensaldo
		return processValSaldo(pchBuffer+4);
	else if(!memcmp(pchBuffer, ":65:", 4))	// aktueller Valutensaldo
		return processValSaldo(pchBuffer+4);
	else if(!memcmp(pchBuffer, "\x2D", 1))	// Ende aktuelle Buchungen
		return processEndBuchung();
	else if(!memcmp(pchBuffer, "/OCMT/", 6))// Andere Waehrung
		;
	else if(!memcmp(pchBuffer, "    ", 4) ||
			*pchBuffer == '\0'			   )// Leeres Feld
	    ;
	else					// Unbekannte Felder
	{
		printf("Unknown Field \"%s\"\n", pchBuffer);
		return -2;
	}

	return 0;
}

int processLastBtxRecord()
{
	int rc;

	if(fTranPending)
	{
		strcpy(buchung.source, makeSourceId(SRC_ID())); // Origin belegen
		rc = writeRecord(buchung); 
		resetRecord(&buchung); 
		fTranPending = 0; 
		if(rc != 0) 
			return rc;
	}

	return 0;
}


//****************************************************************************
//***** Read File into Memory, convert special chars and handle          *****
//***** each record via calling processBtxRecord.                        *****
//***** CAVEAT: Sub-Functions of processBtxRecord call the Buffer        *****
//***** Access functions too!                                            *****
//****************************************************************************
int processBtxFile(FILE * file)
{
	char *pchBuffer = NULL;
	char const *pch;
	struct stat fileStat;
	int iRc = 0;
	int iBufLen = 0;
	int iActPos = 0;
	int iActPosWrite = 0; 
	//	int iSepPos = 0;
	//	int i = 0;

	//***** allocate Memory to hold the complete file ************************
#ifndef CYGWIN
	if(fstat(file->_fileno, &fileStat) != 0)
#else
	if(fstat(file->_file, &fileStat) != 0)
#endif
	{
		fprintf(stderr, "Error stat auf File\n");
		return -2;
	}

	pchBuffer = malloc(fileStat.st_size + 1);
	if(pchBuffer == NULL)
	{
		fprintf(stderr, "Error allocating %d bytes\n", (int)fileStat.st_size);
		return -3;
	}

	//***** read complete file ***********************************************
	iBufLen = fread(pchBuffer, 1, fileStat.st_size, file);
	if(iBufLen != fileStat.st_size)
	{
		fprintf(stderr, "Error reading file: Only %d instead of %d bytes read.\n",
				iBufLen, (int)fileStat.st_size);
		free(pchBuffer);
		return -4;
	}

	//***** correct some charachters which have a special meaning ************
	for(iActPos=0, iActPosWrite = 0; iActPos < iBufLen; iActPos++)
	{
		switch(*(pchBuffer + iActPos ) )
		{
		case '\x0D':
		case '\x0A':   // newline characters are ignored
			break;

		/***** Caveat: Erst beim zweiten Zeichen im konvertierten Buffer
		 * gucken, ob das vorherige Zeichen auch ein X15 ist.
		 * So kann die Kombination \x40\n\x40 erkannt werden.
		 */
		case '\x40':	 // sometimes there are X`40` instead of X`15`
		case '\xA7':     // xcept changes X`15` to X'A7` so we have 
			// to correct this
		case '\x15':
			if(iActPosWrite > 0)		// Erstes Zeichen kopieren
				if( *(pchBuffer + iActPosWrite - 1) == '\x40' ||
						*(pchBuffer + iActPosWrite - 1) == '\xA7' ||
						*(pchBuffer + iActPosWrite - 1) == '\x15')
				{
					*(pchBuffer + iActPosWrite - 1) = '\0';	// Token Ende schreiben
					// kein iActPosWrite++ weil vorherige x15 schon geschrieben ist!
					break;
				}
			// kein break sondern weiter, damit das Zeichen kopiert wird!

		default:		// Alle anderen Zeichen kopieren
			if(iActPos != iActPosWrite)
				*(pchBuffer + iActPosWrite) = *(pchBuffer + iActPos);
			iActPosWrite++;
			break;

		}

	}
	iBufLen = iActPosWrite;	//Verkuerzung wegen ignorierter Zeichen
	*(pchBuffer + iBufLen) = '\0';	// Letztes Ende sicherstellen

	// NUn in statische Variablen schreiben, damit getLine funktioniert
	pchBtxBuffer = pchBuffer;
	iBtxBufferLen = iBufLen;


	//***** now handle the records *******************************************
	while((pch = getLine()) != NULL && iRc == 0)
		iRc = processBtxRecord(pch);

	if(iRc == 0)
		processLastBtxRecord();

	pchBtxBuffer = NULL;
	iBtxBufferLen = 0;
	free(pchBuffer);

	return iRc;
}
