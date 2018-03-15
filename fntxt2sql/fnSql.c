/***** fnSql.c *****/

#include "fntxt2sql.h"

#include <string.h>
#include <math.h>

static const char* ALL_FIELDS =
	"ORIG_BLZ, ORIG_KTONR, DATUM, VALUTA, BETRAG, "
	"WAEHRUNG, BUCHART, BUCHUNGS_SL, GV_CODE, PART_BLZ, PART_KTONR, "
	"PART_NAME1, PART_NAME2, PRIMANOTA, REFERENZ, BUTEXT, "
	"VZWECK1, VZWECK2, VZWECK3, VZWECK4, VZWECK5, VZWECK6, VZWECK7, "
	"SOURCE ";

static void sqlPrintField(char *out, const char *field)
{
	if(field != NULL && *field != '\0')
	{
		strcat(out, "\"");
		strcat(out, field);
		strcat(out, "\"");
	}
	else
		strcat(out, "NULL");
}

static void sqlPrintDoubleField(char *out, const double value)
{
	double dblValue = value;
	double dblValue2, dblValue3;
	int iValue;
	char *pActOut = out + strlen(out);

	if(dblValue < 0.0)
	{
		strcat(pActOut, "-");
		pActOut++;
		dblValue = dblValue * -1.0;
	}
	dblValue2 = floor(dblValue);
	dblValue3 = dblValue - dblValue2 + 0.005; // Nur Nachkomma
	iValue = (int)(dblValue3 *100);

	sprintf(pActOut, "%.0f.%02d", dblValue2, iValue);

	debug_printf(dbg_fld, "Double field(%f) = %s", value, pActOut);
}


