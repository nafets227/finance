/*****  fntxt2sql-util.c *****/

#include "fntxt2sql.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iconv.h>

static const char *SRC_ID(void)
{
	static char achUtilID[256] = "";

	if(achUtilID[0] == '\0')
		strcpy(achUtilID, makeSourceDesc(
				"$RCSfile: fntxt2sql-util.c,v $",
		"$Revision: 1.32 $)"));
	return achUtilID;
}

//****************************************************************************
//***** Feld drucken, wenn nicht NULL ****************************************
//****************************************************************************
void printNullableString(const char * pText)
{
	if(pText && *pText)
		printf(", \"%s\"", pText);
	else
		printf(", NULL");

	return;
}

void printNullableChar(const char Text)
{
	if(Text)
		printf(", \"%c\"", Text);
	else
		printf(", NULL");
}

void resetRecord(Buchung *pBuchung)
{
	memset(pBuchung->datum, '\0', sizeof(pBuchung->datum));
	memset(pBuchung->valuta,'\0', sizeof(pBuchung->valuta));
	pBuchung->betrag = 0;
	pBuchung->buchart = '\0';
	memset(pBuchung->buchungs_sl,'\0', sizeof(pBuchung->buchungs_sl));
	memset(pBuchung->gv_code,'\0', sizeof(pBuchung->gv_code));
	memset(pBuchung->part_blz,'\0', sizeof(pBuchung->part_blz));
	memset(pBuchung->part_ktonr,'\0', sizeof(pBuchung->part_ktonr));
	memset(pBuchung->part_name1,'\0', sizeof(pBuchung->part_name1));
	memset(pBuchung->part_name2,'\0', sizeof(pBuchung->part_name2));
	memset(pBuchung->primanota,'\0', sizeof(pBuchung->primanota));
	memset(pBuchung->referenz,'\0', sizeof(pBuchung->referenz));
	memset(pBuchung->butext,'\0', sizeof(pBuchung->butext));
	memset(pBuchung->vzweck,'\0', sizeof(pBuchung->vzweck));
	memset(pBuchung->source,'\0', sizeof(pBuchung->source));

	return;
}

//****************************************************************************
//***** CodePage Convertierungen, insbesondere fuer UTF8 *********************
//****************************************************************************
int convertCP(char * pch, const char * pchSourceCp, const char *pchDestCp)
{
	iconv_t *pConv = NULL;

	char achBuffer[1024];
	size_t sInput, sOutput;
	char *pchIn, *pchOut;

	if(pch == NULL || *pch == '\0')
		return 0;

	pConv=iconv_open(pchDestCp, pchSourceCp);
	if(pConv == (iconv_t*)-1)
	{
		fprintf(stderr, "Error in iconv_open(\"%s\", \"%s\")\n",
				pchDestCp, pchSourceCp);
		return -1;
	}


	pchIn = pch;
	pchOut = achBuffer;
	sInput = strlen(pch);
	sOutput = sizeof(achBuffer);
	iconv(pConv, &pchIn, &sInput, &pchOut, &sOutput );
	if(sInput != 0)
	{
		fprintf(stderr, "Error in iconv %s->%s(\"%s\")\n",
				pchSourceCp, pchDestCp, pch);
		return -2;
	}

	achBuffer[sizeof(achBuffer)-sOutput] = '\0';
	strcpy(pch, achBuffer);

	return 0;
}

//****************************************************************************
//***** Satz normalisieren, d.h. fuehrende Nullen etc. ***********************
//****************************************************************************
static void normalizeNum(char *pText, size_t size /* incl. termination \0 */)
{	
	int i;

	if(*pText == '\0')	// never change empty fields
		return;

	// right-justify
	while(pText[size-2] == ' ' || pText[size-2] == '\0')
	{
		memmove(pText+1, pText, size-2);
		pText[0] = ' ';
	}

	// delete leading blanks
	for(i=0; pText[i] == ' '; i++)
		pText[i] = '0';

	pText[size-1] = '\0';	// nur zur Sicherheit
}

