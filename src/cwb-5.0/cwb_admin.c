#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "cwb.h"
#include "cwb_admin.h"

#define TITLE_BGCOLOR	"#ccccff"
#define TITLE_BGCOLOR2	"#99ccff"
#define TITLE_BGCOLOR3	"#fa98ed"
#define TITLE_BGCOLOR4	"#66cccc"
#define ENTRY_BGCOLOR	"#dff0ff"

void password_form(SysConfig *sys, DB *db, DBConfig *conf);
void admin_form(SysConfig *sys, DB *db, DBConfig *conf);

void _t_textarea(char *title, char *key, char *value, int colspan);
void _t_radio(char *title, char *key, char *value, int colspan, char *str);
void _t_input(char *title, char *key, char *value, int colspan, int length, int maxlength, char *msg);
void _t_input2(char *title, char *key, char *value, int colspan, int length, int maxlength, char *title2, char *key2, char *value2, int length2, int maxlength2);
void _t_input22(char *title, char *key, char *value, int length, int maxlength, char *title2, char *key2, char *value2, int length2, int maxlength2);

void t_title(char *title, int colspan, char *bgcolor);
void t_hr(int colspan);
#define t_textarea(TITLE, KEY, COLSPAN) _t_textarea(TITLE, #KEY, conf->KEY, COLSPAN);
#define t_radio(TITLE, KEY, COLSPAN, STR) _t_radio(TITLE, #KEY, conf->KEY, COLSPAN, STR);
#define t_input(TITLE, KEY, COLSPAN, LENGTH, MAXLENGTH, MSG) _t_input(TITLE, #KEY, conf->KEY, COLSPAN, LENGTH, MAXLENGTH, MSG);
#define t_input2(TITLE, KEY, COLSPAN, LENGTH, MAXLENGTH, TITLE2, KEY2, LENGTH2, MAXLENGTH2) _t_input2(TITLE, #KEY, conf->KEY, COLSPAN, LENGTH, MAXLENGTH, TITLE2, #KEY2, conf->KEY2, LENGTH2, MAXLENGTH2);
#define t_input22(TITLE, KEY, LENGTH, MAXLENGTH, TITLE2, KEY2, LENGTH2, MAXLENGTH2) _t_input22(TITLE, #KEY, conf->KEY, LENGTH, MAXLENGTH, TITLE2, #KEY2, conf->KEY2, LENGTH2, MAXLENGTH2);

/* ∫Òπ–π¯»£∏¶ ¡∂»∏«œ¥¬ gateway¿Ã¥Ÿ. ∫Òπ–π¯»£∞° ªÛ¿Ã«“∞ÊøÏ ∫Òπ–π¯»£ ¿‘∑¬ ∆˚¿ª √‚∑¬«—¥Ÿ */
int check_password(SysConfig *sys, DB *db, DBConfig *conf){
  /* DBø° ∫Òπ–π¯»£∞° æ¯¿Ω, √ ±‚ ∫Òπ–π¯»£ ¡¶¿€¿ª ¿ß«ÿ ∫π±Õ */
  if(conf->Password == NULL) return PASS_NOT_DEFINE;

  /* ∫Òπ–π¯»£ feedback(password_form()¿∏∑Œ∫Œ≈Õ)¿Ã æ¯¿∏π«∑Œ »Æ¿Œ»≠∏È √‚∑¬ */
  if(db->password == NULL) password_form(sys, db, conf);

  /* feedbackµ«æ˙¥Ÿ, æœ»£»≠µ» ∏∂Ω∫≈Õ ∫Òπ–π¯»£øÕ ∫Ò±≥«—¥Ÿ */
  /* ¿‘∑¬µ» ∫Òπ–π¯»£¥¬ cwb.cø°º≠ æœ»£»≠ø©∫Œ∏¶ ∆«¥‹«—»ƒ æœ»£»≠ µ«æÓ ¿¸¥ﬁµ»¥Ÿ. */
  if(!strcmp(conf->Password, db->password)) return PASS_ADMIN;

  /* ∞‘Ω√π∞¿Ã ¥ÎªÛ¿œ∞ÊøÏ ∞‘Ω√π∞¿« ∫Òπ–π¯»£øÕ ∫Ò±≥«—¥Ÿ */
  if(db->num != 0) {
    int passflag = FALSE;
    if(!strcmp(db->mode, "modify"))     passflag = TRUE;
    if(!strcmp(db->mode, "delete"))     passflag = TRUE;
    if(!strcmp(db->mode, "modifysave")) passflag = TRUE;
    if(passflag == TRUE) {
      char *aPassword, buf[0xFF];

      sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
      DBOpen(buf, GDBM_READER);
      if((aPassword = DBFetch("%d.Password", db->num)) == NULL) password_form(sys, db, conf);
      DBClose();

      if(!strcmp(aPassword, "")) password_form(sys, db, conf);

      if(!strcmp(aPassword, db->password)) return PASS_POSTER;
    }
  }

  password_form(sys, db, conf);
  return PASS_FAIL;
}

