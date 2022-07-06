/*****  fntxt2sql.c,v *****/

#include "fntxt2sql.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * Allgemeine ToDos:
 * @TODO Return Code setzen bei fehlenden Aktions-Paramtern (z.B. -mysql)
 * @TODO Datenbank und internes Handling auf UniCode umstellen (Prio B)
 * @TODO Delete Aktion einfuehren fuer -sql und -mysql (analog create table)
 * @TODO Indizes bei Create Table mit erzeugen
 * @TODO Kategorien in Tabelle einfuehren und Default-maessig belegen
 * @TODO Alte Formate einschliessen
 * @TODO Update in Datenbank wenn mehr Informationen als bisher in DB
 */


// Common Variables for all Source-Files
DebugInfo debugInfo;
Config config;
// Buchung buchung;

char achBuffer[2048];

// Source ID
const char *SRC_ID_MAIN(void)
{
	static char achUtilID[256] = "";

	if(achUtilID[0] == '\0')
		strcpy(achUtilID, makeSourceDesc(
				"$RCSfile: fntxt2sql.c,v $",
		"$Revision: 1.30 $)"));
	return achUtilID;
}
//****************************************************************************
//***** Eine Datei einlesen und alle Saetze behandeln ************************
//****************************************************************************
int processFile(char *achInpFileName)
{
	int iRc;
	FILE * inpFile = 0;

	if (config.inputFormat != inpAqb6)
	{
		if(achInpFileName[0] == '\0')
			inpFile = stdin;
		else
		{
			if( (inpFile = fopen(achInpFileName, "r")) == NULL)
			{
				printf("Error %d opening File %s\n",
						errno, achInpFileName);
				return -1;
			}

		}
	}
	if(config.inputFormat == inpBtx)
		iRc = processBtxFile(inpFile);
#ifdef CONF_AQB6
	else if (config.inputFormat == inpAqb6)
		iRc = processAqb6File(achInpFileName);
#endif // CONF_AQB6
	else
	{
		while((iRc = readFile(inpFile)) == 0)
		{
			switch(config.inputFormat)
			{
			case inpCsv:
				iRc = processCsvRecord(achBuffer);
				break;
			case inpAqm:
				iRc = processAqmRecord(achBuffer);
				break;
			case inpAqbTran:
				iRc = processAqbTranRecord(achBuffer);
				break;
			case inpAqbBal:
				iRc = processAqbBalRecord(achBuffer);
				break;
			default:
				fprintf(stderr, "Unknown input Format %d\n", config.inputFormat);
				iRc = -1;
			}

			if(iRc != 0)
				return iRc;
		} // next record

		if(iRc != 1) // EOF
			return iRc;
		iRc = 0;
	}

	debug_printf(dbg_file, "Reached End Of File\n");
	//	{if(((int)debugInfo.debug_level) & (dbg_file)) fprintf(debugInfo.dbgFile, "Reached End Of File\n");}

	return iRc;

}