// Alle Blankd aus einem Text entfernen (zum Vergleichen besser, da es immer
//      wieder Abweichungen in den Texten gibt)
// In frueheren Versionen wurden nur alle Blanks auf einen reduziert.
// Das hat aber nicht ausreichend geholfen.
// Insbesondere bei SEPA Gutschriften des Kindergelds in 2013 meldet die
// HVB ab 2014 einen Blank wo vorher keiner war.
static void trimText( const char const * pchSource, char *pchDest )
{
	const char *pchActSource = pchSource;
	char * pchActDest = pchDest;
	
	while(*pchActSource != '\0')
	{
		if(*pchActSource != ' ')	// NIcht blanks einfach kopieren und weiter
		{
			*pchActDest = *pchActSource;
			pchActDest++;
		}
		pchActSource++;
	}
	*pchActDest = '\0';	// String Ende nicht vergessen!
	
	return;
}
// Codepage convertieren
static void normalizeText(char * pch)
{
/*	static struct { char chOld, chNew; } CONV[] = {
		{ 0xdc, '??'} };

// Codepage Convertierung manuell
	int iPos, iConvPos;

	for(iPos = 0; pch[iPos] != '\0'; iPos++)
		{
		for(iConvPos = 0; iConvPos <= sizeof(CONV)/sizeof(CONV[0]); iConvPos++)
			{
			if(pch[iPos] == CONV[iConvPos].chOld)
				pch[iPos] = CONV[iConvPos].chNew;
			}
		}
*/
	return;
	
/*
	static const char *pchDestCp = "utf8";
	static iconv_t *pConv = NULL;
	
	char achBuffer[1024];
	size_t sInput, sOutput;
	char *pchIn, *pchOut;
	
	if(pch == NULL || *pch == '\0')
		return;
	
	if(pConv == NULL)
	{
		pConv=iconv_open(pchDestCp, config.achCodePage);
		if(pConv == (iconv_t*)-1)
		{
			fprintf(stderr, "Error in iconv_open(\"%s\", \"%s\")\n",
					pchDestCp, config.achCodePage);
			return;
		}
	}
	
	pchIn = pch;
	pchOut = achBuffer;
	sInput = strlen(pch);
	sOutput = sizeof(achBuffer);
	iconv(pConv, &pchIn, &sInput, &pchOut, &sOutput );
	if(sInput != 0)
		fprintf(stderr, "Error in iconv %s->%s(\"%s\")\n",
				config.achCodePage, pchDestCp, pch);
	else
	{
		achBuffer[sizeof(achBuffer)-sOutput] = '\0';
		strcpy(pch, achBuffer);
	}
*/
}

static void normalizeRecord(Buchung *pBuchung)
{
	int iPos;

	normalizeNum(pBuchung->buchungs_sl, sizeof(pBuchung->buchungs_sl));
	normalizeNum(pBuchung->orig_ktonr, sizeof(pBuchung->orig_ktonr));
	normalizeNum(pBuchung->primanota, sizeof(pBuchung->primanota));

	if(pBuchung->waehrung[0] == '\0')
	{
		if(strncmp(pBuchung->datum, "2002", 4) < 0)		// Vor 2002: Default Waehrung DEM
			strcpy(pBuchung->waehrung, "DEM");
		else
			strcpy(pBuchung->waehrung, "EUR");
	}

	iPos = strlen(pBuchung->butext);
	if( pBuchung->butext[iPos-1] == ')' && 
			pBuchung->butext[iPos-5] == '(' &&
			pBuchung->butext[iPos-6] == ' ')
		pBuchung->butext[iPos-6] = '\0';
		
	// Codepage conversion
	normalizeText(pBuchung->butext);
	normalizeText(pBuchung->part_name1);
	normalizeText(pBuchung->part_name2);
	normalizeText(pBuchung->referenz);
	for(iPos=0; iPos<sizeof(pBuchung->vzweck)/sizeof(pBuchung->vzweck[0]); iPos++)
		normalizeText(pBuchung->vzweck[iPos]);
}