void password_form(SysConfig *sys, DB *db, DBConfig *conf){
  char button_msg[0xff], handle_num[10 + 1];

  if(strcmp(db->mode, "admin") && db->num != 0) sprintf(handle_num, "(%d)", db->num);
  else strcpy(handle_num, "");
  sprintf(button_msg, "Execute '%s%s' operation", db->mode, handle_num);

  headHtml(sys, conf);

  printf("<table border=0>\n");

  printf("<form method='post' action='%s'>\n", sys->ProgramName);
  printf("  <input type='hidden' name='db'         value='%s'>\n", db->db);
  printf("  <input type='hidden' name='mode'       value='%s'>\n", db->mode);
  printf("  <input type='hidden' name='num'        value='%d'>\n", db->num);
  printf("  <input type='hidden' name='page'       value='%d'>\n", db->page);
  printf("  <input type='hidden' name='category'   value='%s'>\n", db->category);
  printf("  <input type='hidden' name='ftype'      value='%d'>\n", db->ftype);
  printf("  <input type='hidden' name='fval'       value='%s'>\n", db->fval);
  printf("  <input type='hidden' name='backdepth'  value='%d'>\n", db->backdepth + 1);

  printf("  <tr><td colspan=2><br></td></tr>\n");

  printf("  <tr>\n");
  printf("    <td align=right><font size=2 face='arial'><B>P a s s w o r d</B></font></td>\n");
  printf("    <td align=left><font size=3><input type='password' name='password' size=20 maxlength=%d></font></td>\n", MAX_PASSWORD_LEN);
  printf("  </tr>\n");

  printf("  <tr><td colspan=2><br></td></tr>\n");

  printf("  <tr>\n");
  printf("    <td align=center colspan=2><font size=2><input type='submit' value=\"%s\"); return true;\"></font></td>\n", button_msg);
  printf("  </tr>\n");

  printf("  <tr><td colspan=2><br></td></tr>\n");

  printf("  <tr>\n");
  printf("    <td align=center colspan=2><a href='javascript:history.go(-%d)' onMouseOver=\"window.status=('Go back'); return true;\"><img src='%s/%s' border=0 alt='Back'></a></td>\n", db->backdepth, sys->HtmlBasePath, sys->IconBack);
  printf("  </tr>\n");
  printf("</form>\n");
  printf("</table>\n");

  tailHtml(sys, conf);
  exit(0);
}

void cwb_admin(SysConfig *sys, DB *db, DBConfig *conf){
  check_password(sys, db, conf);
  admin_form(sys, db, conf);
}

