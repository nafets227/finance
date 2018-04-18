/***** fntxt2sql-hbci.c *****/


#include "fntxt2sql.h"

#ifdef CONF_HBCIPX
#include <paradox.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct _testblk {
	long lBuchZaehler;	//  lBuchZaehler integer,
	long lAutoInc; 		//  AUTOINC integer,
	long nZeile;		//  nZeile integer,
	char achText[29];	//  szText char(28),	
} TextBlk;

static const char *SRC_ID(void)
{
	static char achID[256] = "";

	if(achID[0] == '\0')
		strcpy(achID, makeSourceDesc(
				"$RCSfile: fntxt2sql-hbci.c,v $",
		"$Revision: 1.16 $)"));
	return achID;
}



static int hbci_open(pxdoc_t ** pxDoc, char *pchDirName, char *pchFileName)
{
	int iRc;
	char achFileName[255];

	*pxDoc = PX_new();
	if(pxDoc == NULL)
	{ fprintf(stderr, "Eror in PX_new pxBuchungDb\n"); return -1; }

	// Build pathname
	strcpy(achFileName, pchDirName);
	if(achFileName[strlen(achFileName)-1] != '/')
		strcat(achFileName, "/");
	strcat(achFileName, pchFileName);

	// Open Paradox file
	iRc = PX_open_file(*pxDoc, achFileName);
	if(iRc != 0)
	{ fprintf(stderr, "Eror %d in px_open_file(\"%s\")\n", iRc, achFileName); return iRc; }

	return 0;

}

static void hbci_close(pxdoc_t ** pxDoc)
{
	PX_close(*pxDoc);
	PX_delete(*pxDoc);
	*pxDoc = NULL;

	return;
}

static int hbci_openIndexedDb(pxdoc_t ** pxDoc, pxdoc_t ** pxIndex, char *pchDirName, char *pchFileName)
{
	int iRc;
	char achFileName [255];

	strcpy(achFileName, pchFileName);
	strcat(achFileName, ".db");
	iRc = hbci_open(pxDoc, pchDirName, achFileName);
	if(iRc != 0) return iRc;
	/*	
	strcpy(achFileName, pchFileName);
	strcat(achFileName, ".px");
	iRc = hbci_open(pxIndex, pchDirName, achFileName);
	if(iRc != 0) return iRc;

	iRc = PX_read_primary_index(*pxIndex);
	if(iRc != 0)
		{ fprintf(stderr, "Eror %d in px_read_primary_index(\"%s\")\n", iRc, achFileName); return iRc; }

	iRc = PX_add_primary_index(*pxDoc, *pxIndex);
	if(iRc != 0)
		{ fprintf(stderr, "Eror %d in px_add_primary_index(\"%s\")\n", iRc, achFileName); return iRc; }
	 */	
	return 0;
}

static void hbci_closeIndexedDb(pxdoc_t ** pxDoc, pxdoc_t **pxIndex)
{
	hbci_close(pxDoc);
	//	hbci_close(pxIndex);

	return;  
}

static char * hbci_makeDatum(long lDatum)
{
	static char achDatum[11] = "0000-00-00";
	int year, month, day;
	struct tm *ptmDatum;

	ptmDatum = localtime(&lDatum);
	year = ptmDatum->tm_year + 1900;
	month = ptmDatum->tm_mon+1;
	day = ptmDatum->tm_mday;

	achDatum[ 0] = (year  / 1000) % 10 + '0';
	achDatum[ 1] = (year  /  100) % 10 + '0';
	achDatum[ 2] = (year  /   10) % 10 + '0';
	achDatum[ 3] = (year  /    1) % 10 + '0';

	achDatum[ 5] = (month /   10) % 10 + '0';
	achDatum[ 6] = (month /    1) % 10 + '0';

	achDatum[ 8] = (day   /   10) % 10 + '0';
	achDatum[ 9] = (day   /    1) % 10 + '0';

	return achDatum;
}

//////////////////////////////////////////////////////////////////////////////
// Compare two elements of type TextBlk by lBuchZahler, lAutoInc
static int hbci_comp_Textblk(const void *pv1, const void*pv2)
{
	const TextBlk *p1 = pv1, *p2 = pv2;

	if(p1->lBuchZaehler < p2->lBuchZaehler)
		return -1;
	if(p1->lBuchZaehler > p2->lBuchZaehler)
		return +1;

	if(p1->lAutoInc < p2->lAutoInc)
		return -1;

	if(p1->lAutoInc > p2->lAutoInc)
		return +1;

	return 0;
}

// Compare two elements of type TextBlk by lBuchZahler only
static int hbci_comp_Textblk2(const void *pv1, const void*pv2)
{
	const TextBlk *p2 = pv2;
	const long *p1 = pv1;

	if(*p1  < p2->lBuchZaehler)
		return -1;
	if(*p1  > p2->lBuchZaehler)
		return +1;

	return 0;
}