/*****************************************************************************
 * compare a new (to be written) Record newBuch to a one read from the 
 * database.
 * Return Values:
 *      < 0 : Records are to be treated as not equal
 *      = 0 : Error
 *      > 0 : Records are to be treated as equal.
 *  
 *      Return Code number (pos+neg) indicates which rule applied. 
 * 		Use this for output/debugging only!
 ****************************************************************************/
int compareRecord(const Buchung newBuch, const Buchung existBuch)
{
	static const int 
//		RC_orig_ktonr  =  10,
		RC_orig_blz    =  20,
//		RC_buchart     =  30,
//		RC_datum       =  40,
//		RC_valuta      =  50,
//		RC_waehrung    =  60,
//		RC_betrag      =  70,
//		RC_buchungs_Sl =  80,
//		RC_referenz    =  90,
//		RC_gv_code     = 100,
		RC_part_name1  = 110,
		RC_part_name2  = 120,
		RC_part_ktonr  = 130,
		RC_part_blz    = 140,
		RC_butext      = 150,
//		RC_primanota   = 160,
		RC_vzweck      = 900;
	int iPos;
	int iRc = +1;

	
	if(strcmp(newBuch.orig_blz, existBuch.orig_blz))
	{
		if(  (  !strcmp(newBuch.orig_blz, "70020270") && 
				!strcmp(existBuch.orig_blz, "70020001") ) || 
			 (  !strcmp(newBuch.orig_blz, "70020001") && 
			    !strcmp(existBuch.orig_blz, "70020270") ) )
		{
			// Sicherstellen dass ein Mindest Anzahl von Feldern uebereinstimmen
			if( !strcmp(newBuch.orig_ktonr, existBuch.orig_ktonr) &&
					!strcmp(newBuch.datum , existBuch.datum ) &&
					newBuch.buchart == existBuch.buchart &&
					newBuch.betrag  == existBuch.betrag )
				// Sonderregel: Wenn die BLZ 70020001 und 70020270 auftauchen,
				// dann sind die Buchungen identisch, sobald die Felder Ktonr,
				// Datum, buchart und betrag uebereinstimmen.
				// Hintergrund ist, dass die Hypo-Bank (70020001) auf die HVB
				// (70020270) fusioniert wurde und dabei die Details der Buchungen
				// von HVB anders geliefert werden als von Hypo-Bank.
				return RC_orig_blz + 1; 
		}
		else
			return - RC_orig_blz;
	}
	
	//***** butext ***********************************************************
	if(newBuch.butext[0] != '\0')
	{
		if(existBuch.butext[0] == '\0')
			iRc = RC_butext + 1;
		else
			if(strcmp(newBuch.butext, existBuch.butext))
				return - RC_butext - 2;
	}


	//***** part_name1 *******************************************************
	if(newBuch.part_name1[0] != '\0')
	{
		if(existBuch.part_name1[0] == '\0')
			iRc = RC_part_name1 + 1;
		else
			if(strcmp(newBuch.part_name1, existBuch.part_name1))
				return - RC_part_name1 - 2;
	}

	//***** part_name2 *******************************************************
	if(newBuch.part_name2[0] != '\0')
	{
		if(existBuch.part_name2[0] == '\0')
			iRc = RC_part_name2 + 1;
		else
			if(strcmp(newBuch.part_name2, existBuch.part_name2))
				return - RC_part_name2 - 2;
	}

	//***** part_ktonr *******************************************************
	if(newBuch.part_ktonr[0] != '\0')
	{
		if(existBuch.part_ktonr[0] == '\0')
			iRc = RC_part_ktonr + 1;
		else
			if(strcmp(newBuch.part_ktonr, existBuch.part_ktonr))
				return - RC_part_ktonr - 2;
	}

	//***** part_blz *********************************************************
	if(newBuch.part_blz[0] != '\0')
	{
		if(existBuch.part_blz[0] == '\0')
			iRc = RC_part_blz + 1;
		else
			if(strcmp(newBuch.part_blz, existBuch.part_blz))
				return - RC_part_blz - 2;
	}

	//****** vzweck 1-7 ******************************************************
	char newText[sizeof(newBuch.vzweck)] = "",
	     newTextTrim[sizeof(newBuch.vzweck)],
	     existText[sizeof(existBuch.vzweck)] = "",
	     existTextTrim[sizeof(existBuch.vzweck)];
	const char *ignoreText="SVWZ+";
	const int   ignoreLen=strlen(ignoreText);

	for(iPos = 0; 
			iPos < sizeof(newBuch.vzweck)/sizeof(newBuch.vzweck[0]); 
			iPos++)
	{
		if( *newBuch.vzweck[iPos] != '\0')
		{
			if(strncmp(ignoreText, newBuch.vzweck[iPos], ignoreLen))
				strcat(newText, newBuch.vzweck[iPos]);
			else
				strcat(newText, newBuch.vzweck[iPos] + ignoreLen);
		}

		if( *existBuch.vzweck[iPos] != '\0')
		{
			if(strncmp(ignoreText, existBuch.vzweck[iPos], ignoreLen))
				strcat(existText, existBuch.vzweck[iPos]);
			else
				strcat(existText, existBuch.vzweck[iPos] + ignoreLen);
		}
	}
	trimText(newText, newTextTrim);
	trimText(existText, existTextTrim);

	int iCompLen = strlen(newTextTrim);
	if(iCompLen > strlen(existTextTrim))
		iCompLen = strlen(existTextTrim);

	if(strncmp(newTextTrim, existTextTrim, iCompLen))
		return -RC_vzweck;
	else
		iRc = RC_vzweck;


	return iRc;	// No Differences found - so records are the same!
}

