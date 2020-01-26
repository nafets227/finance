/***** fntxt2sql-aqb6.c *****/

#include "fntxt2sql.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
//***** AqBanking6 Datei behandeln *******************************************
//****************************************************************************
int processAqb6File(char *pchFileName)
{
	fprintf(stderr, "Aqb6 not implemented yet\n");
	return 1; // not implemented yet
}
