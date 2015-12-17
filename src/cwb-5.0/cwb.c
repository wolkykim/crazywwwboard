/*************************************************************
** CrazyWWWBoard.cgi 98 Professional Edition                **
**                                                          **
**  Official distribution site : http://cwb98.nobreak.com   **
**           Technical contact : nobreak@nobreak.com        **
**                                                          **
**                        Developed by 'Seung-young Kim'    **
**                        Last updated at July, 7, 1998     **
**                                                          **
**      Copyright (c) 1996-1998 Nobreak Technologies, Inc.. **
*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "md5_crypt.h"
#include "cwb.h"
#include "cwb_admin.h"
#include "cwb_stat.h"
#include "cwb_write.h"
#include "cwb_read.h"
#include "cwb_list.h"
#include "cwb_delete.h"
#include "cwb_download.h"
#include "cwb_sync.h"

Entry *getRealtimeConfig(SysConfig *sys, char *filename);
void  getConfig(SysConfig *sys, DB *db, DBConfig *conf);
void  firstMeeting(char *prgName, char *dbName);
void  setCookie(char *name, char *value, int expdays);
int   accessLog(char *filename);
void  updateStat(SysConfig *sys, DB *db);
unsigned int getPermission(char *filename);
int   license_check(char *LicenseKey, char *SecureKey);
void  relocateList(char *cginame, DB *db);
void  programInfo(void);
void  editedInfo(void);

int license_valid = FALSE;

int main(int argc, char **argv){
  DB db;
  SysConfig sys;
  Entry *sysp;
  DBConfig conf;
  char sAccessLog[0xff], sErrorLog[0xff];

  /* Timer ���� */
  clock();

  /* Adjust umask */
  umask(S_IWGRP | S_IWOTH);

  /* get realtime config */
  sysp = getRealtimeConfig(&sys, REALTIME_CONFIG_FILE);

  /* Ensure CGI's permission 6755 */
  /*
  if(getPermission(sys.ProgramName) != 06755) qError("Wrong permission. %s must has setuid and setgid bit. You would better adjust it's permission by 'chmod 6755 %s' operation.", sys.ProgramName, sys.ProgramName);
  */

  /* default error log on : ���� ��ġ���� �߻��ϴ� ������ ����ϱ� ����*/
  if(strcmp(sys.LogDirectory, "")) {
    sprintf(sErrorLog,  "%s/%s/error_log",  sys.RealBasePath, sys.LogDirectory);
    qErrorLog(sErrorLog);
  }

  /* License Key �˻� */
#ifdef SECURE_LICENSE_KEY
  license_valid = license_check(sys.LicenseKey, SECURE_LICENSE_KEY);
#else
  license_valid = TRUE;
