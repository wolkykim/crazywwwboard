/*************************************************************
** CrazyWWWBoard Administrative Center                      **
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include "qDecoder.h"
#include "md5_crypt.h"
#include "adm.h"
#include "adm_auth.h"
#include "adm_newpass.h"
#include "adm_list.h"
#include "adm_remove.h"
#include "adm_create.h"
#include "adm_message.h"
#include "adm_config.h"

Entry *getRealtimeConfig(SysConfig *sys, char *filename);
void  getConfig(DB *db, char *cryptSalt);
int   accessLog(char *filename);
unsigned int getPermission(char *filename);
int   license_check(char *LicenseKey, char *SecureKey);
void  programInfo(void);

int license_valid = FALSE;
static clock_t RunTime;

int main(int argc, char **argv){
  Entry *sysp;
  SysConfig sys;
  DB db;
  char sAccessLog[0xff], sErrorLog[0xff];

  /* Timer 시작 */
  clock();

  /* Adjust umask */
  umask(S_IWGRP | S_IWOTH);

  /* ger realtime config */
  sysp = getRealtimeConfig(&sys, REALTIME_CONFIG_FILE);

  /* Ensure CGI's permission 6755 */
  /*
  if(getPermission(sys.AdmPrgName) != 06755) qError("Wrong permission. %s must has setuid and setgid bit. You would better adjust it's permission by 'chmod 6755 %s' operation.", sys.AdmPrgName, sys.AdmPrgName);
  */

  /* log on */
  if(strcmp(sys.LogDirectory, "")) {
    sprintf(sErrorLog,  "%s/%s/adm_error_log",  sys.RealBasePath, sys.LogDirectory);
    sprintf(sAccessLog, "%s/%s/adm_access_log", sys.RealBasePath, sys.LogDirectory);
    qErrorLog(sErrorLog);
    qErrorContact(sys.AdminEmail);
    if(accessLog(sAccessLog) == FALSE) qError("Access logging(%s) fail.", sAccessLog);
  }

  /* 쿼리 패치 */
  qDecoder();

  /* Query fetch */
  getConfig(&db, sys.CryptSalt);

  /* License Key 검사 */
#ifdef SECURE_LICENSE_KEY
  license_valid = license_check(sys.LicenseKey, SECURE_LICENSE_KEY);
#else
  license_valid = TRUE;
#endif

  /* User authentication : 인증이 확인되지 않으면 복귀되지 않는다 */
  adm_auth(&sys, &db);

  /* Jump to each module */
       if(!strcmp(db.mode, "list"))         adm_list(&sys, &db);
  else if(!strcmp(db.mode, "rm_log"))       adm_rm_log(&sys, &db), adm_list(&sys, &db);
  else if(!strcmp(db.mode, "rm_stat"))      adm_rm_stat(&sys, &db), adm_list(&sys, &db);
  else if(!strcmp(db.mode, "rm_db"))        adm_rm_db(&sys, &db), adm_list(&sys, &db);
  else if(!strcmp(db.mode, "create"))       adm_create(&sys, &db), adm_list(&sys, &db);
  else if(!strcmp(db.mode, "message"))      adm_message(&sys, &db);
  else if(!strcmp(db.mode, "messagesave"))  adm_messagesave(&sys, &db), closeWindow();
  else if(!strcmp(db.mode, "config"))       adm_config(&sys, &db);
  else if(!strcmp(db.mode, "configsave"))   adm_configsave(&sys, &db), closeWindow();
  else if(!strcmp(db.mode, "newpass"))      adm_newpass(&sys, &db);
  else if(!strcmp(db.mode, "newpasssave"))  adm_newpasssave(&sys, &db), adm_list(&sys, &db);
  else qError("'Do not use manually.");

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

  if((sys->AdmPrgName = qCGIname()) == NULL) sys->AdmPrgName = OFFICIAL_ADMCGI_NAME;

#define FetchConf(NAME) if((sys->NAME = qfValue(sysp, #NAME)) == NULL) qError("There is no entry named '%s' in realtime configuration file.", #NAME)

  FetchConf(LicenseKey);

  FetchConf(ProgramName);

  FetchConf(AdminName);
  FetchConf(AdminEmail);

  FetchConf(RealBasePath);
  FetchConf(HtmlBasePath);

  FetchConf(DataDirectory);
  FetchConf(DataExtension);

  FetchConf(StatDirectory);
  FetchConf(StatExtension);

  FetchConf(LogDirectory);

  FetchConf(HtmlHead);
  FetchConf(HtmlTail);

  FetchConf(MailHead);
  FetchConf(MailTail);

  FetchConf(CryptSalt);

  FetchConf(IconAdminTitle);
  FetchConf(IconReload);
  FetchConf(IconBack);