//****************************************************************************
//***** Create Table (SQL) erzeugen ******************************************
//****************************************************************************
const char * const * getCreateSql(void)
{
	static const char achCreTab[] =
		"CREATE TABLE %s ("
		"ID"          " INT"          " PRIMARY KEY AUTO_INCREMENT" ","
		"ORIG_KTONR"  " CHAR(10)"     " NOT NULL"                   ","
		"ORIG_BLZ"    " CHAR(8)"      " NOT NULL"                   ","
		"BUCHART"     " CHAR(1)"      " NOT NULL"                   ","
		"DATUM"       " DATE"         " NOT NULL"                   ","
		"VALUTA"      " DATE"                                       ","
		"WAEHRUNG"    " CHAR(3)"      " NOT NULL"                   ","
		"BETRAG"      " DECIMAL(11,2)"" NOT NULL"                   ","
		"BUCHUNGS_SL" " CHAR(3)"                                    ","
		"REFERENZ"    " VARCHAR(27)"                                ","
		"GV_CODE"     " VARCHAR(10)"                                ","
		"PART_NAME1"  " VARCHAR(27)"                                ","
		"PART_NAME2"  " VARCHAR(27)"                                ","
		"PART_KTONR"  " CHAR(34)"                                   ","
		"PART_BLZ"    " CHAR(11)"                                   ","
		"BUTEXT"      " VARCHAR(27)"                                ","
		"PRIMANOTA"   " CHAR(4)"                                    ","
		"VZWECK1"     " VARCHAR(140)"                               ","
		"VZWECK2"     " VARCHAR(140)"                               ","
		"VZWECK3"     " VARCHAR(140)"                               ","
		"VZWECK4"     " VARCHAR(140)"                               ","
		"VZWECK5"     " VARCHAR(140)"                               ","
		"VZWECK6"     " VARCHAR(140)"                               ","
		"VZWECK7"     " VARCHAR(140)"                               ","
		"SOURCE"      " VARCHAR(255)"                               ","
		"DATUM_KOR"   " DATE"                                       ","
		"KATG"        " VARCHAR(27)"  "DEFAULT NULL"                ","
		"KATG2"       " VARCHAR(27)"  "DEFAULT NULL"                ","
		"KATG2_BETRAG"" DECIMAL(11,2)""DEFAULT NULL"                ","
		"KATG3"       " VARCHAR(27)"  "DEFAULT NULL"                ","
		"KATG3_BETRAG"" DECIMAL(11,2)""DEFAULT NULL"                ","
		"KATG4"       " VARCHAR(27)"  "DEFAULT NULL"                ","
		"KATG4_BETRAG"" DECIMAL(11,2)""DEFAULT NULL"                ","
		"KATG5"       " VARCHAR(27)"  "DEFAULT NULL"                ","
		"KATG5_BETRAG"" DECIMAL(11,2)""DEFAULT NULL"
		/* Ab hier MySql spezifisch !!! */                          ","
		"INDEX (ORIG_KTONR, DATUM),"
		"INDEX (KATG),"
		"INDEX (KATG2),"
		"INDEX (KATG3),"
		"INDEX (KATG4),"
		"INDEX (KATG5)"
		") CHARSET=utf8";

	static const char achCreView[] = "CREATE VIEW %s_cat AS "
		"( SELECT "
	        "ID, ORIG_KTONR, ORIG_BLZ, BUCHART, IFNULL(DATUM_KOR, VALUTA) AS DATUM, "
			"WAEHRUNG, "
			"BETRAG - IFNULL(KATG2_BETRAG, 0) - IFNULL(KATG3_BETRAG, 0) - "
			         "IFNULL(KATG4_BETRAG, 0) - IFNULL(KATG5_BETRAG, 0) AS BETRAG, "
			"BUCHUNGS_SL, REFERENZ, GV_CODE, PART_NAME1, "
			"PART_NAME2, PART_KTONR, PART_BLZ, BUTEXT, "
			"PRIMANOTA, VZWECK1, VZWECK2, VZWECK3, VZWECK4, "
			"VZWECK5, VZWECK6, VZWECK7, SOURCE, "
			"substring_index(KATG, '/', 1) AS KATG, "
                        "substring_index(katg, '/', 1) as KATG_L1, "
                        "substring_index(katg, '/', 2) as KATG_L2, "
                        "substring_index(katg, '/', 3) as KATG_L3 "
			"FROM %s ) "
		"UNION ( SELECT "
	        "ID, ORIG_KTONR, ORIG_BLZ, BUCHART, IFNULL(DATUM_KOR, VALUTA) AS DATUM, "
			"WAEHRUNG, "
			"KATG2_BETRAG, "
			"BUCHUNGS_SL, REFERENZ, GV_CODE, PART_NAME1, "
			"PART_NAME2, PART_KTONR, PART_BLZ, BUTEXT, "
			"PRIMANOTA, VZWECK1, VZWECK2, VZWECK3, VZWECK4, "
			"VZWECK5, VZWECK6, VZWECK7, SOURCE, "
			"substring_index(KATG2, '/', 1) AS KATG, "
                        "substring_index(katg2, '/', 1) as KATG_L1, "
                        "substring_index(katg2, '/', 2) as KATG_L2, "
                        "substring_index(katg2, '/', 3) as KATG_L3 "
			"FROM %s "
			"WHERE KATG2_BETRAG <> 0 AND KATG2_BETRAG IS NOT NULL) "
		"UNION ( SELECT "
	        "ID, ORIG_KTONR, ORIG_BLZ, BUCHART, IFNULL(DATUM_KOR, VALUTA) AS DATUM, "
			"WAEHRUNG, "
			"KATG3_BETRAG, "
			"BUCHUNGS_SL, REFERENZ, GV_CODE, PART_NAME1, "
			"PART_NAME2, PART_KTONR, PART_BLZ, BUTEXT, "
			"PRIMANOTA, VZWECK1, VZWECK2, VZWECK3, VZWECK4, "
			"VZWECK5, VZWECK6, VZWECK7, SOURCE, "
			"substring_index(KATG3, '/', 1) AS KATG, "
                        "substring_index(katg3, '/', 1) as KATG_L1, "
                        "substring_index(katg3, '/', 2) as KATG_L2, "
                        "substring_index(katg3, '/', 3) as KATG_L3 "
			"FROM %s "
			"WHERE KATG3_BETRAG <> 0 AND KATG3_BETRAG IS NOT NULL ) "
		"UNION ( SELECT "
	        "ID, ORIG_KTONR, ORIG_BLZ, BUCHART, IFNULL(DATUM_KOR, VALUTA) AS DATUM, "
			"WAEHRUNG, "
			"KATG4_BETRAG, "
			"BUCHUNGS_SL, REFERENZ, GV_CODE, PART_NAME1, "
			"PART_NAME2, PART_KTONR, PART_BLZ, BUTEXT, "
			"PRIMANOTA, VZWECK1, VZWECK2, VZWECK3, VZWECK4, "
			"VZWECK5, VZWECK6, VZWECK7, SOURCE, "
			"substring_index(KATG4, '/', 1) AS KATG, "
                        "substring_index(katg4, '/', 1) as KATG_L1, "
                        "substring_index(katg4, '/', 2) as KATG_L2, "
                        "substring_index(katg4, '/', 3) as KATG_L3 "
			"FROM %s "
			"WHERE KATG4_BETRAG <> 0 AND KATG4_BETRAG IS NOT NULL) "
		"UNION ( SELECT "
	        "ID, ORIG_KTONR, ORIG_BLZ, BUCHART, IFNULL(DATUM_KOR, VALUTA) AS DATUM, "
			"WAEHRUNG, "
			"KATG5_BETRAG, "
			"BUCHUNGS_SL, REFERENZ, GV_CODE, PART_NAME1, "
			"PART_NAME2, PART_KTONR, PART_BLZ, BUTEXT, "
			"PRIMANOTA, VZWECK1, VZWECK2, VZWECK3, VZWECK4, "
			"VZWECK5, VZWECK6, VZWECK7, SOURCE, "
			"substring_index(KATG5, '/', 1) AS KATG, "
                        "substring_index(katg5, '/', 1) as KATG_L1, "
                        "substring_index(katg5, '/', 2) as KATG_L2, "
                        "substring_index(katg5, '/', 3) as KATG_L3 "
			"FROM %s "
			"WHERE KATG5_BETRAG <> 0 AND KATG5_BETRAG IS NOT NULL) ";
	static const char achGrantBatch[] =
			"GRANT SELECT, INSERT on %s TO finance";
	static const char achGrantUser[] =
			"GRANT SELECT, UPDATE (katg2, katg, katg2_betrag, datum_kor) ON %s TO stefan";
	static const char achGrantView[] =
			"GRANT SELECT on %s_cat TO finance,stefan";
	static char achTempCreTab[sizeof(achCreTab)+100] = "";
	static char achTempCreView[sizeof(achCreView)+100] = "";
	static char achTempGrantBatch[sizeof(achGrantBatch)+100] = "";
	static char achTempGrantUser[sizeof(achGrantUser)+100] = "";
	static char achTempGrantView[sizeof(achGrantView)+100] = "";

// @TODO: Neue Tabelle mit Grant erzeugen
//  create view fn_manual_entry as select * from fn_entry where ORIG_KTONR="Bar";
// grant insert, update on fn_manual_entry to stefan

	static char const * apchTempResult[] = {
		achTempCreTab, achTempCreView,
		achTempGrantBatch, achTempGrantUser, achTempGrantView, NULL};

	snprintf(achTempCreTab , sizeof(achTempCreTab ), achCreTab,
		config.achSqlTabName);
	snprintf(achTempCreView, sizeof(achTempCreView), achCreView,
		config.achSqlTabName, config.achSqlTabName,
	    config.achSqlTabName, config.achSqlTabName,
	    config.achSqlTabName, config.achSqlTabName);
	snprintf(achTempGrantBatch, sizeof(achTempGrantBatch), achGrantBatch,
		config.achSqlTabName);
	snprintf(achTempGrantUser , sizeof(achTempGrantUser ), achGrantUser,
		config.achSqlTabName);
	snprintf(achTempGrantView , sizeof(achTempGrantView ), achGrantView,
		config.achSqlTabName);

	return apchTempResult;
}