#endif

  /* ���� ��ġ */
  qDecoder();

  /* get DB config */
  getConfig(&sys, &db, &conf);

  /* Set Cookie */
  if(!strcmp(sys.CookieEnable, "true")) {
    char *tmp;
    if(!strcmp(db.mode, "writesave")) { /* ����, Email�� writesave �ÿ��� ��ŷ �Ѵ�. */
      if((tmp = qValue("pName"))    != NULL) setCookie("NAME", tmp, atoi(sys.CookieExpireDays));
      if((tmp = qValue("pEmail"))   != NULL) setCookie("EMAIL", tmp, atoi(sys.CookieExpireDays));
    }

    if((tmp = qValue("password")) != NULL) {
      if((strlen(tmp) > 0) && (strlen(tmp) <= MAX_PASSWORD_LEN)) {
        setCookie("PASSWORD", crypt(tmp, sys.CryptSalt), 0);
      }
      else if(!strcmp(tmp, "")) { /* expire password */
        setCookie("PASSWORD", "", -1);
      }
      else setCookie("PASSWORD", tmp, 0); /* encrypted password */
    }
  }

  /* db log on : default log�� �ѳ��� ���¿��� getConfig()�� ���� 
  ���� �α׸� ���ϴ� ������, �������� �ʴ� db�������� ������ ���� 
  �α� ������ �������� �ʱ� �����̴�. getConfig()���� �������� �ʴ� 
  db���� ������ qError()�� ���ܵǾ� default log�� ���� ������. */
  if(strcmp(sys.LogDirectory, "")) {
    sprintf(sAccessLog, "%s/%s/access_log.%s", sys.RealBasePath, sys.LogDirectory, qValue("db"));
    sprintf(sErrorLog,  "%s/%s/error_log.%s",  sys.RealBasePath, sys.LogDirectory, qValue("db"));
    qErrorLog(sErrorLog);
    qErrorContact(conf.AdminEmail);
    if(accessLog(sAccessLog) == FALSE) qError("Access logging(%s) fail.", sAccessLog);
  }

  /* ���� ��� */
  if(strcmp(sys.StatDirectory, "")) {
    updateStat(&sys, &db);
  }

  /* ��� �б� */
  if(!strcmp(db.mode, "list"))            cwb_list(sysp, &sys, &db, &conf);
  else if(!strcmp(db.mode, "read"))       cwb_read(sysp, &sys, &db, &conf);
  else if(!strcmp(db.mode, "write"))      cwb_write(&sys, &db, &conf);
  else if(!strcmp(db.mode, "reply"))      cwb_write(&sys, &db, &conf);
  else if(!strcmp(db.mode, "writesave"))  cwb_writesave(&sys, &db, &conf), relocateList(sys.ProgramName, &db);
  else if(!strcmp(db.mode, "modify"))     cwb_modify(&sys, &db, &conf);
  else if(!strcmp(db.mode, "modifysave")) cwb_modifysave(&sys, &db, &conf), relocateList(sys.ProgramName, &db);
  else if(!strcmp(db.mode, "delete"))     cwb_delete(&sys, &db, &conf), relocateList(sys.ProgramName, &db);
  else if(!strcmp(db.mode, "batchdelete"))cwb_batchdelete(&sys, &db, &conf);
  else if(!strcmp(db.mode, "download"))   cwb_download(&sys, &db, &conf);
  else if(!strcmp(db.mode, "admin"))      cwb_admin(&sys, &db, &conf);
  else if(!strcmp(db.mode, "adminsave"))  cwb_adminsave(&sys, &db, &conf), relocateList(sys.ProgramName, &db);
  else if(!strcmp(db.mode, "stat"))       cwb_stat(&sys, &db, &conf);
  else if(!strcmp(db.mode, "sync"))       cwb_sync(&sys, &db, &conf);
  else qError("'Unknown mode '%s'", db.mode);

  /* �޸� ���� */
  qFree();
  qcFree();
  qfFree(sysp);

  return (0);
}

Entry *getRealtimeConfig(SysConfig *sys, char *filename) {
  Entry *sysp;

  sysp = qfDecoder(filename);
  if(sysp == NULL) qError("Realtime configuration file(%s) not found.", filename);

#define FetchConf(NAME) { if((sys->NAME = qfValue(sysp, #NAME)) == NULL) qError("There is no entry named '%s' in realtime configuration file.", #NAME); }

  FetchConf(LicenseKey);

  if((sys->ProgramName = qCGIname()) == NULL) sys->ProgramName = OFFICIAL_CGI_NAME;

  FetchConf(AdminName);
  FetchConf(AdminEmail);

  FetchConf(RealBasePath);
  FetchConf(HtmlBasePath);

  FetchConf(DefaultTitle);
  FetchConf(DefaultCharSet);

  FetchConf(UploadSoftLimit);
  FetchConf(UploadHardLimit);

  FetchConf(DataDirectory);
  FetchConf(DataExtension);

  FetchConf(StatDirectory);
  FetchConf(StatExtension);

  FetchConf(LogDirectory);

  FetchConf(HtmlHead);
  FetchConf(HtmlTail);

  FetchConf(Help);

  FetchConf(IconAdminTitle);
  FetchConf(IconAdmin);
  FetchConf(IconStat);
  FetchConf(IconHome);
  FetchConf(IconBack);
  FetchConf(IconHelp);
  FetchConf(IconList);
  FetchConf(IconReload);
  FetchConf(IconWrite);
  FetchConf(IconModify);
  FetchConf(IconReply);
  FetchConf(IconDelete);
  FetchConf(IconUp);
  FetchConf(IconDown);
  FetchConf(IconBlank);

  FetchConf(IconWidth);
  FetchConf(IconHeight);
  FetchConf(IconOpen);
  FetchConf(IconClose);
  FetchConf(IconOpenNew);
  FetchConf(IconCloseNew);
  FetchConf(IconTypeDefault);
  FetchConf(IconTypeUnknown);

  FetchConf(StatBarWidth);
  FetchConf(StatBarMaxHeight);

  FetchConf(StatBarUnder);
  FetchConf(StatBar90);
  FetchConf(StatBar80);
  FetchConf(StatBar70);
  FetchConf(StatBar60);
  FetchConf(StatBar50);
  FetchConf(StatBar40);
  FetchConf(StatBar30);
  FetchConf(StatBar20);
  FetchConf(StatBar10);
  FetchConf(StatBar00);

  FetchConf(CryptSalt);

  FetchConf(Sendmail);
  FetchConf(SendmailOptions);

  FetchConf(MailHead);
  FetchConf(MailTail);

  FetchConf(MailName);
  FetchConf(MailOrganization);

  FetchConf(CookieEnable);
  FetchConf(CookieExpireDays);

  FetchConf(PageLinkNum);

  FetchConf(TextFieldLength);

  FetchConf(ReSubject);
  FetchConf(ReText);
  FetchConf(ReLine);

  FetchConf(PreviewLines);

  FetchConf(HiddenAdminLink);
  FetchConf(HiddenStatLink);
  FetchConf(HiddenHelpLink);

  FetchConf(AllowDBhtml);
  FetchConf(AllowDBmail);
  FetchConf(AllowUpload);

#undef FetchConf

  return sysp;
}

