/***** fntxt2sql-mysql.c *****/

#include "fntxt2sql.h"

#include <string.h>
#include <stdlib.h>

#ifdef CONF_MYSQL	// Mysql konfiguriert
#include <mysql/mysql.h>

static MYSQL * pMysqlConn = NULL;

static void printMysqlError(MYSQL * conn)
{
	fprintf(stderr, "MySql Error %d (SqlState %s) %s\n",
			mysql_errno(conn), mysql_sqlstate(conn), mysql_error(conn));
}

//****************************************************************************
//***** Mysql initialisieren *************************************************
//****************************************************************************
int initMysql(const char * const pchHost, const char * const pchDbParm,
		const char * const pchUser, const char * const pchPassword)
{
	MYSQL *conn = NULL;
	const char *pchDb;
	int iPort, iCliFlags;

	conn = mysql_init(NULL);
	if(conn == NULL)
	{ fprintf(stderr, "Error in mysql_Init\n"); return -1; }

	if (!pchDbParm || ! *pchDbParm)
		pchDb = "dbFinance";
	else
		pchDb = pchDbParm;

	iPort = 0;
	iCliFlags = 0;
	pMysqlConn = mysql_real_connect(conn, pchHost, pchUser, pchPassword, pchDb,
			iPort, NULL, iCliFlags);
	if(pMysqlConn == NULL)
	{
		printMysqlError(conn); mysql_close(conn);
		fprintf(stderr, "\tTried Host \"%s\", User \"%s\", Passwd \"%s\", "
				"Db \"%s\", Port %d, Clientflag %d.\n",
				pchHost, pchUser, pchPassword, pchDb, iPort,
				iCliFlags);
		return -1;
	}

	if (mysql_set_character_set(pMysqlConn, "latin1") != 0)
	{
		fprintf(stderr, "mysql_set_character_set failed.\n");
	}

	return 0;
}

//****************************************************************************
//***** Mysql de-initialisieren **********************************************
//****************************************************************************
void termMysql(void)
{
	mysql_close(pMysqlConn);
	pMysqlConn = NULL;

	return;
}

//****************************************************************************
//***** SQL Statement ausfuehren *********************************************
//****************************************************************************
static int execSql(const char *pchStmt, MYSQL_RES ** ppResult)
{
	MYSQL_RES *pResult= NULL;
	int iRc;

	iRc = mysql_query(pMysqlConn, pchStmt);
	debug_printf(dbg_sql, "DebugSQL: RC=%d \"%s\"\n", iRc, pchStmt);

	if(iRc != 0)	// error
	{
		fprintf(stderr, "mysql_query returned %d\n\tStatement: \"%s\"\n\t",
				iRc, pchStmt);
		printMysqlError(pMysqlConn);
	}

	pResult = mysql_store_result(pMysqlConn);
	if( pResult == NULL &&
			mysql_field_count(pMysqlConn) != 0)	// SQL Statement should produce results
	{
		fprintf(stderr, "Error in mysql_store_result. ");
		printMysqlError(pMysqlConn);
		iRc = -1;
	}

	if(ppResult != 0)
	{
		if(iRc == 0)
			*ppResult = pResult;
		else
		{
			*ppResult = NULL;
			mysql_free_result(pResult);
		}
	}
	else
		if(pResult)
			mysql_free_result(pResult);

	return iRc;
}