//****************************************************************************
//***** Satz auf Datei oder Datenbank schreiben ******************************
//****************************************************************************
int writeRecord(const Buchung inBuchung)
{
	int i;
	int iRc = 0;
	Buchung buchung = inBuchung;

	normalizeRecord(&buchung);

	switch(config.printFormat)
	{
	case txtMultiLine:
		printf("%s/%s: " // BLZ / Kto
				"%s/%s "  // Buchungsdatum / valuta
				"%+011.2f %s "  // Betrag / Waehrung
				"(%c,%s,%s) "  // Buchungs-Art, -Schluessel, und GV-Code
				"\n\t%s/%s: %s %s" // Partner: BLZ, KtoNr, Name1, Name2
				"\n\tPN %s %s/%s\n", // Primanota, Refernz und Buchungstext
				buchung.orig_blz, buchung.orig_ktonr,
				buchung.datum, buchung.valuta,
				buchung.betrag, buchung.waehrung,
				buchung.buchart, buchung.buchungs_sl, buchung.gv_code,
				buchung.part_blz, buchung.part_ktonr,
				buchung.part_name1, buchung.part_name2,
				buchung.primanota, buchung.referenz, buchung.butext);

		for(i = 0; i < sizeof(buchung.vzweck) / sizeof(buchung.vzweck[0]); i++)
			if(buchung.vzweck[i][0] != '\0')
				printf("\t%s\n", buchung.vzweck[i]);

		printf("\tSRC %s\n", buchung.source);
		break;

	case txtSingleLine:
		printf("%8s/%10s %10s/%10s"  
				"%+11.2f %3s "
				"(%c,"
				"%3s,"
				"%10s) %8s/%10s "
				"%27s %27s %4s"
				"%27s/%27s",
				buchung.orig_blz, buchung.orig_ktonr, buchung.datum, buchung.valuta, 
				buchung.betrag, buchung.waehrung, 
				0x20 <= buchung.buchart && buchung.buchart <= 0x80 ? buchung.buchart : ' ', 
						buchung.buchungs_sl,
						buchung.gv_code, buchung.part_blz, buchung.part_ktonr,
						buchung.part_name1, buchung.part_name2, buchung.primanota, 
						buchung.referenz, buchung.butext);

		for(i = 0; i < sizeof(buchung.vzweck) / sizeof(buchung.vzweck[0]); i++)
			printf("\"%27s\" ", buchung.vzweck[i]);

		printf("%s\n", buchung.source);


		break;

	case txtSql:
		printf("%s;\n", getInsertSql(buchung));
		/*			
			printf("INSERT INTO %s ("
				"ORIG_BLZ, ORIG_KTONR, "
				"DATUM, VALUTA, "
				"BETRAG, WAEHRUNG, "
				"BUCHART, BUCHUNGS_SL, GV_CODE, "
				"PART_BLZ, PART_KTONR, PART_NAME1, PART_NAME2, "
				"PRIMANOTA, REFERENZ, BUTEXT, "
				"VZWECK1, VZWECK2, VZWECK3, VZWECK4, "
				"VZWECK5,VZWECK6, VZWECK7, "
				"SOURCE "
				") VALUES (", config.achSqlTabName);
			printf("\"%s\"", buchung.orig_blz);
			printNullableString(buchung.orig_ktonr);
			printNullableString(buchung.datum);
			printNullableString(buchung.valuta);

			printf(", %f", buchung.betrag);
			printNullableString(buchung.waehrung);
			printNullableChar(buchung.buchart);
			printNullableString(buchung.buchungs_sl);
			printNullableString(buchung.gv_code);
			printNullableString(buchung.part_blz);
			printNullableString(buchung.part_ktonr);
			printNullableString(buchung.part_name1);
			printNullableString(buchung.part_name2);
			printNullableString(buchung.primanota);
			printNullableString(buchung.referenz);
			printNullableString(buchung.butext);

			for(i = 0; i < sizeof(buchung.vzweck) / sizeof(buchung.vzweck[0]); i++)
				printNullableString(buchung.vzweck[i]);

			printNullableString(buchung.source);
			printf(");\n");
		 */			break;

#ifdef CONF_MYSQL
	case execMysql:
		iRc = writeMysqlRecord(buchung);
		if(iRc != 0)
			return iRc;
		break;

	case checkMysql:    
		iRc = checkMysqlRecord(buchung);
		if(iRc != 0)
			return iRc;
		break;
#endif			

	default:
		fprintf(stderr, "Wrong Output format selected\n");
		return -13;
	}

	fflush(stdin);

	return 0;
}