#define ADMIN_WIDTH	"90%"
void admin_form(SysConfig *sys, DB *db, DBConfig *conf){
  char *submit_msg;

  /* ∏ﬁΩ√¡ˆ ∏∏µÎ */
  if(!strcmp(conf->Language, "korean"))        submit_msg = "¡÷¿Œ¥‘ æ»≥Á»˜ ∞°Ω Ω√ø‰.";
  else if(!strcmp(conf->Language, "japanese")) submit_msg = "É}ÉXÉ^ÅIÇ≥ÇÊÇ»ÇÁÅIÅI";
  else                                         submit_msg = "Good bye my master.";

  headAdmin();
  printf("<br><a href='http://cwb98.nobreak.com'><img src='%s/%s' alt='%s %s' border=0></a><br>\n", sys->HtmlBasePath, sys->IconAdminTitle, OFFICIAL_PROGRAM_NAME, OFFICIAL_PROGRAM_VERSION);

  printf("\n<!-- Form -->\n");
  printf("<form method='post' action='%s'>\n", sys->ProgramName);
  printf("  <input type='hidden' name='db' value='%s'>\n", db->db);
  printf("  <input type='hidden' name='mode' value='adminsave'>\n");
  if(db->password != NULL)printf("  <input type='hidden' name='password' value='%s'>\n", db->password);

  printf("\n<!-- Table -->\n");
  printf("<table border=0 width=%s>\n", ADMIN_WIDTH);
  t_hr(4);

   /***************** Web Board Information *********************/
  t_title("Web Board Information", 4, TITLE_BGCOLOR);
  printf("  <tr><th bgcolor='%s'><font size=2 face='arial'>New Password</font></th><td colspan=3><input type='password' name='cPassword' size=8 maxlength=%d> <font size=2 face='arial'><b>Re-enter Password</b></font> <input type='password' name='cPassword2' size=8 maxlength=%d></td></tr>\n", ENTRY_BGCOLOR, MAX_PASSWORD_LEN, MAX_PASSWORD_LEN);
  t_input("Browser Title", Title, 3, 60, 800, NULL);
  t_input("Title Message", Message, 3, 60, 800, NULL);
  t_input2("Homepage URL", HomeUrl, 3, 40, 100, "Target", HomeTarget, 10, 20);
  t_input2("Back URL", BackUrl, 3, 40, 100, "Target", BackTarget, 10, 20);

  t_hr(4);

  /***************** Administrator Contact Information *********************/
  t_title("Administrator Contact Information", 4, TITLE_BGCOLOR);
  t_input("Administrator Name", AdminName, 3, 60, 30, NULL);
  t_input("Administrator E-mail", AdminEmail, 3, 60, 60, NULL);
  t_radio("Mail to Administrator", MailToAdmin, 3, "true=Yes, I want to get a posting article by e-mail\nfalse=No don't do that");
  t_radio("Mail to Poster",        MailToSender, 3, "true=Yes, post reply article to original poster by e-mail\nfalse=No, don't do that");

  t_hr(4);

  /***************** User Interface / Common Module *********************/
  t_title("User Interface / Common", 4, TITLE_BGCOLOR2);
  t_radio("Language",  Language, 3, "english=English\nkorean=Korean\njapanese=Japanese");
  t_radio("Font Size", FontSize, 3, "2=General\n3=Big\n4=Huge");
  t_radio("Underlined Link", UnderlinedLink, 3, "false=Use plane link\ntrue=Use underlined link");
  t_input("Table Width", TableWidth, 3, 10, 4, "Ex) pixels 700 , percentage 80%");
  t_input("Character Set", CharSet, 3, 10, 40, "Ex) English 'iso-8859-1', Korean 'euc-kr', Japanese 'x-sjis'");
  /* UseSysDefHtml, Head, Tail HTML */
  if(!strcmp(sys->AllowDBhtml, "true")) {
    t_radio("HTML Header, Tailer", UseSysDefHtml, 3, "true=I want to use system default\nfalse=I want to use my own html below");
    t_textarea("HTML Header", HtmlHead, 3);
    t_textarea("HTML Tailer", HtmlTail, 3);
  }
  else {
    printf("  <tr><th bgcolor='%s'><font size=2 face='arial'>HTML Header, Tailer</font></th><td colspan=3 align=center><font size=2 face='arial' color=red><i>Disabled by system administrator, system default HTML will be applied</i></font></td></tr>\n", ENTRY_BGCOLOR);
    printf("  <input type='hidden' name='cUseSysDefHtml' value='true'>\n");
    printf("  <input type='hidden' name='cHtmlHead' value=''>\n");
    printf("  <input type='hidden' name='cHtmlTail' value=''>\n");
  }

  /***************** User Interface / List Modele *********************/
  t_title("User Interface / List Module", 4, TITLE_BGCOLOR2);
  t_radio("List Order", ListOrder, 3, "thread=By thread\ndate=By date");
  t_input("Articles / Page", ArticlesPerPage, 3, 10, 3, NULL);
  t_radio("Field Number", FieldNumber, 3, "virtual=Show virtual number\nphysical=Show physical number\nhidden=Hide number");
  t_radio("Field Type Icon", FieldFiletype, 3, "show=Show file type icon\nhidden=Hide file type icon");
  t_radio("Field Filename", FieldFilename, 3, "show=Show filename field\nhidden=Hide filename field");
  t_radio("Field Name", FieldName, 3, "after=Locate after subject\nbefore=Locate before subject\nhidden=Hide name field");
  t_radio("Field Subject", FieldSubject, 3, "wrap=Display whole subject words\ncut=Cut long subject by using &lt;nobr&gt; tag");
  t_radio("Field Date", FieldDate, 3, "show=Show date field\nhidden=Hide date field");
  t_radio("Field Hits", FieldHits, 3, "show=Show hits field\nhidden=Hide hits field");
  t_radio("Field Download", FieldDownload, 3, "show=Show download count field\nhidden=Hide download count field");
  t_radio("Folder Icon", FolderIcon, 3, "true=Show folder icon\nfalse=Hide folder icon");
  t_radio("Highlight Article", HighlightArticle, 3, "0=Today\n1=1\n2=2\n3=3\n4=4\n5=5 days ago\n-1=Disable");
  t_radio("Hide Search", HideSearch, 3, "false=I want to enable search\ntrue=I want to disable search");
  t_radio("Icon Location", ListIconLocation, 3, "bottom=Bottom\ntop=Top\nboth=Both");
  t_radio("Border Size", ListBorderSize, 3, "0=None\n1=Thin\n2=Light medium\n3=Medium\n4=Heavy\n5=Very heavy");
  t_input22("Title Font Color", ListTitleFontColor, 16, 10, "Title Bg Color", ListTitleBgColor, 16, 10);
  t_input22("Article Font Color", ListArticleFontColor, 16, 10, "Article Bg Color", ListArticleBgColor, 16, 10);

  /***************** User Interface / Read Module *********************/
  t_title("User Interface / Read Module", 4, TITLE_BGCOLOR2);
  t_radio("HTML Mode", HtmlAccept, 3, "false=Ignore HTML & Auto Link\ntrue=Accept HTML");
  t_radio("Link Target", LinkTarget, 3, "_top=Full Screen\n=Current Frame");
  t_radio("IP Address", DisplayAddress, 3, "true=Show and enable remote IP address\nfalse=Hide and disable remote IP address");
  t_radio("Icon Location", ReadIconLocation, 3, "auto=Auto\nbottom=Bottom\ntop=Top\nboth=Both");

/*
  ∏Ó∏Ó platformø°º≠¿« CSS «¡∏∞∆√ πÆ¡¶∑Œ ¿”Ω√ µ¿˙∫Ì«—¥Ÿ.
  t_radio("Line Height", ReadLineHeight, 3, "10=10 point\n12=12 point\n14=14 point\n16=16 point\n18=18 point");
*/
  printf("  <input type='hidden' name='cReadLineHeight' value='12'>\n");

  t_radio("Title Border Size", ReadTitleBorderSize, 3, "0=None\n1=Thin\n2=Light medium\n3=Medium\n4=Heavy\n5=Very heavy");
  t_radio("Text Border Size", ReadTextBorderSize, 3, "0=None\n1=Thin\n2=Light medium\n3=Medium\n4=Heavy\n5=Very heavy");
  t_input22("Title Font Color", ReadTitleFontColor, 16, 10, "Title Bg Color", ReadTitleBgColor, 16, 10);
  t_input22("Text Font Color", ReadTextFontColor, 16, 10, "Text Bg Color", ReadTextBgColor, 16, 10);

  /***************** User Interface / Write Modele *********************/
  t_title("User Interface / Write Module", 4, TITLE_BGCOLOR2);

  /* Upload Enable */
  if(!strcmp(sys->AllowUpload, "true")) {
    t_radio("File Upload", UploadEnable, 3, "true=Enable file uploading\nfalse=Disable file uploading");
  }
  else {
    printf("  <tr><th bgcolor='%s'><font size=2 face='arial'>File Upload</font></th><td align=center colspan=3><font size=2 face='arial' color=red><i>Disabled by system administrator</i></font></td></tr>\n", ENTRY_BGCOLOR);
    printf("  <input type='hidden' name='cUploadEnable' value='false'>\n");
  }    

  t_input("Subject Text", SubjectText, 3, 10, 60, "Ex) 'Vender', 'Topic'");
  t_input("Default Subject", DefaultSubject, 3, 60, 50, NULL);
  t_textarea("Default Text", DefaultText, 3);
  t_radio("Category", UseCategory, 3, "false=Disable category selection\ntrue=Enable category selection");
  t_textarea("Category Definition", Category, 3);
  t_input22("Title Font Color", WriteTitleFontColor, 16, 10, "Title Bg Color", WriteTitleBgColor, 16, 10);

  /***************** User Interface / Mail Modele *********************/
  t_title("User Interface / Mail Module", 4, TITLE_BGCOLOR2);

  if(!strcmp(sys->AllowDBmail, "true")) {
    t_radio("Email Header, Tailer", UseSysDefMail, 3, "true=I want to use system default\nfalse=I want to use my own signature below");
    t_textarea("Email Header", MailHead, 3);
    t_textarea("Email Tailer", MailTail, 3);
  }
  else {
    printf("  <tr><th bgcolor='%s'><font size=2 face='arial'>Email Header, Tailer</font></th><td colspan=3 align=center><font size=2 face='arial' color=red><i>Disabled by system administrator, system default signature will be applied</i></font></td></tr>\n", ENTRY_BGCOLOR);
    printf("  <input type='hidden' name='cUseSysDefMail' value='true'>\n");
    printf("  <input type='hidden' name='cMailHead' value=''>\n");
    printf("  <input type='hidden' name='cMailTail' value=''>\n");
  }    

  t_hr(4);

 /***************** Access Permissions *********************/
  t_title("Access Permissions", 4, TITLE_BGCOLOR3);
  t_radio("Listing", ListMode, 3, "anybody=Anybody can access\nadmin=Only administrator can access");
  t_radio("Posting", WriteMode, 3, "anybody=Anybody can write\nadmin=Only administrator can write");
  t_radio("Reading", ReadMode, 3, "anybody=Anybody can read\nadmin=Only administrator can read");
  t_radio("Statistics", StatMode, 3, "anybody=Anybody can view statistics\nadmin=Only administrator can view statistics");

  t_hr(4);

  printf("</table>\n");

  printf("\n<!-- Save -->\n");
  printf("<table border=0 width=%s>\n", ADMIN_WIDTH);
  printf("  <tr><td align=left>\n");
  printf("      <font size=2>\n");
  printf("      <input type='reset' value='  Reset  '>\n");
  printf("      </font>\n");
  printf("      </td>\n");

  printf("      <td align=center>\n");
  printf("      <font size=2>\n");
  printf("      <input type='submit' value='              S a v e              ' onClick=\"alert('%s')\">\n", submit_msg);
  printf("      </font>\n");
  printf("      </td>\n");

  printf("      <td align=right>\n");
  printf("      <a href='%s?db=%s' onMouseOver=\"window.status=('Back to the list'); return true;\"><img src='%s/%s' border=0 alt='List'></a>\n", sys->ProgramName, db->db, sys->HtmlBasePath, sys->IconList);
  printf("      </td>\n");
  printf("  </tr>\n");

  printf("</table>\n");
  printf("</form>\n");

  /****** Database Administration ******/
  printf("\n<!-- Database Administration -->\n");
  printf("<table border=0 width=%s>\n", ADMIN_WIDTH);
  t_hr(4);
  t_title("Database Administration", 4, TITLE_BGCOLOR4);

  /* Batch-delete */
  printf("\n<!-- Batch-delete -->\n");
  printf("<form method='post' action='%s'>\n", sys->ProgramName);
  printf("  <input type='hidden' name='db' value='%s'>\n", db->db);
  printf("  <input type='hidden' name='mode' value='batchdelete'>\n");
  if(db->password != NULL)printf("  <input type='hidden' name='password' value='%s'>\n", db->password);

  printf("  <tr><th bgcolor='%s'><font size=2 face='arial'>Batch-delete</font></th>\n", ENTRY_BGCOLOR);
  printf("      <td align=right>\n");
  printf("      <font size=2>Low physical number</font>\n");
  printf("      <input type=text size=4 name='dNum1'><br>\n");
  printf("      <font size=2>High physical number</font>\n");
  printf("      <input type=text size=4 name='dNum2'><br>\n");
  printf("      </td>\n");
  printf("      <td align=center>\n");
  printf("      <input type=checkbox name='dSync' value='true' checked> <font size=2>Reorganize database after deletion.</font></td>\n");
  printf("      </td>\n");
  printf("      <td align=right><input type='submit' value='    Delete    ' onClick=\"return confirm('Are you sure? It will take a few minutes. Do not DOUBLE CLICK while proceeding.');\"></td>\n");
  printf("  </tr>\n");
  printf("</form>\n");

  /* DB Reorganization */
  printf("\n<!-- DB Reorganization -->\n");
  printf("<form method='post' action='%s'>\n", sys->ProgramName);
  printf("  <input type='hidden' name='db' value='%s'>\n", db->db);
  printf("  <input type='hidden' name='mode' value='sync'>\n");
  if(db->password != NULL)printf("  <input type='hidden' name='password' value='%s'>\n", db->password);

  printf("  <tr><th bgcolor='%s'><font size=2 face='arial'>DB Reorganization</font></th>", ENTRY_BGCOLOR);
  printf("      <td align=center colspan=2>\n");
  printf("      <font size=2>You don't have to use this, however you can increase performance a little bit.<br>Frequent reorganization is not recommended.</font>\n");
  printf("      </td>\n");
  printf("      <td align=right><input type='submit' value=' Reorganize ' onClick=\"return confirm('Frequent reorganization is not recommended.');\"></td>\n");
  printf("  </tr>\n");
  printf("</form>\n");

  printf("  <tr><td colspan=4><hr size=1 noshade></td></tr>\n");

  printf("</table>\n");

  printf("\n<!-- alert -->\n");
  printf("<script language='JavaScript'>\n");
  if(db->password == NULL) { /* √≥¿Ω ¡¢º” */
    printf("  alert('Welcome my NEW MASTER. This is our first meeting, so please tell me your NAME and your E-MAIL, then I will send new articles to you by email. Please remember I am your forever slave.');\n");
  }
  else { /* ¿Á¡¢º” */
    if(!strcmp(conf->Language, "korean"))
      printf("  alert('%s ¡÷¿Œ¥‘ æÓº≠ø¿Ω Ω√ø‰. π´æ˘¿ª µµøÕ µÂ∏±±Óø‰?');\n", conf->AdminName);
    else if(!strcmp(conf->Language, "japanese"))
      printf("  alert('%s É}ÉXÉ^ÅAÇÊÇ§Ç±ÇªÅIâΩÇÇ®èïÇØÇ‹Ç∑Ç©ÅH?');\n", conf->AdminName);
    else
      printf("  alert('Thank you for your visiting my forever master %s. How about your condition.');\n", conf->AdminName);
  }
  printf("</script>\n");

  tailAdmin();
}