void getConfig(SysConfig *sys, DB *db, DBConfig *conf){
  char buf[0xFF];
  char *server_name, *tmp;
  int error = FALSE;

  /*************************************************************
  ** Query Fetch
  *************************************************************/
  /* db */
  if((db->db = qValue("db")) == NULL) qError("You must specify the database name.");
  if(!strcmp(db->db , "")) qError("You must specify the database name.");
  if(qStr09AZaz(db->db) == 0) qError("The operation is canceled. Only regular expression is allowed for the database name(%s).", db->db);
  /* mode */
  if((db->mode = qValue("mode")) == NULL) db->mode = "list";
  /* page */
  if((db->page = qiValue("page")) == 0) db->page = 1;
  /* num */
  db->num = qiValue("num");

  /* category */
  if((db->category = qValue("category")) == NULL) db->category = "";

  /* ftype */
  db->ftype = qiValue("ftype");

  /* ftype�� �����ϴ� ���� list ��⿡���� �Ѵ�, �������� ftype�� ������ �����Ѵ� */
  if(qValue("fname")    != NULL) db->ftype |= (int)FNAME;
  if(qValue("fsubject") != NULL) db->ftype |= (int)FSUBJECT;
  if(qValue("ftext")    != NULL) db->ftype |= (int)FTEXT;

  /* fval */
  if((db->fval = qValue("fval")) == NULL) db->fval = "";
  else qRemoveSpace(db->fval);

  /* fencval */
  db->fencval = qURLencode(db->fval);

  /* default search */
  if(db->ftype == 0 || !strcmp(db->fval, "")) {
    db->ftype = (int)(FSUBJECT | FTEXT);
    db->fval  = "";
  }

  /* password */
  if((db->password = qValue("password")) == NULL) db->password = qcValue("PASSWORD");
  else if (!strcmp(db->password, "")) db->password = NULL;
  else if((strlen(db->password) > 0) && (strlen(db->password) <= MAX_PASSWORD_LEN)) {
    tmp = crypt(db->password, sys->CryptSalt);
    db->password = strdup(tmp);
  }

  /* backdepth */
  db->backdepth = qiValue("backdepth");

  /*************************************************************
  ** DB���� Configuration�� ��´�.
  *************************************************************/
  if ((tmp = getenv("SERVER_NAME"))  == NULL) server_name  = "http://(YOUR HOMEPAGE URL)";
  else {
    server_name = (char *)malloc(strlen("http://") + strlen(tmp) + 1);
    sprintf(server_name, "%s%s", "http://", tmp);
  }

  sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
  DBOpen(buf, GDBM_READER);

#define FetchNull(NAME, VALUE) { if((conf->NAME = DBFetch("conf." #NAME)) == NULL) conf->NAME = VALUE; }
#define FetchZero(NAME, VALUE) { if((conf->NAME = DBiFetch("conf." #NAME)) == 0) conf->NAME = VALUE; }
#define FetchNullMalloc(NAME, VALUE) { if((conf->NAME = DBFetch("conf." #NAME)) == NULL) conf->NAME = strdup(VALUE); }

  if((conf->Password = DBFetch("conf.Password")) == NULL) error = TRUE;
  FetchNull(Title,   "(BROWSER TITLE HERE)");
  FetchNull(Message, "(BOARD TITLE HERE)");

  FetchNull(HomeUrl,    server_name);
  FetchNull(HomeTarget, "_top");
  FetchNull(BackUrl,    "");
  FetchNull(BackTarget, "");

  FetchNull(AdminName, sys->AdminName);
  FetchNull(AdminEmail, sys->AdminEmail);
  FetchNull(MailToAdmin, "true");
  FetchNull(MailToSender, "true");

  /* common */
  FetchNull(Language, "english");
  FetchNull(FontSize, "2");
  FetchNull(UnderlinedLink, "false");
  FetchNull(TableWidth, "90%");
  FetchNull(CharSet, "");
  FetchNull(UseSysDefHtml, "true");
  FetchNull(HtmlHead, "<html>\n<head>\n  <title>YOUR BROWSER TITLE HERE</title>\n</head>\n\n<body bgcolor=\"white\">\n<center>\n<font size=\"5\">BOARD MESSAGES HERE</font><br>\n");
  FetchNull(HtmlTail, "</center>\n</body>\n</html>\n");

  /* list */
  FetchNull(ListOrder, "thread");
  FetchNull(ArticlesPerPage, "15");
  FetchNull(FieldNumber, "virtual");
  FetchNull(FieldFiletype, "show");
  FetchNull(FieldFilename, "hidden");
  FetchNull(FieldName, "after");
  FetchNull(FieldSubject, "wrap");
  FetchNull(FieldDate, "show");
  FetchNull(FieldHits, "show");
  FetchNull(FieldDownload, "hidden");
  FetchNull(FolderIcon, "true");
  FetchNull(HighlightArticle, "0"); /* today */
  FetchNull(HideSearch, "false");
  FetchNull(ListIconLocation, "bottom");
  FetchNull(ListBorderSize, "0");
  FetchNull(ListTitleFontColor, "white");
  FetchNull(ListTitleBgColor, "#3366cc");
  FetchNull(ListArticleFontColor, "black");
  FetchNull(ListArticleBgColor, "#dddddd");

  /* read */
  FetchNull(HtmlAccept, "false");
  FetchNull(LinkTarget, "_top");
  FetchNull(DisplayAddress, "true");
  FetchNull(ReadIconLocation, "auto");
  FetchNull(ReadLineHeight, "12");
  FetchNull(ReadTitleBorderSize, "0");
  FetchNull(ReadTextBorderSize, "0");
  FetchNull(ReadTitleFontColor, "white");
  FetchNull(ReadTitleBgColor, "#003366");
  FetchNull(ReadTextFontColor, "black");
  FetchNull(ReadTextBgColor, "ivory");

  /* write */
  FetchNull(UploadEnable, "true");
  FetchNull(SubjectText, "");
  FetchNull(DefaultSubject, "");
  FetchNull(DefaultText,    "");
  FetchNull(UseCategory, "false");
  FetchNull(Category, "\n_TITLE = [Category]\n_NOTSELECTED = Etc.\n# NUMBER = STRING\n10 = Question\n20 = Answer\n30 = Info\n");
  FetchNull(WriteTitleFontColor, "white");
  FetchNull(WriteTitleBgColor, "#4c81ce");

  /* email */
  FetchNull(UseSysDefMail, "true");
  FetchNull(MailHead, "+ New article is posted.\n");
  FetchNull(MailTail, "+ Powered by 'Nobreak Technologies, Inc.'.\n");

  /* permissons */
  FetchNull(ListMode, "anybody");
  FetchNull(WriteMode, "anybody");
  FetchNull(ReadMode, "anybody");
  FetchNull(StatMode, "anybody");

  FetchZero(LastNum, 0);

#undef FetchNull
#undef FetchZero
#undef FetchNullMalloc

  DBClose();

  if(error == TRUE && strcmp(db->mode, "admin") && strcmp(db->mode, "adminsave")) {
    firstMeeting(sys->ProgramName, db->db);
  }

}