//////////////////////////////////////////////////////////////////////////////





#define HBCI_ERR_WRONG_TYPE(value, expected)	 \
	fprintf(stderr, #value " (%d) != " #expected " (%d)\n",(int)(value->type), (int)expected)

static int hbci_convertBuchung(pxval_t **pxValues, Buchung *pBuchung, 
		TextBlk *pTextBlk, long lCountTextBlk)
{
	static const int iVZweckCnt = 
		sizeof(pBuchung->vzweck) / sizeof(pBuchung->vzweck[0]);
	long lBuchungZaehler;
	int iVZweckNr = 0;
	int iPos;
	TextBlk *pTextBlkAct = NULL;

	memset(pBuchung, '\0', sizeof(*pBuchung));
	pBuchung->buchart = 'B';
	strcpy(pBuchung->source, makeSourceId(SRC_ID()));


	// szKtoNr char(16),
	if(pxValues[0]->type != pxfAlpha || pxValues[0]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[0], pxfAlpha);  return -1;}
	debug_printf(dbg_fld, "KtoNr: %s\n", pxValues[0]->value.str.val);
	strncpy(pBuchung->orig_ktonr, pxValues[0]->value.str.val, sizeof(pBuchung->orig_ktonr));

	// szBLZ char(10),
	if(pxValues[1]->type != pxfAlpha || pxValues[1]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[1], pxfAlpha);  return -1;}
	debug_printf(dbg_fld, "BLZ: %s\n", pxValues[1]->value.str.val);
	strncpy(pBuchung->orig_blz, pxValues[1]->value.str.val, sizeof(pBuchung->orig_blz));

	// lAuszZaehler integer,
	// Field is ignored

	// AUTOINC integer,
	if(pxValues[3]->type != pxfAutoInc || pxValues[3]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[3], pxfAutoInc);  return -1;}
	debug_printf(dbg_fld, "AUTOINC: %d\n", (int)pxValues[3]->value.lval);
	lBuchungZaehler = pxValues[3]->value.lval;

	// nBuchNr integer,
	if(pxValues[4]->type != pxfShort || pxValues[4]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[4], pxfShort);  return -1;}
	debug_printf(dbg_fld, "BuchNr: %d\n", (int)pxValues[4]->value.lval);
	sprintf(pBuchung->referenz, "%d", (int)pxValues[4]->value.lval);

	// lBuchDatum integer,
	if(pxValues[5]->type != pxfLong || pxValues[5]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[5], pxfLong);  return -1;}
	debug_printf(dbg_fld, "BuchDatum: %d\n", (int)pxValues[5]->value.lval);
	
	/* Wenn der Datumswert 0 ist wird +1 zuruekgegeben und
	 * dadurch die Buchung ignoriert! 
	 */
	if(pxValues[5]->value.lval == 0)
		return 1;
	
	strncpy(pBuchung->datum, 
			hbci_makeDatum(pxValues[5]->value.lval),
			sizeof(pBuchung->datum));

	// lWertDatum integer,
	if(pxValues[6]->type != pxfLong || pxValues[6]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[6], pxfLong);  return -1;}
	debug_printf(dbg_fld, "WertDatum: %d\n", (int)pxValues[6]->value.lval);
	strncpy(pBuchung->valuta, 
			hbci_makeDatum(pxValues[6]->value.lval),
			sizeof(pBuchung->valuta));

	// szPostenNr char(10),
	if(pxValues[7]->type != pxfAlpha)
	{ HBCI_ERR_WRONG_TYPE(pxValues[7], pxfAlpha);  return -1;}
	debug_printf(dbg_fld, "PNR: %s\n", pxValues[7]->value.str.val);
	if(pxValues[7]->isnull == 0)
		strncpy(pBuchung->primanota, pxValues[7]->value.str.val, sizeof(pBuchung->primanota));
	else
		*pBuchung->primanota = '\0';

	// szBuchText char(28),
	if(pxValues[8]->type != pxfAlpha || pxValues[8]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[8], pxfAlpha);  return -1;}
	debug_printf(dbg_fld, "BuText: %s\n", pxValues[8]->value.str.val);
	strncpy(pBuchung->butext, pxValues[8]->value.str.val, sizeof(pBuchung->butext));

	// lBetrag integer,
	if(pxValues[9]->type != pxfLong || pxValues[9]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[9], pxfLong);  return -1;}
	debug_printf(dbg_fld, "Betrag: %d\n", (int)pxValues[9]->value.lval);
	pBuchung->betrag = (double)pxValues[9]->value.lval;
	pBuchung->betrag /= 100;

	// bBuchSoll integer,
	if(pxValues[10]->type != pxfShort || pxValues[10]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[10], pxfShort);  return -1;}
	debug_printf(dbg_fld, "SollKZ: %d\n", (int)pxValues[10]->value.lval);
	if(pxValues[10]->value.lval != 0)
		pBuchung->betrag *= -1;

	// lReserveBuchung integer,
	// field is ignored

	//////////////////////////////////////////////////////////////////////////
	// Nun die Verwendungszwecke aus der im Speicher gehaltenen TextBlk

	pTextBlkAct = bsearch(
			&lBuchungZaehler, pTextBlk, lCountTextBlk, 
			sizeof(*pTextBlk), hbci_comp_Textblk2);

	// Find first matching record
	while(pTextBlkAct > pTextBlk &&
			(pTextBlkAct-1)->lBuchZaehler == lBuchungZaehler)
		pTextBlkAct--;

	while (pTextBlkAct != NULL &&
			pTextBlkAct < pTextBlk + lCountTextBlk &&
			pTextBlkAct ->lBuchZaehler == lBuchungZaehler && 
			iVZweckNr < sizeof(pBuchung->vzweck) / sizeof(pBuchung->vzweck[0]))
	{
		if(pTextBlkAct->nZeile == 10000) // This number marks a original 
		{			     				 // currency text during Euro conversion period.
			strncpy(pBuchung->waehrung, pTextBlkAct->achText, sizeof(pBuchung->waehrung));
			pTextBlkAct++;
			continue;
		}
		
		// Sonderlocke: Wenn EUR und nur Nummern in VZweck1 dann ignorieren!
		if(iVZweckNr == 0 && !memcmp(pTextBlkAct->achText, "EUR", 3))
		{
			int fIsSpecial = -1;
			for(iPos = 3; 
					iPos < sizeof(pTextBlkAct->achText) &&
						pTextBlkAct->achText[iPos] != '\0'; 
					iPos++)
				if( (pTextBlkAct->achText[iPos] >= '0' && pTextBlkAct->achText[iPos] <= '9') ||
						pTextBlkAct->achText[iPos] == '.' ||
						pTextBlkAct->achText[iPos] == ',' ||
						pTextBlkAct->achText[iPos] == ' ')
					;	// Character im Spezial-Set
				else
				{
					fIsSpecial = 0;
					break;
				}
			if(fIsSpecial)
			{
				debug_printf(dbg_fld, "Ignoring Special EUR VZweck \"%s\"\n",
						pTextBlkAct->achText);
				pTextBlkAct++;
				continue;
			}
		}
			
		debug_printf(dbg_fld, "VZweck[%d]: \"%s\" (%d)\n", 
				(int)iVZweckNr, pTextBlkAct->achText, (int)pTextBlkAct->nZeile); 
		strncpy(pBuchung->vzweck[iVZweckNr], 
				pTextBlkAct->achText, 
				sizeof(pBuchung->vzweck[iVZweckNr]));
		pTextBlkAct++;
		iVZweckNr++;
	}
	
	// Sonderbehandlung: Wenn der Text "Bankverbdg.: (Blz./KtoNr.)" erscheint, 
	// werden die naehsten 2 VZwecks besonders interpretiert.
	// " Bankverbdg.: (Blz./KtoNr.)" "       70020270 / 780762394" "    HYPOVEREINSBANK HYPOTH."
	for(iVZweckNr = 0; iVZweckNr < iVZweckCnt; iVZweckNr++)
		if(strcmp(pBuchung->vzweck[iVZweckNr], "Bankverbdg.: (Blz./KtoNr.)") == 0)
		{
			if(iVZweckNr+1 < iVZweckCnt &&
			   strlen(pBuchung->part_blz) == 0)
				strncpy(pBuchung->part_blz, 
						&pBuchung->vzweck[iVZweckNr+1][0], 
						sizeof(pBuchung->part_blz)-1);
			if(iVZweckNr+1 < iVZweckCnt &&
			   strlen(pBuchung->part_ktonr) == 0 )
				strncpy(pBuchung->part_ktonr,
						&pBuchung->vzweck[iVZweckNr+1][11], 
						sizeof(pBuchung->part_ktonr)-1);
			if(iVZweckNr+2 < iVZweckCnt &&
			   strlen(pBuchung->part_name1) == 0 )
				strncpy(pBuchung->part_name1,
						pBuchung->vzweck[iVZweckNr+2], 
						sizeof(pBuchung->part_name1)-1);
			if(iVZweckNr+3 < iVZweckNr &&
			   strlen(pBuchung->part_name2) == 0 )
				strncpy(pBuchung->part_name2,
						pBuchung->vzweck[iVZweckNr+3], 
						sizeof(pBuchung->part_name2)-1);
			debug_printf(dbg_fld, "Special VZweck detected: %8s/%10s \"%27s\" \"%27s\"\n",
					pBuchung->part_blz, pBuchung->part_ktonr, pBuchung->part_name1, pBuchung->part_name2);
			pBuchung->vzweck[iVZweckNr][0] = '\0';
			pBuchung->vzweck[iVZweckNr+1][0] = '\0';
			pBuchung->vzweck[iVZweckNr+2][0] = '\0';
			pBuchung->vzweck[iVZweckNr+3][0] = '\0';
		}

	return 0;
}