void cwb_adminsave(SysConfig *sys, DB *db, DBConfig *conf){
  int pass;
  char buf[0xFF];

  pass = check_password(sys, db, conf);  

  if(pass == PASS_NOT_DEFINE && !strcmp("", qRemoveSpace(qValue("cPassword"))))qError("You must specify the password, when you configure a db at first time.");

  if(strcmp(qValue("cPassword"), qRemoveSpace(qValue("cPassword2"))))qError("Two passwords must be same.");

  if(!strcmp(qRemoveSpace(qValue("cAdminName")), ""))  qError("The field for the administrator's name must be specify.");

  if(!strcmp(qRemoveSpace(qValue("cAdminEmail")), "")) qError("The field for the administrator's e-mail must be specify.");

  if(qCheckEmail(qRemoveSpace(qValue("cAdminEmail"))) == 0) qError("Administrator's e-mail address is incorrect.");

  sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
  DBOpen(buf, GDBM_WRITER);

  if(strcmp(qValue("cPassword"), "")) DBStore("conf.Password", crypt(qValue("cPassword"), sys->CryptSalt), GDBM_REPLACE);

#define FetchStore(NAME) { DBStore("conf." #NAME, qValue("c" #NAME), GDBM_REPLACE); }

  FetchStore(Title);
  FetchStore(Message);

  FetchStore(HomeUrl);
  FetchStore(HomeTarget);
  FetchStore(BackUrl);
  FetchStore(BackTarget);

  FetchStore(AdminName);
  FetchStore(AdminEmail);
  FetchStore(MailToAdmin);
  FetchStore(MailToSender);

  /* common */
  FetchStore(Language);
  FetchStore(FontSize);
  FetchStore(UnderlinedLink);
  FetchStore(TableWidth);
  FetchStore(CharSet);
  FetchStore(UseSysDefHtml);
  FetchStore(HtmlHead);
  FetchStore(HtmlTail);

  /* list */
  FetchStore(ListOrder);
  FetchStore(ArticlesPerPage);
  FetchStore(FieldNumber);
  FetchStore(FieldFiletype);
  FetchStore(FieldFilename);
  FetchStore(FieldName);
  FetchStore(FieldSubject);
  FetchStore(FieldDate);
  FetchStore(FieldHits);
  FetchStore(FieldDownload);
  FetchStore(FolderIcon);
  FetchStore(HighlightArticle);
  FetchStore(HideSearch);
  FetchStore(ListIconLocation);
  FetchStore(ListBorderSize);
  FetchStore(ListTitleFontColor);
  FetchStore(ListTitleBgColor);
  FetchStore(ListArticleFontColor);
  FetchStore(ListArticleBgColor);

  /* read */
  FetchStore(HtmlAccept);
  FetchStore(LinkTarget);
  FetchStore(DisplayAddress);
  FetchStore(ReadIconLocation);
  FetchStore(ReadLineHeight);
  FetchStore(ReadTitleBorderSize);
  FetchStore(ReadTextBorderSize);
  FetchStore(ReadTitleFontColor);
  FetchStore(ReadTitleBgColor);
  FetchStore(ReadTextFontColor);
  FetchStore(ReadTextBgColor);

  /* write */
  FetchStore(UploadEnable);
  FetchStore(SubjectText);
  FetchStore(DefaultSubject);
  FetchStore(DefaultText);
  FetchStore(UseCategory);
  FetchStore(Category);
  FetchStore(WriteTitleFontColor);
  FetchStore(WriteTitleBgColor);

  /* email */
  FetchStore(UseSysDefMail);
  FetchStore(MailHead);
  FetchStore(MailTail);

  /* permissions */
  FetchStore(ListMode);
  FetchStore(WriteMode);
  FetchStore(ReadMode);
  FetchStore(StatMode);

#undef FetchStore

  DBClose();
}