void firstMeeting(char *prgName, char *dbName) {
  qContentType("text/html");

  printf("<html>\n");
  printf("<head>\n");
  printf("  <title>%s %s / Nobreak Technologies, Inc.</title>\n", OFFICIAL_PROGRAM_NAME, OFFICIAL_PROGRAM_VERSION);
  printf("  <meta http-equiv='refresh' content=\"10; url='%s?db=%s&mode=admin'\">\n", prgName, dbName);
  printf("</head>\n");
  printf("\n");
  printf("<body bgcolor=white>\n");
  printf("  <center>\n");
  printf("  <br>\n");
  printf("  <font color=black size=3 face='Arial'>\n");
  printf("  <font color=darkblue size=5><B>%s %s</B></font><p>\n", OFFICIAL_PROGRAM_NAME, OFFICIAL_PROGRAM_VERSION);
  printf("  <b>This is your first contact, so you must set some configurations.<br>Click <a href='%s?db=%s&mode=admin'>Admin</a>. Or wait 10 seconds, please.</b><p>\n", prgName, dbName);
  printf("  <font size=2>\n");
  printf("    <font color=red><b>P</b></font><font color=pink>owered</font> by <font color=darkblue>Nobreak Technologies, Inc.</font>\n");
  printf("  </font>\n");
  printf("  </font>\n");
  printf("  </center>\n");
  printf("</body>\n");
  printf("</html>\n");
  exit(0);
}