int createTable(void)
{
	int iRc = 0, i;
	const char * const * ppchStmts;

	ppchStmts = getCreateSql();

	switch(config.printFormat)
	{
	case txtMultiLine:
	case txtSingleLine:
	case txtSql:
		for(i = 0; ppchStmts[i] != NULL; i++)
			printf("%s\n", ppchStmts[i]);

		iRc = 0;
		break;
#ifdef CONF_MYSQL
	case execMysql:
		for(i = 0; ppchStmts[i] != NULL; i++)
		{
			iRc = createMysqlTable(ppchStmts[i]);
			if (iRc < 0)
				break;
		}
		break;
#endif
	default:
		fprintf(stderr, "wrong print format %d\n", config.printFormat);
		iRc = -1;
	}

	return iRc;
}

//****************************************************************************
//***** String ins Datums-Format konvertieren ********************************
//****************************************************************************
char* makeDatum(const char *pchBuffer)
{
	static char achDatum[10+1] = "0000-00-00";
	const char *pchActBuffer = pchBuffer;


	switch(strlen(pchActBuffer))
	{
	case 6:   // JJMMTT
		if(*(pchActBuffer) >= '7')
			strcpy(achDatum, "19");
		else
			strcpy(achDatum, "20");

		achDatum[ 2] = *(pchActBuffer+0);
		achDatum[ 3] = *(pchActBuffer+1);
		pchActBuffer += 2;
		// Kein break!!!

	case 4:    // MMTT
		achDatum[ 5] = *(pchActBuffer+0);
		achDatum[ 6] = *(pchActBuffer+1);
		achDatum[ 8] = *(pchActBuffer+2);
		achDatum[ 9] = *(pchActBuffer+3);
		break;

	case 10:
		if(*(pchActBuffer+2) == '.' && *(pchActBuffer+5) == '.') //TT.MM.JJJJ
		{
			memcpy(achDatum, pchActBuffer+6, 4);	// JJJJ
			achDatum[5] = *(pchActBuffer+3);
			achDatum[6] = *(pchActBuffer+4);
			achDatum[8] = *(pchActBuffer+0);
			achDatum[9] = *(pchActBuffer+1);
		}
		else if(*(pchActBuffer+4) == '/' && *(pchActBuffer+7) == '/') // JJJJ/MM/TT
		{
			memcpy(achDatum, pchActBuffer+0, 4);	// JJJJ
			achDatum[5] = *(pchActBuffer+5);
			achDatum[6] = *(pchActBuffer+6);
			achDatum[8] = *(pchActBuffer+8);
			achDatum[9] = *(pchActBuffer+9);
		}
		else
			printf("Ungueltiges Datums-Format-10) %s\n", pchBuffer);
		break;

	default:
		printf("Ungueltiges Datums-Format %s\n", pchBuffer);
		return NULL;
	}

	debug_printf(dbg_datum, "\tmakeDatum(\"%s\") = \"%s\"\n",
			pchBuffer, achDatum);

	return achDatum;
}

