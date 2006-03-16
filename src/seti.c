/*
 * seti.c: information about SETI for Conky
 *
 *  $Id$
 */

#include "conky.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *seti_dir = NULL;

/*
 * Need to code for BOINC, because the old SETI@Home does not use xml to store data.
 * Perhaps in the .conkyrc file there could be an option for BOINC or old SETI.
 */

/*static float seti_get_float (FILE *fp, const char *name)
{
  char buffer[80];
  char *token;

  while (!feof(fp) && !ferror (fp))
  {
    fgets(buffer, 80, fp);
    token = strtok(buffer, ">");

    if (strcmp(token, name) == 0)
    {
      token = strtok(NULL, "<");
      if ( token != NULL )
        return atof (token);
      break;
    }
  }
  return 0.0f;
}*/

float seti_get_data(FILE * fp, const char *name)
{
	char token[1000];
	char buffer[1000];
	float data;

	while (fgets(token, 1000, fp) != NULL)	//read the file
		if (strncmp(token, name, strlen(name)) == 0) {	//and search for the data in name
			strcpy(buffer, strchr(token, '=') + 1);	//copy just the number
			break;
		}
	data = atof(buffer);
	return data;
}

void update_seti()
{
	if (seti_dir == NULL)
		return;

	char filename[80];

	struct information *current_info = &info;

	current_info->seti_prog = current_info->seti_credit = 0.0f;

	/* read total user credit */

	/*FILE *fp = fopen(filename, "r");
	   if (!fp)
	   return;

	   seti_credit = seti_get_float(fp, "<user_total_credit");

	   fclose (fp); */

	snprintf(filename, 80, "%s/user_info.sah", seti_dir);

	FILE *fp = fopen(filename, "r");

	if (!fp) {
		return;
	}

	current_info->seti_credit = seti_get_data(fp, "nresults");

	fclose(fp);

	/* read current progress */

	/*snprintf(filename, 80, "%s/slots/0/state.sah", seti_dir);
	   fp = fopen(filename, "r");
	   if (!fp)
	   return;

	   seti_prog = seti_get_float(fp, "<prog");

	   fclose (fp);

	   snprintf(filename, 80, "%s/slots/0/init_data.xml", seti_dir); */

	snprintf(filename, 80, "%s/state.sah", seti_dir);

	fp = fopen(filename, "r");
	if (!fp)
		return;

	current_info->seti_prog = seti_get_data(fp, "prog");

	fclose(fp);

}