/*******************************************
  ��Ű���� �����Ѵ�.
  ������ ���� ��츦 �����Ѵ�.
  1. ������ �ش� ��Ű������ ���� �������� ���� : �ǳʶ� 
  2. ������ �ش� ��Ű������ ���� �������� ���� : ���� 
  3. ������ �ش� ��Ű������ �ְ� �������� ���� : �ı� 
  4. ������ �ش� ��Ű������ �������� ����      : �ǳʶ� 
  5. ������ �ش� ��Ű������ �������� �ٸ�      : ���� 

  ���� ���� �ϴ������� �ݽ������� ��Ű������ ���������� ���� 
  �ߺ��� ��Ű ������ ������ �ּ�ȭ�ϱ� �����̴�.

  �� ����� �����ν� �̸��� �����ּ��� �ش縸������ �����ϰ� 
  �����ɼ� �ְ�, ������ ���� ���ؼ��� ������ �ǳʶٹǷ� 
  ��Ű �������� ����ϸ� �ٽ��ѹ� �Է��� ���־�� �Ѵ�.
*******************************************/
void setCookie(char *name, char *value, int expdays){
  char *cookie;
  int  set;

  if(value == NULL) return;

  cookie  = qcValue(name);

  if(cookie == NULL) { /* ������ ������ ��Ű���� ���� */
    /* value string�� ""�� �ƴϸ� �����Ѵ� */
    if(strcmp(value, "")) set = TRUE;
  }
  else {  /* ������ ������ ��Ű���� �����Ѵ� */
    /* value�� ""�̸� ������ ��Ű ���� */
    if(!strcmp(value,     "")) set = TRUE, expdays = -1;
    /* ������ ��Ű���� �ٸ� value�� �����Ǿ����� �ٲ۴� */
    else if(strcmp(value, cookie)) set = TRUE;
    else set = FALSE;
  }

  if(set == TRUE) qSetCookie(name, value, expdays, NULL, NULL, NULL);
}

int accessLog(char *filename) {
  FILE *fp;
  char *http_user_agent, *remote_host, *mode, *num;
  struct tm *time;

  time = qGetTime();
  http_user_agent = qGetEnv("HTTP_USER_AGENT", "unknown");
  if((remote_host = qGetEnv("REMOTE_HOST", getenv("REMOTE_ADDR"))) == NULL) remote_host = "unknown";

  if((mode        = qValue("mode")) == NULL) mode = "list";
  if((num         = qValue("num"))  == NULL) num  = "";

  if((fp = fopen(filename, "at")) == NULL) return FALSE;
  fprintf(fp, "%04d/%02d/%02d(%02d:%02d) mode=%-10s num=%-5s from %s (%s)\n", time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour, time->tm_min, mode, num, remote_host, http_user_agent);
  fclose(fp);

  return TRUE;
}