//****************************************************************************
//***** Insert Statemen (SQL) erzeugen ***************************************
//****************************************************************************
const char * getInsertSql(const Buchung buchung)
{
	static char achSql[4096];
	int i;

	sprintf(achSql,
			"INSERT INTO %s (%s) VALUES (", config.achSqlTabName, ALL_FIELDS);

	sqlPrintField(achSql, buchung.orig_blz);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.orig_ktonr);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.datum);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.valuta);
	strcat(achSql, ", "); sqlPrintDoubleField(achSql, buchung.betrag);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.waehrung);
	sprintf(&achSql[strlen(achSql)], ", \"%c\"", buchung.buchart);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.buchungs_sl);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.gv_code);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.part_blz);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.part_ktonr);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.part_name1);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.part_name2);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.primanota);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.referenz);
	strcat(achSql, ", "); sqlPrintField(achSql, buchung.butext);

	for(i = 0; i < sizeof(buchung.vzweck) / sizeof(buchung.vzweck[0]); i++)
	{ strcat(achSql, ", "); sqlPrintField(achSql, buchung.vzweck[i]); }

	strcat(achSql, ", "); sqlPrintField(achSql, buchung.source); strcat(achSql, ")");

	return achSql;
}

//****************************************************************************
//***** Select Statement (SQL) erzeugen **************************************
//****************************************************************************
const char * getSelectSql(const Buchung buchung)
{
	static char achSql[4096] = "";

	// Look if similar record exists...
	sprintf(achSql,
			"SELECT %s"
			"FROM %s "
			"WHERE "
			"ORIG_KTONR='%s' AND "
			"DATUM in ('%s', '%s') AND "
			"BUCHART='%c' AND "
			"BETRAG=", ALL_FIELDS, config.achSqlTabName,
			buchung.orig_ktonr,
			buchung.datum, buchung.valuta,
			buchung.buchart);
	sqlPrintDoubleField(achSql+strlen(achSql), buchung.betrag);

	return achSql;
}

//****************************************************************************
