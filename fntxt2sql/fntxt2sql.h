/***** fntxt2sql.h *****/

#include <stdio.h>

//****************************************************************************
//***** Debugging Help *******************************************************
//****************************************************************************
typedef struct _DebugInfo {
	FILE * dbgFile;
	enum _level {
		dbg_in = 1,
		dbg_out = 2,
		dbg_fld = 4,
		dbg_datum = 8,
		dbg_file = 16,
		dbg_sql = 32} debug_level;
} DebugInfo;
extern DebugInfo debugInfo;

#define debug_printf(dbg, a...) \
	if(debugInfo.debug_level & dbg) \
	{ \
		fprintf(debugInfo.dbgFile, ## a); \
		fflush(debugInfo.dbgFile); \
	}

//****************************************************************************
//***** Configuration ********************************************************
//****************************************************************************
typedef struct _Config {
	char achSqlTabName[255]; // Data for SQL Generation
	char achInpFileName[255]; // Data for Input File Handling
	char achMysqlHost[255];
	char achMysqlDatabase[255];
	char achMysqlUser[255];
	char achMysqlPassword[255];
	enum _printFormat
	{
		outpInit = 0,
		txtMultiLine = 1,
		txtSingleLine = 2,
		txtSql = 3,
		execMysql = 4,
		checkMysql = 5} printFormat; // Output Format
	enum {
		inpInit = 0,
		old_inpBtxTxt = 1,
		inpCsv     = 2,
		inpAqm     = 3,
		old_inpAqb = 4,
		inpBtx     = 5,
		inpAqbTran = 6,
		inpAqbBal  = 7,
		inpAqb6    = 8
		} inputFormat; // Input Format
	char achCodePage[20];
} Config;
extern Config config;

// Structure for Transactions
typedef struct {
	char orig_ktonr[10+1]; // CHAR(10)      NOT NULL
	char orig_blz[8+1]; // CHAR(8)       NOT NULL
	char buchart; // CHAR(1)       NOT NULL
	char datum[10+1]; // DATE          NOT NULL
	char valuta[10+1]; // DATE
	char waehrung[3+1]; // CHAR(3)       NOT NULL
	double betrag; // DECIMAL(9,2)  NOT NULL
	char buchungs_sl[3+1]; // CHAR(3)
	char referenz[27+1]; // VARCHAR(27)
	char gv_code[10+1]; // VARCHAR(10)
	char part_name1[27+1]; // VARCHAR(27)
	char part_name2[27+1]; // VARCHAR(27)
	char part_ktonr[34+1]; // CHAR(34)
	char part_blz[11+1]; // CHAR(11)
	char butext[27+1]; // VARCHAR(27)
	char primanota[4+1]; // CHAR(4)
	char vzweck[7][140+1]; // VARCHAR(140)
	char source[255+1]; // VARCHAR(255)
} Buchung;

extern char achBuffer[2048];

//****************************************************************************
//***** Functions from fntxt2sql.c *******************************************
//****************************************************************************
const char *SRC_ID_MAIN(void);

//****************************************************************************
//***** Functions from fntxt2sql-util.c **************************************
//****************************************************************************
void printNullableChar(const char Text);
void printNullableString(const char * pText);
int readFile(FILE * inpFile);
double makeBetrag(const char *pchBuffer);
char* makeDatum(const char *pchBuffer);
char* makePktDatum(const char *pchBuffer);
int compareRecord(const Buchung newBuch, const Buchung existBuch);
int writeRecord(const Buchung buchung);
void resetRecord(Buchung *pBuchung);
char * makeSourceId(const char *pchSource);
const char * makeSourceDesc(const char *pchRcsFile, const char *pchRevision);
int createTable(void);
int convertCP(char * pch, const char * pchSourceCp, const char *pchDestCp);

//****************************************************************************
//***** Functions from fntxt2sql-btx.c ***************************************
//****************************************************************************
int processBtxFile(FILE * file);

//****************************************************************************
//***** Functions from fntxt2sql-aqb6.c **********************************
//****************************************************************************
int processAqb6File(char *pchFileName);

//****************************************************************************
//***** Functions from fntxt2sql-csv.c ***************************************
//****************************************************************************
int processCsvRecord(char * pchBuffer);

//****************************************************************************
//***** Functions from fntxt2sql-aqm.c ***************************************
//****************************************************************************
int processAqmRecord(char * pchBuffer);

//****************************************************************************
//***** Functions from fntxt2sql-aqb-tran.c **********************************
//****************************************************************************
int processAqbTranRecord(char * pchBuffer);

//****************************************************************************
//***** Functions from fntxt2sql-aqb-bal.c ***********************************
//****************************************************************************
int processAqbBalRecord(char * pchBuffer);

//****************************************************************************
//***** Functions from fntxt2sql-hbci.c **************************************
//****************************************************************************
int processHbci(char * dirName);

//****************************************************************************
//***** Functions from fntxt2sql-mysql.c **************************************
//****************************************************************************
int initMysql(const char *pchHost, const char *pchDb, const char *pchUser,
	const char *pchPassword);
int writeMysqlRecord(const Buchung buchung);
int checkMysqlRecord(const Buchung buchung);
void termMysql(void);
int createMysqlTable(const char *pchCreateSql);

//****************************************************************************
//***** Functions from fnSql.c ***********************************************
//****************************************************************************
const char * const * getCreateSql(void);
const char * getInsertSql(const Buchung buchung);
const char * getSelectSql(const Buchung buchung);

