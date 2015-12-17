#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "adm.h"
#include "adm_message.h"
#include "adm_newpass.h"

void adm_newpass(SysConfig *sys, DB *db) {
  char *msg = "";

  if(license_valid == FALSE) msg = "onClick=\"javascript:history.back();alert('You can not change master password under limited mode.');\"";
  headHtml(sys);

  printf("<form method=post action='%s'>\n", sys->AdmPrgName);
  printf("<input type='hidden' name='mode'     value='newpasssave'>\n");
  printf("<input type='hidden' name='password' value='%s'>\n", db->password);

  printf("<table border=1 cellspacing=0 cellpadding=3>\n");
  printf("  <tr><td bgcolor=#4c81ce align=center colspan=2><font size=3 face='arial'><b>Change Password</b></font></td></tr>\n");

  printf("  <tr>\n");
  printf("    <td bgcolor=#cdf1ff align=center><font size=2 face='arial'><b>New Password</b></font></td>\n");
  printf("    <td bgcolor=#f0f9ff align=center><input type='password' name='cPassword' size=20 maxlength=%d></td>\n", MAX_PASSWORD_LEN);
  printf("  </tr>\n");

  printf("  <tr>\n");
  printf("    <td bgcolor=#cdf1ff align=center><font size=2 face='arial'><b>Re-enter Password</b></font></td>\n");
  printf("    <td bgcolor=#f0f9ff align=center><input type='password' name='cPassword2' size=20 maxlength=%d></td>\n", MAX_PASSWORD_LEN);
  printf("  </tr>\n");

  printf("</table>\n");

  printf("<br>\n");

  printf("<font size=2><input type='submit' value='    APPLY NEW PASSWORD    ' %s></font>\n", msg);
  printf("</form>\n");

  printf("</table>\n");

  printf("<a href='javascript:history.back()' onMouseOver=\"window.status=('Back'); return true;\"><img src='%s/%s' border=0 alt='Back'></a>\n", sys->HtmlBasePath, sys->IconBack);

  tailHtml();
}

void adm_newpasssave(SysConfig *sys, DB *db) {
  char *password, *rePassword;
  char *encPassword;

  password   = qValue("cPassword");
  rePassword = qValue("cPassword2");

  if(password == NULL || rePassword == NULL) qError("Do not use manually.");

  if(strcmp(password, rePassword)) qError("Two passwords must be same.");

  if(qStr09AZaz(password) == FALSE) qError("Only regular expression(0-9, A-Z, a-z) is allowed for the password.");

  if(strlen(password) < 2) qError("Password is too short.");

  encPassword = crypt(password, sys->CryptSalt);

  saveMessage(PASSWORD_FILE, encPassword);

  db->password = encPassword;
}