#undef FetchConf

  return sysp;
}

void  getConfig(DB *db, char *cryptSalt) {
  if( (db->db       = qValue("db"))       == NULL ) db->db       = "";
  if( (db->mode     = qValue("mode"))     == NULL ) db->mode     = "list";

  /* password */
  if( (db->password = qValue("password")) == NULL ) db->password = "";
  else if((strlen(db->password) > 0) && (strlen(db->password) <= MAX_PASSWORD_LEN)) {
    db->password = crypt(db->password, cryptSalt);
  }
}

void headHtml(SysConfig *sys) {
  /* mime header 출력 */
  qContentType("text/html");

  /* HTML DTD 출력 */
  /*
  printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n\n");
  */

  /* 프로그램 정보 출력 */
  programInfo();

  printf("<html>\n");
  printf("<head>\n");
  printf("  <title>CrazyWWWBoard Administrative Center</title>\n");
/*
  printf("  <meta http-equiv='pragma' content='no-cache'>\n");
*/
  printf("\n");

  printf("  <script language='JavaScript'>\n");
  printf("  <!--\n");
  printf("    var child = null;\n");
  printf("    var count = 0;\n");
  printf("\n");
  printf("    function fork ( type , url ) {\n");
  printf("      var childname = 'CWB98PE' + count++;\n");
  printf("\n");
  printf("      if(child != null) {    // child was created before.\n");
  printf("        if(!child.closed) {  // if child window is still opened, close window.\n");
  printf("          child.close();\n");
  printf("        }\n");
  printf("      }\n");
  printf("      // here, we can ensure that child window is closes.\n");
  printf("           if(type == 'view'   ) child = window.open(url, childname, 'toolbar=no,location=no,directories=no,status=yes,menubar=no,scrollbars=yes,resizable=yes,copyhistory=no,width=700,height=600');\n");
  printf("      else if(type == 'stat'   ) child = window.open(url, childname, 'toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=yes,resizable=yes,copyhistory=no,width=700,height=560');\n");
  printf("      else if(type == 'conf'   ) child = window.open(url, childname, 'toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=yes,resizable=yes,copyhistory=no,width=700,height=600');\n");
  printf("      else if(type == 'sync'   ) child = window.open(url, childname, 'toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=yes,resizable=yes,copyhistory=no,width=500,height=240');\n");
  printf("      else if(type == 'log'    ) child = window.open(url, childname, 'toolbar=no,location=no,directories=no,status=yes,menubar=no,scrollbars=yes,resizable=yes,copyhistory=no,width=700,height=600');\n");
  printf("      else if(type == 'message') child = window.open(url, childname, 'toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=yes,resizable=yes,copyhistory=no,width=700,height=600');\n");
  printf("      else if(type == 'config' ) child = window.open(url, childname, 'toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=yes,resizable=yes,copyhistory=no,width=700,height=540');\n");
  printf("      else                       alert('Fatal: in function fork()');\n");
  printf("      return;\n");
  printf("    }\n");
  printf("\n");
  printf("    function term() {\n");
  printf("      if(child != null) {\n");
  printf("        if(!child.closed) child.close();\n");
  printf("      }\n");
  printf("      return;\n");
  printf("    }\n");
  printf("  // -->\n");
  printf("  </script>\n");
  printf("</head>\n");
  printf("\n");

  printf("<body bgcolor='white' text='black' link='#ec5c05' vlink='#ec5c05' alink='red'>\n");
  printf("<center>\n");

  /* DB Message */
  printf("<br><a href='http://cwb98.nobreak.com'><img src='%s/%s' alt='%s %s' border=0></a><br>\n", sys->HtmlBasePath, sys->IconAdminTitle, OFFICIAL_ADMPRG_NAME, OFFICIAL_ADMPRG_VERSION);

  if(license_valid == FALSE) {
    struct utsname info;
    char script_path[0xff], *pt;
  
    uname(&info);

    strcpy(script_path, qGetEnv("SCRIPT_NAME", ""));
    for(pt = script_path + strlen(script_path); pt >= script_path && *pt != '/'; pt--) *pt = '\0';

    printf("<table border=0>\n");
    printf("<tr><td align='center'>\n");
    printf("<a href='http://cwb98.nobreak.com'><font size=3 face='Arial' color='red'><b>\n");
    printf("INVALID LICENSE KEY, RUNNING UNDER LIMITED MODE\n");
    printf("</b></font></a><br>\n");
    printf("</td></tr>\n");
    printf("<tr><td align='center'>\n");
    printf("<table><form method='get' action='http://register.nobreak.com/form_reg.shtml'><tr><td>\n");
    printf("  <font size=2>\n");
    printf("  <input type='hidden' name='product'  value='%s %s (%s)'>\n", OFFICIAL_PROGRAM_NAME, OFFICIAL_PROGRAM_VERSION, PLATFORM);
    printf("  <input type='hidden' name='url'      value='http://%s:%s%s'>\n", qGetEnv("SERVER_NAME", ""), qGetEnv("SERVER_PORT", ""), script_path);
    printf("  <input type='hidden' name='hostname' value='%s'>\n", info.nodename);
    printf("  <input type='hidden' name='platform' value='%s %s %s'>\n", info.sysname, info.release, info.machine);
    printf("  <input type='hidden' name='name'     value='%s'>\n", sys->AdminName);
    printf("  <input type='hidden' name='email'    value='%s'>\n", sys->AdminEmail);
    printf("  <input type='submit' value='    R E G I S T E R  (Getting LICENSE KEY)    '>\n");
    printf("  </font>\n");
    printf("</td></tr></form></table>\n");
    printf("</td></tr>\n");
    printf("</table>\n");
  }
  printf("<hr size=1 width='%s' noshade>\n", LIST_WIDTH);

  printf("\n<!-- Script Start-->\n");
}

