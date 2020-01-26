/***** fntxt2sql-aqb6.c *****/

#include "fntxt2sql.h"

#ifdef CONF_AQB6
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aqbanking6/aqbanking/banking.h>

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
	AB_BANKING *ab;


	ab=AB_Banking_new("aqbanking-cli", NULL, 0);
	if (ab == NULL)
	{
		fprintf(stderr, "Could not init AqBanking6\n");
		return 1;
	}

	fprintf(stderr, "Aqb6 not implemented yet\n");

	return 1; // not implemented yet
}
#endif // CONF_AQB6