void t_title(char *title, int colspan, char *bgcolor) {

  printf("\n  <!-- %s -->\n", title);
  printf("  <tr><th bgcolor='%s' colspan='%d'><font size=3 face='arial'>%s</font></th></tr>\n", bgcolor, colspan, title);
}

void t_hr(int colspan) {
  printf("  <tr><td colspan='%d'><hr size=1 noshade></td></tr>\n", colspan);
}

void _t_textarea(char *title, char *key, char *value, int colspan) {
  char *tmpvalue;

  tmpvalue = strdup(value);
  printf("  ");
  printf("<tr><th bgcolor='%s'><font size='2' face='arial'>%s</font></th>", ENTRY_BGCOLOR, title);
  printf("<td colspan='%d'><textarea name='c%s' cols=60 rows=4>", colspan, key); qPuts(1, tmpvalue); printf("</textarea></td></tr>\n");
  free(tmpvalue);
}

void _t_radio(char *title, char *key, char *value, int colspan, char *str) {
  Entry *first, *entries;
  char *checked;
  int checked_count = 0;

  printf("  ");
  printf("<tr><th bgcolor='%s'><font size='2' face='arial'>%s</font></th>", ENTRY_BGCOLOR, title);
  printf("<td colspan='%d'><font size='2' face='arial'>", colspan);

  for(first = entries = qsDecoder(str); entries; entries = entries->next){
    if(!strcmp(entries->name, value)) checked = " checked", checked_count++; else checked = "";
    printf("<input type='radio' name='c%s' value='%s'%s> %s", key, entries->name, checked, entries->value);
    if(entries->next != NULL) printf(" , ");
  }
  if(first) qsFree(first);

  if(checked_count != 1) printf("<font color='red'>Key is Mismatched</font>");
  printf("</font></td></tr>\n");
}