//****************************************************************************
//***** Haupt-Prozedur *******************************************************
//****************************************************************************
int main(int argc, char *argv[])
{
	int iRc = 0;
	int iActArg;

	enum { process=1, createTab=2, procHbci = 3 } action = 1;

	//***** Initialise Debug Info ************************************************
	debugInfo.dbgFile = stderr;
	//#ifdef DEBUG
	//    debugInfo.debug_level = 0xFFFFFFFF;
	//#else
	debugInfo.debug_level = 0;
	//#endif

	//***** Initialise config ****************************************************
	config.printFormat = txtMultiLine;
	config.inputFormat = inpInit;
	config.achInpFileName[0] = '\0';
	strcpy(config.achSqlTabName, "fn_entry");
	strcpy(config.achCodePage, "UTF-8");

	//***** Parse Options ********************************************************
	for(iActArg=1; iActArg < argc; iActArg++)
	{
		//***** Input Parameters *****
		if(     !strcasecmp(argv[iActArg], "/CSV") ||
				!strcasecmp(argv[iActArg], "-CSV")     )
			config.inputFormat = inpCsv;

		else if(!strcasecmp(argv[iActArg], "/AQM") ||
				!strcasecmp(argv[iActArg], "-AQM")     )
			config.inputFormat = inpAqm;

		else if(!strcasecmp(argv[iActArg], "/AQB-TRAN") ||
				!strcasecmp(argv[iActArg], "-AQB-TRAN")     )
			config.inputFormat = inpAqbTran;

		else if(!strcasecmp(argv[iActArg], "/AQB-BAL") ||
				!strcasecmp(argv[iActArg], "-AQB-BAL")     )
			config.inputFormat = inpAqbBal;

#ifdef CONF_AQB6
		else if(!strcasecmp(argv[iActArg], "/AQB6") ||
				!strcasecmp(argv[iActArg], "-AQB6")     )
			config.inputFormat = inpAqb6;
#endif // CONF_AQB6

		else if(!strcasecmp(argv[iActArg], "/BTX") ||
				!strcasecmp(argv[iActArg], "-BTX")     )
			config.inputFormat = inpBtx;

		//***** Output Parameters *****
		else if(!strcasecmp(argv[iActArg], "/1") ||
				!strcasecmp(argv[iActArg], "-1")     )
			config.printFormat = txtSingleLine;

		else if(!strcasecmp(argv[iActArg], "/M") ||
				!strcasecmp(argv[iActArg], "-M")     )
			config.printFormat = txtMultiLine;

		else if(!strcasecmp(argv[iActArg], "/SQL") ||
				!strcasecmp(argv[iActArg], "-SQL")       )
			config.printFormat = txtSql;

#ifdef CONF_MYSQL
		else if(!strcasecmp(argv[iActArg], "/MYSQL") ||
				!strcasecmp(argv[iActArg], "-MYSQL")     )
			config.printFormat = execMysql;

		else if(!strcasecmp(argv[iActArg], "/MYSQLCHECK") ||
				!strcasecmp(argv[iActArg], "-MYSQLCHECK")     )
			config.printFormat = checkMysql;
		else if(!strcasecmp(argv[iActArg], "/MYSQL_HOST") ||
				!strcasecmp(argv[iActArg], "-MYSQL_HOST")     )
		{
			if(iActArg+1 >= argc)
			{
				printf( "Error: /MYSQL_HOST parameter given "
				"without value\n");
				return -1;
			}
			strcpy(config.achMysqlHost, argv[iActArg+1]);
			iActArg++;
		}
		else if(!strcasecmp(argv[iActArg], "/MYSQL_DATABASE") ||
				!strcasecmp(argv[iActArg], "-MYSQL_DATABASE")     )
		{
			if(iActArg+1 >= argc)
			{
				printf( "Error: /MYSQL_DATABASE parameter given "
				"without value\n");
				return -1;
			}
			strcpy(config.achMysqlDatabase, argv[iActArg+1]);
			iActArg++;
		}
		else if(!strcasecmp(argv[iActArg], "/MYSQL_USER") ||
				!strcasecmp(argv[iActArg], "-MYSQL_USER")     )
		{
			if(iActArg+1 >= argc)
			{
				printf( "Error: /MYSQL_USER parameter given "
				"without value\n");
				return -1;
			}
			strcpy(config.achMysqlUser, argv[iActArg+1]);
			iActArg++;
		}
		else if(!strcasecmp(argv[iActArg], "/MYSQL_PASSWORD") ||
				!strcasecmp(argv[iActArg], "-MYSQL_PASSWORD")     )
		{
			if(iActArg+1 >= argc)
			{
				printf( "Error: /MYSQL_PASSWORD parameter given "
				"without value\n");
				return -1;
			}
			strcpy(config.achMysqlPassword, argv[iActArg+1]);
			iActArg++;
		}

#endif

		//***** Actions *****
#ifdef CONF_HBCIPX
		else if(!strcasecmp(argv[iActArg], "/HBCI") ||
				!strcasecmp(argv[iActArg], "-HBCI")     )
			action = procHbci;
#endif // CONF_HBCIPX

		else if(!strcasecmp(argv[iActArg], "/CRE") ||
				!strcasecmp(argv[iActArg], "-CRE")     )
			action = createTab;

		//***** Name Options
		else if(!strcasecmp(argv[iActArg], "/TAB") ||
				!strcasecmp(argv[iActArg], "-TAB")     )
		{
			if(iActArg+1 >= argc)
			{
				printf( "Error: /TAB parameter given "
				"without tabname\n");
				return -1;
			}
			strcpy(config.achSqlTabName, argv[iActArg+1]);
			iActArg++;
		}

		else if(!strcasecmp(argv[iActArg], "/F") ||
				!strcasecmp(argv[iActArg], "-F")     )
		{
			if(iActArg+1 >= argc)
			{
				printf( "Error: /F parameter given "
				"without Filename\n");
				return -1;
			}
			strcpy(config.achInpFileName, argv[iActArg+1]);
			iActArg++;
		}

		else if(!strcasecmp(argv[iActArg], "/CP") ||
				!strcasecmp(argv[iActArg], "-CP")     )
		{
			if(iActArg+1 >= argc)
			{
				printf( "Error: /CP parameter given "
				"without Filename\n");
				return -1;
			}
			strcpy(config.achCodePage, argv[iActArg+1]);
			iActArg++;
		}

		//***** Debug Options *****
		else if(!strcasecmp(argv[iActArg], "/D") ||
				!strcasecmp(argv[iActArg], "-D")     )
		{
			debugInfo.debug_level = 0xFFFFFFFF;
			debugInfo.dbgFile = stderr;
		}

		else if(!strcasecmp(argv[iActArg], "/D1") ||
				!strcasecmp(argv[iActArg], "-D1")     )
		{
			debugInfo.debug_level = 0xFFFFFFFF;
			debugInfo.dbgFile = stdout;
		}

		//***** Unknown Option *****
		else
		{
			fprintf(stderr, "Unknown parm: %s\n", argv[iActArg]);
			return -1;
		}
	}

	debug_printf(dbg_in, "Input File Name=%s\n", config.achInpFileName);
	debug_printf(dbg_in, "SQL Table Name=%s\n", config.achSqlTabName);

#ifdef CONF_MYSQL
	if(config.printFormat == execMysql || config.printFormat == checkMysql)
	{
		iRc = initMysql(config.achMysqlHost, config.achMysqlDatabase,
				config.achMysqlUser, config.achMysqlPassword);
		if(iRc != 0)
			return iRc;
	}
#endif

	switch(action)
	{
	case process:
		iRc = processFile(config.achInpFileName);
		break;
	case createTab:
		iRc = createTable();
		break;
#ifdef CONF_HBCIPX
	case procHbci:
		iRc = processHbci(config.achInpFileName);
		break;
#endif
	default:
		debug_printf(dbg_in, "internal error action=%d\n",
				action);
		iRc = -1;
	}

#ifdef CONF_MYSQL
	if(config.printFormat == execMysql || config.printFormat == checkMysql)
		termMysql();
#endif

	debug_printf(dbg_in, "Program ended. RC = %d\n", iRc);

	return iRc;

}