static int hbci_convertTextblk(pxval_t **pxValues, TextBlk *pTextBlk)
{
	memset(pTextBlk, '\0', sizeof(*pTextBlk));

	//  szKtoNr char(16) - Field ignored
	//  szBLZ char(10)   - Field ignored
	//  lAuszZaehler integer - Field ignored
	//  lBuchZaehler integer,
	if(pxValues[3]->type != pxfLong || pxValues[3]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[3], pxfLong);  return -1;}
	debug_printf(dbg_fld, "lBuchZaehler: %d\n", (int)pxValues[3]->value.lval);
	pTextBlk->lBuchZaehler = pxValues[3]->value.lval;

	//  AUTOINC integer,
	if(pxValues[4]->type != pxfAutoInc || pxValues[4]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[4], pxfAutoInc);  return -1;}
	debug_printf(dbg_fld, "lAutoInc: %d\n", (int)pxValues[4]->value.lval);
	pTextBlk->lAutoInc = pxValues[4]->value.lval;

	//  nZeile integer,
	if(pxValues[5]->type != pxfShort || pxValues[5]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[5], pxfShort);  return -1;}
	debug_printf(dbg_fld, "nZeile: %d\n", (int)pxValues[5]->value.lval);
	pTextBlk->nZeile = pxValues[5]->value.lval;

	//  szText char(28),	
	if(pxValues[6]->type != pxfAlpha || pxValues[6]->isnull != 0)
	{ HBCI_ERR_WRONG_TYPE(pxValues[6], pxfAlpha);  return -1;}
	debug_printf(dbg_fld, "szText: %s\n", pxValues[6]->value.str.val);
	strncpy(pTextBlk->achText, pxValues[6]->value.str.val, sizeof(pTextBlk->achText)-1);

	return 0;
}