void updateStat(SysConfig *sys, DB *db) {
  struct tm *time;
  int stats;
  char buf[0xff];

  sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->StatDirectory, db->db, sys->StatExtension);
  if(qCheckFile(buf) == FALSE) DBCreate(buf, 0644);
  DBOpen(buf, GDBM_WRITER);

  time = qGetTime();

  if(DBFetch("stat.Started") == NULL) {
    sprintf(buf, "%04d%02d%02d%02d%02d%02d", time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
    DBStore("stat.Started", buf, GDBM_REPLACE);
  }

  sprintf(buf, "stat.%04d%02d%02d", time->tm_year, time->tm_mon, time->tm_mday);
  stats = DBiFetch(buf);
  DBiStore(buf, ++stats, GDBM_REPLACE);

  DBClose();
}

void headHtml(SysConfig *sys, DBConfig *conf) {
  /* mime header ��� */
  qContentType("text/html");

  /* HTML DTD ��� */
  /*
  printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n\n");
  */

  /* ���α׷� ���� ��� */
  /* programInfo()�� �����ڰ� ������ ������ �������� �ʴ��� �ݵ�� ��������
     ��µǾ�� �մϴ�.
     The function, programInfo() must be printed out. You can not delete or
     modify it until developer allow that.
  */
  programInfo();

  if(!strcmp(conf->UseSysDefHtml, "true") || !strcmp(sys->AllowDBhtml, "false")) {
    char *title, *charset;
    char buf[0xff];

    /* Header HTML File Path */
    sprintf(buf, "%s/%s", sys->RealBasePath, sys->HtmlHead);

    /* Browser Title : db title�� �������� system title�� ����Ѵ� */
    if(strcmp(conf->Title, "")) title = conf->Title;
    else title = sys->DefaultTitle;

    /* Character Set : db charset�� �������� system charset�� ����Ѵ� */
    if(strcmp(conf->CharSet, "")) charset = conf->CharSet;
    else charset = sys->DefaultCharSet;

    printf("<html>\n");
    printf("<head>\n");
    if(strcmp(title,   "")) printf("  <title>%s</title>\n", title);
    if(strcmp(charset, "")) printf("  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=%s\">\n", charset);
    if(!strcmp(conf->UnderlinedLink, "false")) {
      printf("  <style type='text/css'><!-- Disable Underlined Link -->\n");
      printf("  <!--\n");
      printf("    A:link { text-decoration:none }\n");
      printf("    A:visited { text-decoration:none }\n");
      printf("  -->\n");
      printf("  </style>\n");
    }
    printf("</head>\n");

    printf("\n<!-- HTML Header -->\n");
    qSendFile(buf);
    printf("\n");

    /* DB Message */
    printf("\n<!-- Caption -->\n");
    printf("<table border=0 width=%s><tr><th>\n", conf->TableWidth);
    printf("  <font size='%d'>%s</font>\n", atoi(conf->FontSize) + 1, conf->Message);
    printf("</th></tr></table>\n");
  }
  else {
    printf("%s\n", conf->HtmlHead);
  }

  printf("\n<!-- Script Start-->\n");
}

void tailHtml(SysConfig *sys, DBConfig *conf) {
  printf("\n<!-- Script End-->\n");

  if(!strcmp(conf->UseSysDefHtml, "true") || !strcmp(sys->AllowDBhtml, "false")) {
    char buf[0xff];
    sprintf(buf, "%s/%s", sys->RealBasePath, sys->HtmlTail);

    printf("\n<!-- HTML Tailer -->\n");
    qSendFile(buf);
    printf("\n");
    printf("</html>\n");
  }
  else {
    printf("%s\n", conf->HtmlTail);
  }

  runTime();
}

void headAdmin(void) {
  /* mime header ��� */
  qContentType("text/html");

  programInfo();

  printf("<html>\n");
  printf("<head>\n");
  printf("  <title>%s %s</title>\n", OFFICIAL_PROGRAM_NAME, OFFICIAL_PROGRAM_VERSION);
  printf("</head>\n");
  printf("\n");
  printf("<body bgcolor='white'>\n");
  printf("<center>\n");

  /* DB Message */
  /*
  printf("<font size=5 face='Arial'>%s %s</font><br>\n", OFFICIAL_PROGRAM_NAME, OFFICIAL_PROGRAM_VERSION);
  */

  printf("\n<!-- Script Start-->\n");
}

