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
		"CREATE TABLE IF NOT EXISTS %s ("
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

	static const char achCreViewCat[] = "CREATE OR REPLACE VIEW %s_cat AS "
		"( SELECT "
			"ID, ORIG_KTONR, ORIG_BLZ, BUCHART, IFNULL(DATUM_KOR, VALUTA) AS DATUM, "
			"WAEHRUNG, "
			"BETRAG - IFNULL(KATG2_BETRAG, 0) - IFNULL(KATG3_BETRAG, 0) - "
			         "IFNULL(KATG4_BETRAG, 0) - IFNULL(KATG5_BETRAG, 0) AS BETRAG, "
			"BUCHUNGS_SL, REFERENZ, GV_CODE, PART_NAME1, "
			"PART_NAME2, PART_KTONR, PART_BLZ, BUTEXT, "
			"PRIMANOTA, VZWECK1, VZWECK2, VZWECK3, VZWECK4, "
			"VZWECK5, VZWECK6, VZWECK7, SOURCE, "
		// @TODO: use full KATG field includding slashed
			"substring_index(KATG, '/', 1) AS KATG, "
			"substring_index(KATG, '/', 1) AS KATG_L1, "
			"substring_index(KATG, '/', 2) AS KATG_L2, "
			"substring_index(KATG, '/', 3) AS KATG_L3 "
			"FROM %s ) "
		"UNION ( SELECT "
			"ID, ORIG_KTONR, ORIG_BLZ, BUCHART, IFNULL(DATUM_KOR, VALUTA) AS DATUM, "
			"WAEHRUNG, "
			"KATG2_BETRAG, "
			"BUCHUNGS_SL, REFERENZ, GV_CODE, PART_NAME1, "
			"PART_NAME2, PART_KTONR, PART_BLZ, BUTEXT, "
			"PRIMANOTA, VZWECK1, VZWECK2, VZWECK3, VZWECK4, "
			"VZWECK5, VZWECK6, VZWECK7, SOURCE, "
		// @TODO: use full KATG field includding slashed
			"substring_index(KATG2, '/', 1) AS KATG, "
			"substring_index(KATG2, '/', 1) AS KATG_L1, "
			"substring_index(KATG2, '/', 2) AS KATG_L2, "
			"substring_index(KATG2, '/', 3) AS KATG_L3 "
			"FROM %s "
			"WHERE KATG2_BETRAG <> 0 AND KATG2_BETRAG IS NOT NULL ) "
		"UNION ( SELECT "
	        "ID, ORIG_KTONR, ORIG_BLZ, BUCHART, IFNULL(DATUM_KOR, VALUTA) AS DATUM, "
			"WAEHRUNG, "
			"KATG3_BETRAG, "
			"BUCHUNGS_SL, REFERENZ, GV_CODE, PART_NAME1, "
			"PART_NAME2, PART_KTONR, PART_BLZ, BUTEXT, "
			"PRIMANOTA, VZWECK1, VZWECK2, VZWECK3, VZWECK4, "
			"VZWECK5, VZWECK6, VZWECK7, SOURCE, "
		// @TODO: use full KATG field includding slashed
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
		// @TODO: use full KATG field includding slashed
			"substring_index(KATG4, '/', 1) AS KATG, "
			"substring_index(katg4, '/', 1) as KATG_L1, "
			"substring_index(katg4, '/', 2) as KATG_L2, "
			"substring_index(katg4, '/', 3) as KATG_L3 "
			"FROM %s "
			"WHERE KATG4_BETRAG <> 0 AND KATG4_BETRAG IS NOT NULL ) "
		"UNION ( SELECT "
	        "ID, ORIG_KTONR, ORIG_BLZ, BUCHART, IFNULL(DATUM_KOR, VALUTA) AS DATUM, "
			"WAEHRUNG, "
			"KATG5_BETRAG, "
			"BUCHUNGS_SL, REFERENZ, GV_CODE, PART_NAME1, "
			"PART_NAME2, PART_KTONR, PART_BLZ, BUTEXT, "
			"PRIMANOTA, VZWECK1, VZWECK2, VZWECK3, VZWECK4, "
			"VZWECK5, VZWECK6, VZWECK7, SOURCE, "
		// @TODO: use full KATG field includding slashed
			"substring_index(KATG5, '/', 1) AS KATG, "
			"substring_index(katg5, '/', 1) as KATG_L1, "
			"substring_index(katg5, '/', 2) as KATG_L2, "
			"substring_index(katg5, '/', 3) as KATG_L3 "
			"FROM %s "
			"WHERE KATG5_BETRAG <> 0 AND KATG5_BETRAG IS NOT NULL ) ";
	static const char achCreViewManual[] = "CREATE OR REPLACE VIEW %s_manual AS "
		"SELECT * "
			"FROM %s "
			"WHERE ORIG_KTONR = 'Bar' ";
	static const char achCreViewBalance[] = "CREATE OR REPLACE VIEW %s_balance AS "
		"SELECT "
			"ORIG_KTONR, JAHR, MONAT, "
			"SALDO_DAY, SALDO, SALDO + "
			"(	SELECT ifnull(sum(s1.betrag),0) "
				"FROM fn_entry s1 "
				"WHERE "
					"s1.orig_ktonr=l0.orig_ktonr "
	       			"AND s1.datum_kor is not null "
					"AND s1.datum_kor <= l0.saldo_day "
					"AND s1.datum > l0.saldo_day "
				") - ("
				"SELECT ifnull(sum(s2.betrag),0) "
				"FROM fn_entry s2 "
				"WHERE "
					"s2.orig_ktonr=l0.orig_ktonr "
					"AND s2.datum_kor is not null "
					"AND s2.datum_kor > l0.saldo_day "
					"AND s2.datum <= l0.saldo_day "
				") as saldo_corrected "
		"FROM ( "
			"SELECT DISTINCT "
				"orig_ktonr, "
				"year(datum) as jahr, "
				"month(datum) as monat, "
				"last_value(datum) OVER ( "
					"partition by orig_ktonr, jahr, monat "
	       			"order by orig_ktonr, datum, buchart desc "
					"rows between unbounded preceding and unbounded following "
					") as saldo_day, "
				"last_value(saldo) OVER ( "
					"partition by orig_ktonr, jahr, monat "
	       			"order by orig_ktonr, datum, buchart desc "
					"rows between unbounded preceding and unbounded following "
					") as saldo "
			"FROM ( "
				"SELECT "
					"*, "
					"sum(betrag) OVER ( "
						"partition by orig_ktonr, saldo_partition "
		       			"order by orig_ktonr, datum, buchart desc "
						"rows between unbounded preceding and current row "
						") as saldo "
				"FROM ( "
					"SELECT "
						"*, "
						"sum(if(buchart='E',1,0)) OVER ( "
							"partition by orig_ktonr "
							"order by orig_ktonr, datum, buchart desc "
							") as saldo_partition "
					"FROM %s "
					"WHERE buchart='B' OR weekday(datum) = 5 "
					") as l2 "
				") as l1 "
			"ORDER BY orig_ktonr, datum, buchart desc "
			") as l0 "
		"ORDER BY orig_ktonr, jahr, monat";

	static const char achGrantTab[] =
			"GRANT SELECT, UPDATE ( "
				"DATUM_KOR, KATG, "
				"KATG2, KATG2_BETRAG, "
				"KATG3, KATG3_BETRAG, "
				"KATG4, KATG4_BETRAG, "
				"KATG5, KATG5_BETRAG "
			") ON %s TO fin_user";
	static const char achGrantViewCat[] =
			"GRANT SELECT on %s_cat TO fin_user";
	static const char achGrantViewManual[] =
			"GRANT SELECT,INSERT,UPDATE on %s_manual TO fin_user";
	static const char achGrantViewBalance[] =
			"GRANT SELECT on %s_balance TO fin_user";

	static char achTempCreTab[sizeof(achCreTab)+sizeof(config.achSqlTabName)] = "";
	static char achTempCreViewCat[sizeof(achCreViewCat)+6*sizeof(config.achSqlTabName)] = "";
	static char achTempCreViewManual[sizeof(achCreViewManual)+2*sizeof(config.achSqlTabName)] = "";
	static char achTempCreViewBalance[sizeof(achCreViewBalance)+2*sizeof(config.achSqlTabName)] = "";
	static char achTempGrantTab[sizeof(achGrantTab)+sizeof(config.achSqlTabName)] = "";
	static char achTempGrantViewCat[sizeof(achGrantViewCat)+sizeof(config.achSqlTabName)] = "";
	static char achTempGrantViewManual[sizeof(achGrantViewManual)+sizeof(config.achSqlTabName)] = "";
	static char achTempGrantViewBalance[sizeof(achGrantViewBalance)+sizeof(config.achSqlTabName)] = "";

	static char const * apchTempResult[] = {
		achTempCreTab, achTempCreViewCat, achTempCreViewManual, achTempCreViewBalance,
		achTempGrantTab, achTempGrantViewCat, achTempGrantViewManual, achTempGrantViewBalance,
		NULL};

	snprintf(achTempCreTab , sizeof(achTempCreTab ), achCreTab,
		config.achSqlTabName);
	snprintf(achTempCreViewCat, sizeof(achTempCreViewCat), achCreViewCat,
		config.achSqlTabName, config.achSqlTabName,
	    config.achSqlTabName, config.achSqlTabName,
	    config.achSqlTabName, config.achSqlTabName);
	snprintf(achTempCreViewManual , sizeof(achTempCreViewManual ), achCreViewManual,
	    config.achSqlTabName, config.achSqlTabName);
	snprintf(achTempCreViewBalance , sizeof(achTempCreViewBalance ), achCreViewBalance,
	    config.achSqlTabName, config.achSqlTabName);
	snprintf(achTempGrantTab , sizeof(achTempGrantTab ), achGrantTab,
		config.achSqlTabName);
	snprintf(achTempGrantViewCat, sizeof(achTempGrantViewCat), achGrantViewCat,
		config.achSqlTabName);
	snprintf(achTempGrantViewManual, sizeof(achTempGrantViewManual), achGrantViewManual,
		config.achSqlTabName);
	snprintf(achTempGrantViewBalance, sizeof(achTempGrantViewBalance), achGrantViewBalance,
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