void tailHtml(void) {
  printf("\n<!-- Script End-->\n");

  printf("<hr size=1 width='%s' noshade>\n", LIST_WIDTH);
  printf("<a href='http://www.nobreak.com' target='_top'><font size=2 face='Arial'>(c) Nobreak Technologies, Inc.</font></a>\n");

  printf("</center>\n");
  printf("</body>\n");
  printf("</html>\n");

  /* Running Time */
  RunTime = clock();  /* microsec */
  RunTime /= 1000;    /* milisec */
  printf("\n<!-- CPU Time %.2f seconds -->\n", (float)RunTime / (float)1000);

}

void closeWindow(void) {
  /* mime header 출력 */
  qContentType("text/html");

  printf("<html>\n");
  printf("<head>\n");

  printf("  <script language='JavaScript'>\n");
  printf("    function clickclose() {\n");
  printf("      close();\n");
  printf("    }\n");
  printf("  </script>\n");
  printf("\n");

  printf("</head>\n");
  printf("\n");

  printf("<body onLoad=\"clickclose()\">\n");
  printf("</body>\n");
  printf("</html>\n");
}

int accessLog(char *filename) {
  FILE *fp;
  char *http_user_agent, *remote_host;
  struct tm *time;

  time = qGetTime();
  http_user_agent = qGetEnv("HTTP_USER_AGENT", "unknown");
  if((remote_host = qGetEnv("REMOTE_HOST", getenv("REMOTE_ADDR"))) == NULL) remote_host = "unknown";

  if((fp = fopen(filename, "at")) == NULL) return FALSE;
  fprintf(fp, "%04d/%02d/%02d(%02d:%02d) from %s (%s)\n", time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour, time->tm_min, remote_host, http_user_agent);
  fclose(fp);

  return TRUE;
}

unsigned int getPermission(char *filename) {
  struct stat buf;

  if(stat(filename, &buf) != 0) qError("File(%s) not found.", filename);
  buf.st_mode &= 007777;

  return (unsigned int)buf.st_mode; 
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

void programInfo(void) {
  struct utsname info;
  static int stat = FALSE;
  char buf[0xff];
  
  if(stat == TRUE) return;
  stat = TRUE;
  uname(&info);
  sprintf(buf, "Running on %s(%s) %s, %s", info.sysname, info.nodename, info.release, info.machine);

  printf("<!-------------------------------------------------------------->\n");
  printf("<!-- CrazyWWWBoard Administrative Center : %10s %-5s   -->\n", OFFICIAL_ADMPRG_NAME, OFFICIAL_ADMPRG_VERSION);
  printf("<!--                                                          -->\n");
  printf("<!--  Official distribution site : http://cwb98.nobreak.com   -->\n");
  printf("<!--           Technical contact : support@nobreak.com        -->\n");
  printf("<!--                                                          -->\n");
  printf("<!--               Developed by 'Nobreak Technologies, Inc.'  -->\n");
  printf("<!--                                                          -->\n");
  printf("<!--      Copyright (c) 1996-1998 Nobreak Technologies, Inc.. -->\n");
  printf("<!-------------------------------------------------------------->\n");
  printf("<!-- %56s -->\n", buf);
  printf("<!-------------------------------------------------------------->\n");

  printf("\n");
}