void tailAdmin(void) {
  printf("\n<!-- Script End-->\n");
  printf("<font size=2 face='Arial'>\n");
  printf("Copyright(c)1996-1998 <a href='http://www.nobreak.com' target='_top'>Nobreak Technologies, Inc.</a>, All rights reserved.<br>\n");
  printf("Official distribution site : <a href='http://cwb98.nobreak.com' target='_top'>http://cwb98.nobreak.com</a><br>\n");
  printf("</font>\n");

  printf("</center>\n");
  printf("</body>\n");
  printf("</html>\n");

  runTime();
}

void runTime(void) {
  clock_t RunTime;

  qContentType("text/html"); /* �̹� ȣ��Ǿ����� ����ƮŸ���� ��µ��� �ʴ´� */

  /* Running Time */
  RunTime = clock();  /* microsec */
  RunTime /= 1000;    /* milisec */

  printf("\n<!-- CPU Time %.2f seconds -->\n", (float)RunTime / (float)1000);
/*
  printf("\n<br><font size=2 color=red>CPU Time %.2f seconds</font><br>\n", (float)RunTime / (float)1000);
*/
}

unsigned int getPermission(char *filename) {
  struct stat buf;

  if(stat(filename, &buf) != 0) qError("File(%s) not found.", filename);
  buf.st_mode &= 007777;  /* bit masking */

  return (unsigned int)buf.st_mode; 
}

/* String���� "�� '�� `�� �ٲ۴� */
char *exchangeQuot(char *str) {
  int i;

  for(i = 0; str[i] != '\0'; i++) {
    if(str[i] == '\"' || str[i] == '\'') str[i] = '`';
  }

  return str;
}

/* ���̼���Ű �˻� */
int license_check(char *LicenseKey, char *SecureKey) {
  char script_path[0xff], *pt, buf[0xff];

  strcpy(script_path, strdup(qGetEnv("SCRIPT_NAME", "")));
  for(pt = script_path + strlen(script_path); pt >= script_path && *pt != '/'; pt--) *pt = '\0';
  sprintf(buf, "%s|http://%s:%s%s", SecureKey, qGetEnv("SERVER_NAME", ""), qGetEnv("SERVER_PORT", ""), script_path);

  if(!strcmp(LicenseKey, md5_crypt(buf))) return TRUE;
  return FALSE;
}


/******************************************************************************
   �ٷ� ��� ���⸦ ������� �ʰ�, �� �Ʒ��� ���� �������ø� ��Ű���� ���ص��� 
   �ʴ� ����ڴ� ������ �б� �ٶ���.

   �̷��� �ⱳ�� �θ��� ������ ����ũ�� ����Ʈ �ͽ��÷ξ� 4.0 �������� ȣȯ�� 
   �����̴�.
   �� �Խ����� ������� ���ϰ� ���� ����� ����, �޽����� ������� ��Ϻ��� 
   ȭ������ ���ƿ���, ��Ϻ��� ȭ�鿡�� ���� �дٰ� ǥ�õ� back �������� Ŭ���ϸ� 
   ���� ���� �Ѿ�� ȭ���� ������ŭ javascript:history.go(-n) �Ͽ� ��Ϻ��� ȭ������ 
   ���ƿ����� �Ǿ��ִ�. �̴� ��Ϻ��� ȭ������ ���ƿȿ� �־�, �ٽ� CGI�� ȣ���ϴ� 
   ������ ���̰��� ���̴�.

   �׷���, �ͽ��÷ξ�� �޽����� �������� ������ ȭ���� Form������ ������ ����� 
   �Ǵ��ϰ� ����ڰ� �Խù��� ���� �۾����� ������ ���� ������ BACK ��ư���� 
   �����丮 ���� �Ϸ��ϸ� �ɽõ� �������� �������� �ʰ� ����Ǿ��ٴ� �޼����� 
   �����.

   �׸��Ͽ�, �Խù��� ����� �������� �Ʒ��� �Լ��� �̿��Ͽ�, CGI�� �ٽ� �ѹ� 
   ȣ�������ν� ��Ϻ��� ȭ���� form�� ����� �������� ���������� �νĽ�Ű�� 
   �����̴�. Locate ����� ����� ����� ����ɼ� ������, �� ������ �̰����� 
   �ذ���� �ʱ⿡ �������ø� �����Ѵ�.

   �ͽ��÷ξ�� form�������� ĳ�ø� �����Ű�°��� �Ϲ����� url encoding Ÿ���� 
   form������ �ش����� �ʰ�, multipart/form-data Ÿ���� ���ۿ��� �ش��Ѵ�.
   �̴� ������ ���� ������ �߻��ϴµ��ϴ�.
   multipart/form-data�� ������ ������ �����ϱ� ����, �ӽ����Ͽ��� ������ ������ 
   ������ �װ��� �����ϰ� �ӽ������� �����ϴµ�, ����ڰ� �ٽ� �� �������� �䱸�ϸ� 
   �ͽ��÷ξ�� �ӽ� ������ �����Ǿ��µ� �� �������� �ٽÿ䱸�ϴ�, �������� ����������� 
   ������ �Ҿ��ٰ� �׷��� �޽����� �����µ� �ϴ�.

   �׷��� �� �Լ��� ������� ���� ������ ���� ȿ���� ������ �ִ�.
   �Խù��� �ۼ�����, ��Ϻ���ȭ���� �����µ�, ����ڰ� ȭ���� �ٽþ���� 
   �������� reload ��ư�� �����ٰ� ����. ������������, ���ܰ��� ������ 
   ��� �����ڳĴ� �޽����� �Բ�, �ߺ����� �����Ǵµ�, �̹���� ����ϸ� 
   �̷��� ������ �ذ�ȴ�.
******************************************************************************/
void relocateList(char *cginame, DB *db) {
  qContentType("text/html");

  printf("<html>\n");
  printf("<head>\n");

  printf("  <title>Please, wait a second while refreshing page %d</title>\n", db->page);

  printf("  <meta http-equiv=\"refresh\" content=\"0; url=");
  printf("%s?db=%s&mode=list&page=%d&category=%s&ftype=%d&fval=%s", cginame, db->db, db->page, db->category, db->ftype, db->fencval);
  printf("\">\n");

  printf("</head>\n");
  printf("</html>\n");
  exit(0);
}

