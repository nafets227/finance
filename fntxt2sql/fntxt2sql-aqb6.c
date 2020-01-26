/***** fntxt2sql-aqb6.c *****/

#include "fntxt2sql.h"

#ifdef CONF_AQB6
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aqbanking6/aqbanking/banking.h>
#include <gwenhywfar/syncio_file.h>
#include <gwenhywfar/gui.h>

static const char *SRC_ID(void)
{
	static char achID[256] = "";

	if (achID[0] == '\0')
		strcpy(achID, makeSourceDesc(
				"$RCSfile: fntxt2sql-aqb6.c,v $",
		"$Revision: 1.2 $)"));
	return achID;
}

//****************************************************************************
//***** Helper functions *****************************************************
//****************************************************************************
#define save_strcpy(dst, src) \
	save_strncpy(dst, src, sizeof(dst)-1)
static char * save_strncpy(char *dst, const char *src, size_t size)
{
	if(src == NULL || *src == '\0')
	{
		*dst = 0;
		return dst;
	}
	else
		return strncpy(dst,src,size);
}

static const char * makeDatumGwen(const GWEN_DATE *date)
{
	return makeDatum(GWEN_Date_GetString(date));
}

//****************************************************************************
//***** read File with AqBanking *********************************************
//****************************************************************************
static AB_IMEXPORTER_CONTEXT *readContext(const char *ctxFile)
{
	AB_IMEXPORTER_CONTEXT *ctx;
	GWEN_SYNCIO *sio;
	GWEN_DB_NODE *dbCtx;

	if (ctxFile == NULL || *ctxFile == '\0')
	{
		sio=GWEN_SyncIo_File_fromStdin();
		GWEN_SyncIo_AddFlags(sio,
					GWEN_SYNCIO_FLAGS_DONTCLOSE |
					GWEN_SYNCIO_FILE_FLAGS_READ);
	}
	else
	{
		sio=GWEN_SyncIo_File_new(ctxFile, GWEN_SyncIo_File_CreationMode_OpenExisting);
		GWEN_SyncIo_AddFlags(sio, GWEN_SYNCIO_FILE_FLAGS_READ);
		if(GWEN_SyncIo_Connect(sio) < 0)
		{
			fprintf(stderr, "Cannot open Input File %s\n", ctxFile);
			GWEN_SyncIo_free(sio);
			return NULL;
		}
	}

	/* actually read */
	dbCtx=GWEN_DB_Group_new("context");
	if (GWEN_DB_ReadFromIo(dbCtx, sio,
			GWEN_DB_FLAGS_DEFAULT | GWEN_PATH_FLAGS_CREATE_GROUP) < 0)
		{
		fprintf(stderr, "Error reading Input file %s\n", ctxFile);
		GWEN_DB_Group_free(dbCtx);
		GWEN_SyncIo_Disconnect(sio);
		GWEN_SyncIo_free(sio);
		return NULL;
		}
	GWEN_SyncIo_Disconnect(sio);
	GWEN_SyncIo_free(sio);

	ctx=AB_ImExporterContext_fromDb(dbCtx);
	if (!ctx) {
		fprintf(stderr, "No context in Aqb6 Input file %s\n", ctxFile);
		GWEN_DB_Group_free(dbCtx);
		return NULL;
	}

	GWEN_DB_Group_free(dbCtx);
	return ctx;
}

//****************************************************************************
//***** AqBanking6 Saldo behandeln *******************************************
//****************************************************************************
static int processAqb6Bal(const AB_IMEXPORTER_ACCOUNTINFO *iea, const AB_BALANCE *b)
{
	Buchung buchung;

	debug_printf(dbg_in, "processAqb6Bal.\n");

	memset(&buchung, '\0', sizeof(buchung));

	switch (AB_Balance_GetType(b))
	{
	case AB_Balance_TypeNoted: // @TODO: check which balance to take
		buchung.buchart = 'E';
		break;
//	case AB_Balance_TypeBooked:
//		buchung.buchart = 'F';
//		break;
	default:
		debug_printf(dbg_in, "Ignoring Aqb6 Balance type %d\n", AB_Balance_GetType(b))
		return 0; // Ignore this Balance
	}

	save_strcpy(buchung.orig_blz, AB_ImExporterAccountInfo_GetBankCode(iea));
	save_strcpy(buchung.orig_ktonr, AB_ImExporterAccountInfo_GetAccountNumber(iea));
	const char *pchWaehrung = AB_ImExporterAccountInfo_GetCurrency(iea);
	if (pchWaehrung == NULL)
	{
		debug_printf(dbg_fld, "Currency is NULL, trying SubAccount as workaround.\n")
		pchWaehrung = AB_ImExporterAccountInfo_GetSubAccountId(iea);
	}
	if (pchWaehrung != NULL)
		save_strcpy(&buchung.waehrung[0], AB_ImExporterAccountInfo_GetCurrency(iea));

	save_strcpy(buchung.datum, makeDatumGwen(AB_Balance_GetDate(b)));
	const AB_VALUE * v = AB_Balance_GetValue(b);
	buchung.betrag = AB_Value_GetValueAsDouble(v);

	save_strcpy(buchung.source, makeSourceId(SRC_ID()));

	debug_printf(dbg_fld, "Saldo %+9.2f %s %s %s %s\n",
			buchung.betrag, buchung.waehrung, buchung.datum,
			buchung.gv_code, buchung.primanota);

	return writeRecord(buchung);
}