int processHbci(char * dirName)
{
	pxdoc_t * pxBuchung = 0, *pxBuchungIndex = 0, *pxTextBlk = 0, *pxTextBlkIndex = 0;
	long lRecCountBuchung = 0, lRecCountTextBlk = 0;
	pxval_t ** pxValues = 0;
	int iRc, iRecNo;
	Buchung buchung;
	TextBlk *textBlk = NULL;


	PX_boot();

	iRc = hbci_openIndexedDb(&pxBuchung, &pxBuchungIndex, dirName, "buchung");
	if(iRc != 0) return iRc;

	iRc = hbci_openIndexedDb(&pxTextBlk, &pxTextBlkIndex, dirName, "textblk");
	if(iRc != 0) return iRc;


	// @TODO auszug Datenbank behandeln, die die Salden enthaelt

	lRecCountTextBlk  = PX_get_num_records(pxTextBlk);
	textBlk = calloc(lRecCountTextBlk, sizeof(*textBlk));
	if(textBlk == NULL)
	{ fprintf(stderr, "Not enough memory for textBlk\n"); return -1;}

	for(iRecNo = 1; iRecNo < lRecCountTextBlk; iRecNo++)
	{
		debug_printf(dbg_file, "reading TextBlk.db record #%d\n", iRecNo);
		pxValues = PX_retrieve_record(pxTextBlk, iRecNo);

		hbci_convertTextblk(pxValues, textBlk+(iRecNo-1));	
	}
	qsort(textBlk, PX_get_num_records(pxTextBlk), sizeof(*textBlk), hbci_comp_Textblk);


	lRecCountBuchung  = PX_get_num_records(pxBuchung);
	for(iRecNo = 1; iRecNo < lRecCountBuchung; iRecNo++)
	{
		debug_printf(dbg_file, "reading buchung.db record #%d\n", iRecNo);
		pxValues = PX_retrieve_record(pxBuchung, iRecNo);

		iRc = hbci_convertBuchung(pxValues, &buchung, textBlk, lRecCountTextBlk);
		if(iRc == 0)
		{
			iRc = writeRecord(buchung);
			if(iRc != 0)
				break;
		}
		else if(iRc != 1)	// RC +1 heisst Buchung ignorieren
			break;
	}

	hbci_closeIndexedDb(&pxBuchung, &pxBuchungIndex);
	hbci_closeIndexedDb(&pxTextBlk, &pxTextBlkIndex);

	PX_shutdown();

	return iRc;
}
#endif // CONF_HBCIPX