/**********************************************************************
** �Ʒ��� programInfo() �Լ��� �����ϰų�, �����Ҽ� ������, �������� �� 
** ����� �ݵ�� ��µǾ�� �Ѵ�. ����Ʈ������ ����ó�� �����ϴ°��� �� 
** �������� ������ �䱸 ������ �ƴ�����, �������� ���� ���� �̸� ������ 
** ������� CWB�� ����� �㰡���� �ʴ´�.
**
** �����ڴ� ����Ʈ��� ���� �ŷڼ��ִ� ����/������ ����, editedInfo()
** �� �غ��Ͽ���. �ҽ��� �����ȴٸ�, editedInfo()�� ������, �Ҽ�ȸ��,
** ��������, ����ó���� ����ϱ� �ٶ���.
**
** You can not edit below function programInfo().
**********************************************************************/
void programInfo(void) {
  static int stat = FALSE;
  
  if(stat == TRUE) return;
  stat = TRUE;

  if(license_valid == TRUE) {
    printf("<!-- LICENSE KEY IS VERIFIED, AUTHENTICATED USER              -->\n");
  }
  else {
    printf("<!-- INVALID LICENSE KEY, Running under limited mode          -->\n");
  }

  printf("<!-------------------------------------------------------------->\n");
  printf("<!-- %-13s %-42s -->\n", OFFICIAL_PROGRAM_NAME, OFFICIAL_PROGRAM_VERSION);
  printf("<!--                                                          -->\n");
  printf("<!--  Official distribution site : http://cwb98.nobreak.com   -->\n");
  printf("<!--           Technical contact : support@nobreak.com        -->\n");
  printf("<!--                                                          -->\n");
  printf("<!--               Developed by 'Nobreak Technologies, Inc.'  -->\n");
  printf("<!--                                                          -->\n");
  printf("<!--      Copyright (c) 1996-1998 Nobreak Technologies, Inc.. -->\n");
  printf("<!-------------------------------------------------------------->\n");

  editedInfo();
  printf("\n");
}

/*****************************************************************************
** This is a example. You can edit this to describe your contact information.
*****************************************************************************/
void editedInfo(void) {

  return;       /* remove this line to activate this function */

  printf("<!-- This software is edited by 'Your Name' to implement      -->\n");
  printf("<!-- sorted list at January, 2001. And we use this software   -->\n");
  printf("<!-- at our homepage, http://japan.cgiserver.net              -->\n");
  printf("<!--                                                          -->\n");
  printf("<!-- You can contact to us via e-mail, YOUR@EMAIL.ADDRESS     -->\n");
  printf("<!-------------------------------------------------------------->\n");
}
