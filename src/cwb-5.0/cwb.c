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

  /* Timer 시작 */
  clock();

  /* Adjust umask */
  umask(S_IWGRP | S_IWOTH);

  /* get realtime config */
  sysp = getRealtimeConfig(&sys, REALTIME_CONFIG_FILE);

  /* Ensure CGI's permission 6755 */
  /*
  if(getPermission(sys.ProgramName) != 06755) qError("Wrong permission. %s must has setuid and setgid bit. You would better adjust it's permission by 'chmod 6755 %s' operation.", sys.ProgramName, sys.ProgramName);
  */

  /* default error log on : 쿼리 패치도중 발생하는 오류를 기록하기 위해*/
  if(strcmp(sys.LogDirectory, "")) {
    sprintf(sErrorLog,  "%s/%s/error_log",  sys.RealBasePath, sys.LogDirectory);
    qErrorLog(sErrorLog);
  }

  /* License Key 검사 */
#ifdef SECURE_LICENSE_KEY
  license_valid = license_check(sys.LicenseKey, SECURE_LICENSE_KEY);
#else
  license_valid = TRUE;
#endif

  /* 쿼리 패치 */
  qDecoder();

  /* get DB config */
  getConfig(&sys, &db, &conf);

  /* Set Cookie */
  if(!strcmp(sys.CookieEnable, "true")) {
    char *tmp;
    if(!strcmp(db.mode, "writesave")) { /* 성명, Email은 writesave 시에만 쿠킹 한다. */
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

  /* db log on : default log를 켜놓은 상태에서 getConfig()를 한후 
  실제 로그를 온하는 이유는, 존재하지 않는 db명으로의 쿼리에 대해 
  로그 파일을 생성하지 않기 위함이다. getConfig()에서 존재하지 않는 
  db로의 접근은 qError()로 차단되어 default log에 남고 끝난다. */
  if(strcmp(sys.LogDirectory, "")) {
    sprintf(sAccessLog, "%s/%s/access_log.%s", sys.RealBasePath, sys.LogDirectory, qValue("db"));
    sprintf(sErrorLog,  "%s/%s/error_log.%s",  sys.RealBasePath, sys.LogDirectory, qValue("db"));
    qErrorLog(sErrorLog);
    qErrorContact(conf.AdminEmail);
    if(accessLog(sAccessLog) == FALSE) qError("Access logging(%s) fail.", sAccessLog);
  }

  /* 접속 기록 */
  if(strcmp(sys.StatDirectory, "")) {
    updateStat(&sys, &db);
  }

  /* 모듈 분기 */
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

  /* 메모리 해제 */
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

  /* ftype을 조합하는 경우는 list 모듈에서만 한다, 나머지는 ftype을 가지고 전달한다 */
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
  ** DB에서 Configuration을 얻는다.
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
  쿠키값을 설정한다.
  다음과 같은 경우를 조정한다.
  1. 기존에 해당 쿠키정보가 없고 설정값이 공란 : 건너뜀 
  2. 기존에 해당 쿠키정보가 없고 설정값이 있음 : 설정 
  3. 기존에 해당 쿠키정보가 있고 설정값이 공란 : 파기 
  4. 기존에 해당 쿠키정보가 설정값이 같음      : 건너뜀 
  5. 기존에 해당 쿠키정보가 설정값과 다름      : 설정 

  위와 같이 하는이유는 넷스케입의 쿠키관리가 안정적이지 못해 
  중복된 쿠키 설정의 오류를 최소화하기 위함이다.

  이 방법을 씀으로써 이름과 우편주소의 해당만기일이 상이하게 
  설정될수 있고, 동일한 값에 대해서는 설정을 건너뛰므로 
  쿠키 만기일이 경과하면 다시한번 입력을 해주어야 한다.
*******************************************/
void setCookie(char *name, char *value, int expdays){
  char *cookie;
  int  set;

  if(value == NULL) return;

  cookie  = qcValue(name);

  if(cookie == NULL) { /* 기존에 설정된 쿠키값이 없다 */
    /* value string이 ""이 아니면 설정한다 */
    if(strcmp(value, "")) set = TRUE;
  }
  else {  /* 기존에 설정된 쿠키값이 존재한다 */
    /* value가 ""이면 기존의 쿠키 제거 */
    if(!strcmp(value,     "")) set = TRUE, expdays = -1;
    /* 기존의 쿠키값과 다른 value가 설정되었으면 바꾼다 */
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
  /* mime header 출력 */
  qContentType("text/html");

  /* HTML DTD 출력 */
  /*
  printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n\n");
  */

  /* 프로그램 정보 출력 */
  /* programInfo()는 제작자가 삭제및 수정에 동의하지 않는한 반드시 수정없이
     출력되어야 합니다.
     The function, programInfo() must be printed out. You can not delete or
     modify it until developer allow that.
  */
  programInfo();

  if(!strcmp(conf->UseSysDefHtml, "true") || !strcmp(sys->AllowDBhtml, "false")) {
    char *title, *charset;
    char buf[0xff];

    /* Header HTML File Path */
    sprintf(buf, "%s/%s", sys->RealBasePath, sys->HtmlHead);

    /* Browser Title : db title이 없을때는 system title을 사용한다 */
    if(strcmp(conf->Title, "")) title = conf->Title;
    else title = sys->DefaultTitle;

    /* Character Set : db charset이 없을때는 system charset을 사용한다 */
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
  /* mime header 출력 */
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

  qContentType("text/html"); /* 이미 호출되었으면 콘텐트타입은 출력되지 않는다 */

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

/* String에서 "와 '를 `로 바꾼다 */
char *exchangeQuot(char *str) {
  int i;

  for(i = 0; str[i] != '\0'; i++) {
    if(str[i] == '\"' || str[i] == '\'') str[i] = '`';
  }

  return str;
}

/* 라이센스키 검사 */
int license_check(char *LicenseKey, char *SecureKey) {
  char script_path[0xff], *pt, buf[0xff];

  strcpy(script_path, strdup(qGetEnv("SCRIPT_NAME", "")));
  for(pt = script_path + strlen(script_path); pt >= script_path && *pt != '/'; pt--) *pt = '\0';
  sprintf(buf, "%s|http://%s:%s%s", SecureKey, qGetEnv("SERVER_NAME", ""), qGetEnv("SERVER_PORT", ""), script_path);

  if(!strcmp(LicenseKey, md5_crypt(buf))) return TRUE;
  return FALSE;
}


/******************************************************************************
   바로 목록 보기를 출력하지 않고, 왜 아래와 같이 리프레시를 시키는지 이해되지 
   않는 사용자는 다음을 읽기 바란다.

   이러한 기교를 부리는 이유는 마이크로 소프트 익스플로어 4.0 버젼과의 호환성 
   때문이다.
   본 게시판은 사용자의 편리하고 빠른 사용을 위해, 메시지를 기록한후 목록보기 
   화면으로 돌아오고, 목록보기 화면에서 글을 읽다가 표시된 back 아이콘을 클릭하면 
   현재 까지 넘어온 화면의 갯수만큼 javascript:history.go(-n) 하여 목록보기 화면으로 
   돌아오도록 되어있다. 이는 목록보기 화면으로 돌아옴에 있어, 다시 CGI를 호출하는 
   지연을 줄이고자 함이다.

   그러나, 익스플로어는 메시지를 저장한후 생성된 화면을 Form에의해 생성된 결과로 
   판단하고 사용자가 게시물을 보는 작업등을 행한후 위에 설명한 BACK 버튼으로 
   히스토리 백을 하려하면 케시된 페이지를 보여주지 않고 만기되었다는 메세지를 
   남긴다.

   그리하여, 게시물을 기록한 다음에는 아래의 함수를 이용하여, CGI를 다시 한번 
   호출함으로써 목록보기 화면이 form의 결과로 생성되지 않은것으로 인식시키기 
   위함이다. Locate 헤더를 사용한 방법도 고려될수 있으나, 본 문제는 이것으로 
   해결되지 않기에 리프레시를 적용한다.

   익스플로어에서 form문에대한 캐시를 만기시키는것은 일반적인 url encoding 타입의 
   form문에는 해당하지 않고, multipart/form-data 타입의 전송에만 해당한다.
   이는 다음과 같은 이유로 발생하는듯하다.
   multipart/form-data는 쿼리를 서버에 전송하기 전에, 임시파일에다 전송할 포멧을 
   만든후 그것을 전송하고 임시파일을 삭제하는데, 사용자가 다시 그 페이지를 요구하면 
   익스플로어는 임시 파일이 삭제되었는데 그 페이지를 다시요구하니, 페이지를 만들었을때의 
   쿼리를 잃었다고 그러한 메시지를 보내는듯 하다.

   그러나 본 함수의 사용으로 인해 다음과 같은 효율을 얻을수 있다.
   게시물을 작성한후, 목록보기화면이 나오는데, 사용자가 화면을 다시얻고자 
   브라우저의 reload 버튼을 눌렀다고 하자. 기존같았으면, 전단계의 쿼리를 
   디시 보내겠냐는 메시지와 함께, 중복투고가 성립되는데, 이방법을 사용하면 
   이러한 문제가 해결된다.
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
** 아래의 programInfo() 함수는 수정하거나, 삭제할수 없으며, 개발자의 의 
** 도대로 반드시 출력되어야 한다. 소프트웨어의 제작처를 유지하는것은 관 
** 례적으로 무리한 요구 사항은 아니지만, 개발자의 동의 없이 이를 준하지 
** 않을경우 CWB의 사용은 허가되지 않는다.
**
** 개발자는 소프트웨어에 대한 신뢰성있는 유지/보수를 위해, editedInfo()
** 를 준비하였다. 소스가 수정된다면, editedInfo()에 수정인, 소속회사,
** 수정내역, 연락처등을 명시하길 바란다.
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
