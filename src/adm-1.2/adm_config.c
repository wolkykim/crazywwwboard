#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "adm.h"
#include "adm_message.h"
#include "adm_config.h"

void adm_config(SysConfig *sys, DB *db) {
  char *config;
  char *msg = "";

  config = readMessage(REALTIME_CONFIG_FILE);

/*
  if(license_valid == FALSE) msg = "onClick=\"alert('You can not store configurations under limited mode.');window.close();\"";
*/

  headHtml(sys);

  printf("<form method=post action='%s'>\n", sys->AdmPrgName);
  printf("<input type='hidden' name='mode'     value='configsave'>\n");
  printf("<input type='hidden' name='password' value='%s'>\n", db->password);

  printf("<table border=1 cellspacing=0 width=%s>\n", LIST_WIDTH);
  printf("  <tr><td bgcolor=#4c81ce align=center><font size=2 face='arial'><b>Global Configuration</b></font></td></tr>\n");
  printf("  <tr><td bgcolor=#f0f9ff align=center><textarea name='cConfig' cols=72 rows=20>"); qPuts(1, config); printf("</textarea></td></tr>\n");
  printf("</table>\n");

  printf("<br>\n");

  printf("<font size=2><input type='submit' value='              SAVE CONFIGURATION              ' %s></font>\n", msg);
  printf("</form>\n");

  tailHtml();
}

void adm_configsave(SysConfig *sys, DB *db) {
  char *config;

  config = qValue("cConfig");

  if(config == NULL) qError("Do not use manually.");

  saveMessage(REALTIME_CONFIG_FILE, config);
}