char* makePktDatum(const char *pchBuffer)
{
	static char achDatum[10+1] = "0000-00-00";
	const char *pchActBuffer = pchBuffer;

	// Tag behandeln
	if(*(pchActBuffer+1)== '.')
	{
		achDatum[8] = '0';
		achDatum[9] = *pchActBuffer;
		pchActBuffer += 2;
	}
	else if (*(pchActBuffer+2) == '.')
	{
		achDatum[8] = *pchActBuffer;
		achDatum[9] = *(pchActBuffer+1);
		pchActBuffer += 3;
	}
	else
	{
		printf("Ungueltiges Datums-Format-Tag %s\n", pchBuffer);
		return NULL;
	}

	// Monat behandeln
	if(*(pchActBuffer+1)== '.')
	{
		achDatum[5] = '0';
		achDatum[6] = *pchActBuffer;
		pchActBuffer += 2;
	}
	else if (*(pchActBuffer+2) == '.')
	{
		achDatum[5] = *pchActBuffer;
		achDatum[6] = *(pchActBuffer+1);
		pchActBuffer += 3;
	}
	else
	{
		printf("Ungueltiges Datums-Format-Monat %s\n", pchBuffer);
		return NULL;
	}

	// Jahr behandeln
	if(*(pchActBuffer+2)== '\0')
	{
		achDatum[0] = '2';
		achDatum[1] = '0';
		achDatum[2] = *pchActBuffer;
		achDatum[3] = *(pchActBuffer+1);
		pchActBuffer += 2;
	}
	else if (*(pchActBuffer+4) == '\0')
	{
		achDatum[0] = *pchActBuffer;
		achDatum[1] = *(pchActBuffer+1);
		achDatum[2] = *(pchActBuffer+2);
		achDatum[3] = *(pchActBuffer+3);
		pchActBuffer += 4;
	}
	else
	{
		printf("Ungueltiges Datums-Format-Jahr %s\n", pchBuffer);
		return NULL;
	}

	if(*pchActBuffer != '\0')
	{
		printf("Ungueltiges Datums-Format-zuLang \"%s\"\n", pchBuffer);
		return NULL;
	}

	debug_printf(dbg_datum, "\tmakePktDatum(\"%s\") = \"%s\"\n",
			pchBuffer, achDatum);

	return achDatum;
}

//****************************************************************************
//***** String ins Datums-Format konvertieren ********************************
//****************************************************************************
double makeBetrag(const char *pchBuffer)
{
	char achBuffer[200];
	char *pc = achBuffer;
	double dValue;

	strcpy(achBuffer, pchBuffer);

	while(*pc != '\0')
	{
		switch(*pc)
		{
		case '.':			// Zeichen loeschen
			memmove(pc, pc + 1, strlen(pc) - 1 + 1);
			break;
		case ',':
			*pc = '.';
			pc++;
		default:
			pc++;
		}
	}

	dValue = atof(achBuffer);

	debug_printf(dbg_datum, "\tmakeBetrag(\"%s\") = %+9.2f\n",
			pchBuffer, dValue);

	return dValue;
}