//****************************************************************************
//***** Eine Zeile vom MySql Ergebnis lesen und zu Buchung konvertieren ******
//***** Return-Codes:
//*****      -1 Fehler
//*****       0 Keine Zeile mehr vorhanden
//*****      +1 Eine Zeile erfolgreich gelesen und konvertiert
//****************************************************************************
int fetchSqlBuchung(MYSQL_RES *pResult, Buchung *pBuchung)
{
	MYSQL_ROW row;
	static const int iExpFieldCount = 24;
	int iFieldCount;

	memset(pBuchung, '\0', sizeof(*pBuchung));

	iFieldCount = mysql_num_fields(pResult);
	if(iFieldCount != iExpFieldCount)
	{
		fprintf(stderr,
				"Wrong number of result values from mysql: %d instead of %d\n",
				iFieldCount, iExpFieldCount);
		return -1;
	}

	row = mysql_fetch_row(pResult);
	if(row == NULL)
	{
		if(mysql_errno(pMysqlConn) == 0)	// No more Records
			return 0;
		else
		{
			fprintf(stderr, "mysql_fetch_row error. ");
			printMysqlError(pMysqlConn);
			return -1;
		}
	}

	// Record successfully retrieved so now copy the values into buchung structure
	strncpy(pBuchung->orig_blz   , row[ 0] ? row[ 0] : "",
		sizeof(pBuchung->orig_blz   ));
	strncpy(pBuchung->orig_ktonr , row[ 1] ? row[ 1] : "",
		sizeof(pBuchung->orig_ktonr ));
	strncpy(pBuchung->datum      , row[ 2] ? row[ 2] : "",
		sizeof(pBuchung->datum      ));
	strncpy(pBuchung->valuta     , row[ 3] ? row[ 3] : "",
		sizeof(pBuchung->valuta     ));
	pBuchung->betrag = atof(row[ 4]);
	strncpy(pBuchung->waehrung   , row[ 5] ? row[ 5] : "",
		sizeof(pBuchung->waehrung   ));
	pBuchung->buchart = row[ 6][0];
	strncpy(pBuchung->buchungs_sl, row[ 7] ? row[ 7] : "",
		sizeof(pBuchung->buchungs_sl));
	strncpy(pBuchung->gv_code    , row[ 8] ? row[ 8] : "",
		sizeof(pBuchung->gv_code    ));
	strncpy(pBuchung->part_blz   , row[ 9] ? row[ 9] : "",
		sizeof(pBuchung->part_blz   ));
	strncpy(pBuchung->part_ktonr , row[10] ? row[10] : "",
		sizeof(pBuchung->part_ktonr ));
	strncpy(pBuchung->part_name1 , row[11] ? row[11] : "",
		sizeof(pBuchung->part_name1 ));
	strncpy(pBuchung->part_name2 , row[12] ? row[12] : "",
		sizeof(pBuchung->part_name2 ));
	strncpy(pBuchung->primanota  , row[13] ? row[13] : "",
		sizeof(pBuchung->primanota  ));
	strncpy(pBuchung->referenz   , row[14] ? row[14] : "",
		sizeof(pBuchung->referenz   ));
	strncpy(pBuchung->butext     , row[15] ? row[15] : "",
		sizeof(pBuchung->butext     ));
	strncpy(pBuchung->vzweck[0]  , row[16] ? row[16] : "",
		sizeof(pBuchung->vzweck[0]  ));
	strncpy(pBuchung->vzweck[1]  , row[17] ? row[17] : "",
		sizeof(pBuchung->vzweck[1]  ));
	strncpy(pBuchung->vzweck[2]  , row[18] ? row[18] : "",
		sizeof(pBuchung->vzweck[2]  ));
	strncpy(pBuchung->vzweck[3]  , row[19] ? row[19] : "",
		sizeof(pBuchung->vzweck[3]  ));
	strncpy(pBuchung->vzweck[4]  , row[20] ? row[20] : "",
		sizeof(pBuchung->vzweck[4]  ));
	strncpy(pBuchung->vzweck[5]  , row[21] ? row[21] : "",
		sizeof(pBuchung->vzweck[5]  ));
	strncpy(pBuchung->vzweck[6]  , row[22] ? row[22] : "",
		sizeof(pBuchung->vzweck[6]  ));
	strncpy(pBuchung->source     , row[23] ? row[23] : "",
		sizeof(pBuchung->source     ));

	return 1;
}

//****************************************************************************
//***** Satz in Mysql schreiben **********************************************
//****************************************************************************
static int processMysqlRecord(const Buchung buchung, int fWrite)
{
	int iRc = 0;
	MYSQL_RES * pResult = NULL;
	Buchung existBuch;
	int iCompResult;
	int fIsDuplicate = 0;

	// First of all try reading similar records from the Mysql Table
	iRc = execSql(getSelectSql(buchung), &pResult);
	if(iRc != 0) return iRc;

	while(1)
	{
		iRc = fetchSqlBuchung(pResult, &existBuch);
		if(iRc < 0)	// error
		{
			mysql_free_result(pResult);
			return iRc;
		}
		else if(iRc == 0) // no more rows
		{
			iCompResult = 0;	// Just for correct OUtput
			fIsDuplicate = 0;
			break;
		}
		else	// Row found
		{
			iCompResult = compareRecord(buchung, existBuch);
			debug_printf(dbg_sql, "Write-Compare: RC=%d\n", (int)iCompResult);
			if(iCompResult == 0)	// Error
			{
				mysql_free_result(pResult);
				return -66;
			}
			else if(iCompResult > 0)	// Record identical means Row found
			{
				fIsDuplicate = -1;
				break;
			}
			else // Records not identical means No Row found yet
				;	// continue with next possibly equal Record.
		}
	}
	mysql_free_result(pResult);

	if(!fIsDuplicate)	// Buchung noch nicht in der Datenbank
	{
		if(fWrite)
		{
			iRc = execSql(getInsertSql(buchung), NULL);

			if(iRc != 0)
			{
				fprintf(stderr, "Error %d inserting Booking\n", iRc);
				return iRc;
			}
		}
		printf("%sOk(%03i): %s/%s %s %c %10.2f %25s\n",
				fWrite ? "Buch" : "Check",
				iCompResult,
				buchung.orig_blz, buchung.orig_ktonr, buchung.datum,
				buchung.buchart ,buchung.betrag, buchung.vzweck[0]);
	}
	else	// Aehnliche Buchung bereits in der Datenbank
	{
		printf("%sIgn(%03i): %s/%s %s %c %10.2f %25s\n",
				fWrite ? "Buch" : "Check",
				iCompResult,
				buchung.orig_blz, buchung.orig_ktonr, buchung.datum,
				buchung.buchart ,buchung.betrag, buchung.vzweck[0]);
		iRc = 0;
	}

	return iRc;
}

int writeMysqlRecord(const Buchung buchung)
{
	return processMysqlRecord(buchung, 1);
}

int checkMysqlRecord(const Buchung buchung)
{
	return processMysqlRecord(buchung, 0);
}


//****************************************************************************
//***** Tabelle in Mysql anlegen *********************************************
//****************************************************************************
int createMysqlTable(const char *pchCreateSql)
{
	return execSql(pchCreateSql, NULL);
}

#endif // CONF_MYSQL