void _t_input(char *title, char *key, char *value, int colspan, int length, int maxlength, char *msg) {
  printf("  ");
  printf("<tr><th bgcolor='%s'><font size='2' face='arial'>%s</font></th>", ENTRY_BGCOLOR, title);
  printf("<td colspan='%d'><input type='text' size='%d' maxlength='%d' name='c%s' value=\"", colspan, length, maxlength, key); qPrintf(1, "%s", value); printf("\">");
  if(msg != NULL) printf(" <font size='2' face='arial'><i>%s</i></font>", msg);
  printf("</td></tr>\n");
}

void _t_input2(char *title, char *key, char *value, int colspan, int length, int maxlength, char *title2, char *key2, char *value2, int length2, int maxlength2) {
  printf("  ");
  printf("<tr><th bgcolor='%s'><font size='2' face='arial'>%s</font></th>", ENTRY_BGCOLOR, title);
  printf("<td colspan='%d'><input type='text' size='%d' maxlength='%d' name='c%s' value=\"", colspan, length, maxlength, key); qPrintf(1, "%s", value); printf("\">");
  printf("<font size='2' face='arial'><b>%s</b></font> <input type='text' size='%d' maxlength='%d' name='c%s' value=\"", title2, length2, maxlength2, key2); qPrintf(1, "%s", value2); printf("\">");
  printf("</td></tr>\n");
}

void _t_input22(char *title, char *key, char *value, int length, int maxlength, char *title2, char *key2, char *value2, int length2, int maxlength2) {
  printf("  ");
  printf("<tr><th bgcolor='%s'><font size='2' face='arial'>%s</font></th>", ENTRY_BGCOLOR, title);
  printf("<td><input type='text' size='%d' maxlength='%d' name='c%s' value=\"", length, maxlength, key); qPrintf(1, "%s", value); printf("\"></td>");
  printf("<th><font size='2' face='arial'>%s</font></th>", title2);
  printf("<td><input type='text' size='%d' maxlength='%d' name='c%s' value=\"", length2, maxlength2, key2); qPrintf(1, "%s", value2); printf("\"></td></tr>\n");
}
