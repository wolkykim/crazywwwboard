#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "adm.h"
#include "adm_message.h"

void adm_message(SysConfig *sys, DB *db) {
  char p_htmlhead[0xff], *htmlhead;
  char p_htmltail[0xff], *htmltail;
  char p_mailhead[0xff], *mailhead;
  char p_mailtail[0xff], *mailtail;
  char *msg = "";

  sprintf(p_htmlhead, "%s/%s", sys->RealBasePath, sys->HtmlHead);
  sprintf(p_htmltail, "%s/%s", sys->RealBasePath, sys->HtmlTail);
  sprintf(p_mailhead, "%s/%s", sys->RealBasePath, sys->MailHead);
  sprintf(p_mailtail, "%s/%s", sys->RealBasePath, sys->MailTail);

  htmlhead = readMessage(p_htmlhead);
  htmltail = readMessage(p_htmltail);
  mailhead = readMessage(p_mailhead);
  mailtail = readMessage(p_mailtail);

/*
  if(license_valid == FALSE) msg = "onClick=\"alert('You can not store messages under limited mode.');window.close();\"";
*/

  headHtml(sys);

  printf("<form method=post action='%s'>\n", sys->AdmPrgName);
  printf("<input type='hidden' name='mode'     value='messagesave'>\n");
  printf("<input type='hidden' name='password' value='%s'>\n", db->password);

  printf("<table border=1 cellspacing=0 width=%s>\n", LIST_WIDTH);
  printf("  <tr><td bgcolor=#4c81ce align=center colspan=2><font size=2 face='arial'><b>System Default HTML Messages</b></font></td></tr>\n");
  printf("  <tr><td bgcolor=#cdf1ff align=center width=120><font size=2 face='arial'><b>CWB Output</b></font></td>\n");
  printf("      <td bgcolor=#f0f9ff><font size=2 face='arial'>\n");
  printf("      &lt;html&gt;<br>\n");
  printf("      &lt;head&gt;<br>\n");
  printf("      &lt;title&gt;BROWSER TITLE&lt;/title&gt;<br>\n");
  printf("      &lt;meta http-equiv=\"Content-Type\" content=\"text/html; charset=CHARSET\"&gt;<br>\n");
  printf("      &lt;/head&gt;\n");
  printf("      </font></td></tr>\n");

  printf("  <tr><td bgcolor=#cdf1ff align=center width=120><font size=2 face='arial'><b>HTML Header</b></font></td>\n");
  printf("      <td bgcolor=#f0f9ff><textarea name='cHtmlHead' cols=60 rows=6>"); qPuts(1, htmlhead); printf("</textarea></td></tr>\n");

  printf("  <tr><td bgcolor=#cdf1ff align=center><font size=2 face='arial'><b>CWB Output</b></font></td>\n");
  printf("      <td bgcolor=#f0f9ff align=center><font size=2 face='arial'>\n");
  printf("      Main Contents HERE\n");
  printf("      </font></td></tr>\n");

  printf("  <tr><td bgcolor=#cdf1ff align=center><font size=2 face='arial'><b>HTML Tailer</b></font></td>\n");
  printf("      <td bgcolor=#f0f9ff><textarea name='cHtmlTail' cols=60 rows=6>"); qPuts(1, htmltail); printf("</textarea></td></tr>\n");

  printf("  <tr><td bgcolor=#cdf1ff align=center><font size=2 face='arial'><b>CWB Output</b></font></td>\n");
  printf("      <td bgcolor=#f0f9ff><font size=2 face='arial'>\n");
  printf("      &lt;/html&gt;\n");
  printf("      </font></td></tr>\n");

  printf("</table>\n");

  printf("<br>\n");

  printf("<table border=1 cellspacing=0 width=%s>\n", LIST_WIDTH);
  printf("  <tr><td bgcolor=#4c81ce align=center colspan=2><font size=2 face='arial'><b>System Default Email Messages</b></font></td></tr>\n");
  printf("  <tr><td bgcolor=#cdf1ff align=center width=120><font size=2 face='arial'><b>Email Header</b></font></td>\n");
  printf("      <td bgcolor=#f0f9ff><textarea name='cMailHead' cols=60 rows=4>"); qPuts(1, mailhead); printf("</textarea></td></tr>\n");

  printf("  <tr><td bgcolor=#cdf1ff align=center width=120><font size=2 face='arial'><b>CWB Output</b></font></td>\n");
  printf("      <td bgcolor=#f0f9ff align=center><font size=2 face='arial'>\n");
  printf("      Contents of article HERE\n");
  printf("      </td></tr>\n");

  printf("  <tr><td bgcolor=#cdf1ff align=center><font size=2 face='arial'><b>Email Tailer</b></font></td>\n");
  printf("      <td bgcolor=#f0f9ff><textarea name='cMailTail' cols=60 rows=4>"); qPuts(1, mailtail); printf("</textarea></td></tr>\n");
  printf("</table>\n");
  printf("<br>\n");
  printf("<font size=2><input type='submit' value='                SAVE MESSAGES                ' %s></font>\n", msg);
  printf("</form>\n");

  tailHtml();
}

void adm_messagesave(SysConfig *sys, DB *db) {
  char p_htmlhead[0xff], *htmlhead;
  char p_htmltail[0xff], *htmltail;
  char p_mailhead[0xff], *mailhead;
  char p_mailtail[0xff], *mailtail;

  sprintf(p_htmlhead, "%s/%s", sys->RealBasePath, sys->HtmlHead);
  sprintf(p_htmltail, "%s/%s", sys->RealBasePath, sys->HtmlTail);
  sprintf(p_mailhead, "%s/%s", sys->RealBasePath, sys->MailHead);
  sprintf(p_mailtail, "%s/%s", sys->RealBasePath, sys->MailTail);

  htmlhead = qValue("cHtmlHead");
  htmltail = qValue("cHtmlTail");
  mailhead = qValue("cMailHead");
  mailtail = qValue("cMailTail");

  if(htmlhead == NULL || htmltail == NULL || mailhead == NULL || mailtail == NULL) {
    qError("Do not use manually.");
  }

  saveMessage(p_htmlhead, htmlhead);
  saveMessage(p_htmltail, htmltail);
  saveMessage(p_mailhead, mailhead);
  saveMessage(p_mailtail, mailtail);
  
}

/* malloc하여 주어진 파일을 읽는다 */
char *readMessage(char *path) {
  struct stat buf;
  FILE *fp;
  char *tmp;
  int  i;

  if(stat(path, &buf) != 0) return NULL;

  tmp = (char *)malloc(sizeof(char) * ((int)buf.st_size + 1));

  fp = fopen(path, "rt");
  for(i=0; i < (int)buf.st_size ; i++) {
    tmp[i] = (char)fgetc(fp);
  }
  tmp[i] = '\0';
  fclose(fp);

  return tmp;
}

/* path에 string을 UNIX 형태로 저장한다. 
   stringCRLFstringCRLFstringCRLF -> stringLFstringLFstringLF */
#define CR	(13)
void saveMessage(char *path, char *string) {
  FILE *fp;

  if((fp = fopen(path, "wt")) == NULL) qError("File(%s) save fail.", path);

  /* unix 형태로 변환하며 저장 */
  for(; *string != '\0'; string++) {
    if(*string != CR) fputc((int)*string, fp);
  }
  fclose(fp);
}