//****************************************************************************
//***** AqBanking6 Transaktion behandeln *************************************
//****************************************************************************
static int processAqb6Tran(const AB_TRANSACTION *t)
{
	Buchung buchung;

	debug_printf(dbg_in, "processAqb6Tran.\n");

	memset(&buchung, '\0', sizeof(buchung));

	buchung.buchart = 'B';

	save_strcpy(buchung.orig_blz, AB_Transaction_GetLocalBankCode(t));
	save_strcpy(buchung.orig_ktonr, AB_Transaction_GetLocalAccountNumber(t));

	save_strcpy(buchung.part_name1, AB_Transaction_GetRemoteName(t));
	save_strcpy(buchung.part_blz,   AB_Transaction_GetRemoteBankCode(t));
	save_strcpy(buchung.part_ktonr, AB_Transaction_GetRemoteAccountNumber(t));

	const AB_VALUE *v = AB_Transaction_GetValue(t);
	buchung.betrag = AB_Value_GetValueAsDouble(v);
	save_strcpy(buchung.waehrung, AB_Value_GetCurrency(v));
	save_strcpy(buchung.datum, makeDatumGwen(AB_Transaction_GetDate(t)));
	save_strcpy(buchung.valuta, makeDatumGwen(AB_Transaction_GetValutaDate(t)));

	save_strcpy(buchung.vzweck[0], AB_Transaction_GetPurpose(t));

	save_strcpy(buchung.source, makeSourceId(SRC_ID()));

	debug_printf(dbg_fld, "Buchung %+9.2f %s %s %s %s\n",
			buchung.betrag, buchung.waehrung, buchung.datum,
			buchung.gv_code, buchung.primanota);

	return writeRecord(buchung);
}

//****************************************************************************
//***** AqBanking6 Datei behandeln *******************************************
//****************************************************************************
int processAqb6File(char *pchFileName)
{
	AB_BANKING *ab = NULL;
	AB_IMEXPORTER_CONTEXT *ctx = NULL;
	GWEN_GUI *gui = NULL;

	gui=GWEN_Gui_new();
	GWEN_Gui_AddFlags(gui, GWEN_GUI_FLAGS_NONINTERACTIVE);
    GWEN_Gui_SetGui(gui);

	ab=AB_Banking_new("nafets227/finance", NULL, 0);
	if (ab == NULL)
	{
		fprintf(stderr, "Could not init AqBanking6 (new)\n");
		return -1;
	}

	if (AB_Banking_Init(ab))
	{
		fprintf(stderr, "Could not init AqBanking6 (_Init)\n");
		AB_Banking_free(ab);
		return -1;
	}

	ctx = readContext(pchFileName);
	if (ctx == NULL)
	{
		AB_Banking_Fini(ab);
		AB_Banking_free(ab); ab = NULL;
		return -1;
	}

	for (AB_IMEXPORTER_ACCOUNTINFO *iea =
			AB_ImExporterContext_GetFirstAccountInfo(ctx);
	    iea;
	    iea=AB_ImExporterAccountInfo_List_Next(iea))
	{ // for all accounts
		const char *iban = AB_ImExporterAccountInfo_GetIban(iea);
		debug_printf(dbg_in, "processsAqb6Acc: %s\n", iban);
		for (const AB_TRANSACTION *t =
				AB_ImExporterAccountInfo_GetFirstTransaction(iea, 0, 0);
			t;
			t=AB_Transaction_List_Next(t))
		{ // for all transactions of account
			if (processAqb6Tran(t))
				return -1;
		} // for all transactionds of account

		for(const AB_BALANCE *b =
				AB_ImExporterAccountInfo_GetFirstBalance(iea);
			b;
			b=AB_Balance_List_Next(b) )
		{ // for all balances of account
			if (processAqb6Bal(iea, b))
				return -1;
		} // for all balances of account
	} // for all accounts

	AB_ImExporterContext_free(ctx);
	AB_Banking_Fini(ab);
	AB_Banking_free(ab); ab = NULL;

	return 0;
}
#endif // CONF_AQB6