//****************************************************************************
//***** Source-Hinweis erzeugen **********************************************
//****************************************************************************
char * makeSourceId(const char *pchSource)
{
	static char achSourceId[255+1 /*sizeof(Buchung.source)*/] = "";

	strcpy(achSourceId, config.achInpFileName);
	strcat(achSourceId, " [");
	strncat(achSourceId, pchSource, sizeof(achSourceId)-strlen(achSourceId)-1);
	strncat(achSourceId, " ", sizeof(achSourceId)-strlen(achSourceId)-1);
	strncat(achSourceId, SRC_ID_MAIN(), sizeof(achSourceId)-strlen(achSourceId)-1);
	strncat(achSourceId, " ", sizeof(achSourceId)-strlen(achSourceId)-1);
	strncat(achSourceId, SRC_ID(), sizeof(achSourceId)-strlen(achSourceId)-1);
	strncat(achSourceId, "]", sizeof(achSourceId)-strlen(achSourceId)-1);

	return achSourceId;
}

// remove $ and Varnames from CVS substituted Variables
static const char * makeCVSDesc(const char *pchText)
{
	static char achResult[256] = "";
	int iResult = 0;

	if(*pchText == '$') pchText++;		// $ ignorieren

	while(*pchText == ' ')
		pchText++;

	while(*pchText != ' ' && *pchText != '\0') // Erster Wort ignorieren
		pchText++;

	while(*pchText == ' ')
		pchText++;

	if(*pchText != '\0') 
		while (*pchText != '$' && *pchText != '\0')
		{
			achResult[iResult] = *pchText;
			iResult++;
			pchText++;
		}

	while(achResult[iResult-1] == ' ' && iResult > 0)
		iResult--;

	achResult[iResult] = '\0';

	debug_printf(dbg_in, "makeCVSDesc(\"%s\") = \"%s\"\n", pchText, achResult);

	return achResult;
}

const char * makeSourceDesc(const char *pchRcsFile, const char *pchRevision)
{
	static char achResult[256] = "";

	strcpy(achResult, makeCVSDesc(pchRcsFile));
	strcat(achResult, " (");
	strcat(achResult, makeCVSDesc(pchRevision));
	strcat(achResult, ")");

	debug_printf(dbg_in, "makeSourceDesc(\"%s\",\"%s\") = \"%s\"\n", pchRcsFile, pchRevision, achResult);

	return achResult;
}

//****************************************************************************
//***** Einen Satz der Datei lesen *******************************************
//****************************************************************************
/* static int fBufferPending = 0; */
int readFile(FILE * inpFile)
{
	int i;

	/*    if(fBufferPending)
	{
	fBufferPending = 0;
	debug_printf(dbg_file, "Returning previously read line: %s\n", achBuffer);
	return 0;
	}
	 */
	for(;;)
	{
		if(fgets(achBuffer, sizeof(achBuffer), inpFile) == NULL)
		{
			if(feof(inpFile))
				return 1; // Warning: No record

			debug_printf(dbg_file, "Error reading File\n");
			return -1;
		}

		if(strlen(achBuffer) == 0)		// Wenn leere Zeile
		{
			debug_printf(dbg_file, "Ignoring empty line\n");
			continue;
		}

		// Ignore newline
		if(achBuffer[strlen(achBuffer)-1] == '\n')
			achBuffer[strlen(achBuffer)-1] = '\0';

		// Ignore carriage return
		if(achBuffer[strlen(achBuffer)-1] == '\r')
			achBuffer[strlen(achBuffer)-1] = '\0';

		if(achBuffer[0] == '\0')		// Wenn zeile nur mit \n
		{
			debug_printf(dbg_file, "Ignoring empty line with newline\n");
			continue;
		}

		// Leere Zeilen ignorieren
		for(i = 0; i < strlen(achBuffer); i++)
		{
			if(achBuffer[i] != ' ')
			{
				debug_printf(dbg_file, "readFile: %s\n", achBuffer);
				return 0;
			}
		}

		debug_printf(dbg_file, "Ignoring empty line\n");
		continue;
	}

}

//****************************************************************************
//***** Einen Satz der Datei zurueckgehen ************************************
//****************************************************************************
/*
int seekFileBack()
{
	fBufferPending = -1;
	debug_printf(dbg_file, "seekFileBack: %s\n", achBuffer);

	return 0;
}
 */
