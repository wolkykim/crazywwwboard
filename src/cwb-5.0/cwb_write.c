#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "cwb.h"
#include "cwb_admin.h"
#include "cwb_write.h"

typedef struct Reply {
  char *Name;
  char *Email;
  char *Subject;
  char *Text;
  char *Category;
  char *Password;
}Reply;

char *replySubject(char *header, char* orgsubj);
char *replyText(char *orgtext, char *name, char *retext, char *reline);
void write_form(SysConfig *sys, DB *db, DBConfig *conf, Reply *reply);
char *joinText(char *txt1, char *txt2);
int  getlines(char *string);
int  uploadSum(char *dbpath, int lastnum);
int  sendmail(SysConfig *sys, char *To, DBArticle *article, DB *db, DBConfig *conf);

/* write & reply */
void cwb_write(SysConfig *sys, DB *db, DBConfig *conf){
  Reply reply;

  /* ø¿∑˘ ∞ÀªÁ */
  if(!strcmp(db->mode, "write") && db->num != 0) qError("Do not use manually.");
  if(!strcmp(db->mode, "reply") && db->num == 0) qError("Do not use manually.");

  /* ∞¸∏Æ¿⁄∏∏ ∞‘Ω√π∞¿ª ¿€º∫«“ºˆ ¿÷¥Ÿ∏È, æœ»£ ∞ÀªÁ */
  if(!strcmp(conf->WriteMode, "admin")) check_password(sys, db, conf);

  /* ƒÌ≈∞ø°º≠ ¿Ã∏ß∞˙ E-mail¿ª ∞°¡Æø» */
  if(!strcmp(sys->CookieEnable, "true")) {
    if(qcValue("NAME")) reply.Name  = strdup(qcValue("NAME"));
    else reply.Name  = strdup("");
    if(qcValue("EMAIL")) reply.Email = strdup(qcValue("EMAIL"));
    else reply.Email = strdup("");
  }
  else { /* ƒÌ≈∞∞° disable µ«æ˙¿ªΩ√ ±‚¡∏¿« ƒÌ≈∞¡§∫∏µµ π´Ω√«—¥Ÿ */
    reply.Name  = strdup("");
    reply.Email = strdup("");
  }

  if(db->num != 0) {  /* ∞¸∑√±€ æ≤±‚ */
    char buf[0xff], *rName, *rSubject, *rText;

    sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
    DBOpen(buf, GDBM_READER);

    if((rName    = DBFetch ("%d.Name",    db->num)) == NULL) qError("Original article(%d) not found.", db->num);
    if((rSubject = DBFetch ("%d.Subject", db->num)) == NULL) rSubject = strdup("");
    if((rText    = DBFetch ("%d.Text",    db->num)) == NULL) rText = strdup("");

    reply.Subject   = replySubject(sys->ReSubject, rSubject);
    reply.Text      = replyText(rText, rName, sys->ReText, sys->ReLine);
    if((reply.Category  = DBFetch("%d.Category", db->num)) == NULL) reply.Category = strdup("");
    reply.Password  = strdup("");

    free(rName), free(rSubject), free(rText);

    DBClose();
  }
  else { /* ±◊≥… æ≤±‚ */
    reply.Subject  = strdup(conf->DefaultSubject);
    reply.Text     = strdup(conf->DefaultText);
    reply.Category = strdup(db->category);
    reply.Password = strdup("");
  }

  write_form(sys, db, conf, &reply);

  free(reply.Name),
  free(reply.Email);
  free(reply.Subject);
  free(reply.Text);
  free(reply.Category);
  free(reply.Password);
}

char *replySubject(char *header, char* orgsubj) {
  char *subject;

  /* Re: ∞° æ»µ» ¡¶∏Òø° Re:∏¶ √ﬂ∞° «—¥Ÿ */
  if(strncmp(orgsubj, header, strlen(header))) {
    subject = (char *)malloc(sizeof(char) * (strlen(header) + 1 + strlen(orgsubj) + 1));
    sprintf(subject, "%s %s", header, orgsubj);
  }
  else subject = strdup(orgsubj);

  return subject;
}

#define DUMMY_LINES	(10)
char *replyText(char *orgtext, char *name, char *retext, char *reline) {
  char *text;
  char header[0xff], *reline2 = " ", *tailer = "\n";

  char *buf, *offset;
  int  eos, textsize;

  sprintf(header, "%s %s\n", name, retext);

  textsize =  strlen(header);
  textsize += strlen(orgtext);
  textsize += (strlen(reline) + strlen(reline2)) * (getlines(orgtext) + DUMMY_LINES);
  textsize += strlen(tailer);
  textsize += 1; /* '\0' */

  if((text = (char *)malloc(textsize)) == NULL) qError("Memory allocation fail.");

  strcpy(text, header);

  for(buf = offset = orgtext, eos = 0; eos == 0; ) {
    for(buf = offset; *offset != '\n' && *offset != '\0'; offset++);
    if(*offset == '\0') eos = 1;
    else *offset = '\0', offset++;

    strcat(text, (const char *)reline);
    strcat(text, (const char *)reline2);
    strcat(text, (const char *)buf);
    strcat(text, "\n");
  }
  strcat(text, tailer);

  /* For debugging....
  qContentType("text/html");
  printf("<!-- Malloc Size : %d bytes , StrSize : %d bytes -->\n", textsize, strlen(text));
  */

  return text;
}

void cwb_modify(SysConfig *sys, DB *db, DBConfig *conf){
  Reply reply;
  char buf[0xff];

  if(db->num == 0) qError("Do not use manually.");

  check_password(sys, db, conf);

  sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
  DBOpen(buf, GDBM_READER);

  if((reply.Name     = DBFetch("%d.Name",     db->num)) == NULL) qError("Original article(%d) not found.", db->num);
  if((reply.Email    = DBFetch("%d.Email",    db->num)) == NULL) reply.Email    = strdup("");
  if((reply.Subject  = DBFetch("%d.Subject",  db->num)) == NULL) reply.Subject  = strdup("");
  if((reply.Text     = DBFetch("%d.Text",     db->num)) == NULL) reply.Text     = strdup("");
  if((reply.Category = DBFetch("%d.Category", db->num)) == NULL) reply.Category = strdup("");
  reply.Password = strdup("");

  DBClose();

  write_form(sys, db, conf, &reply);

  free(reply.Name);
  free(reply.Email);
  free(reply.Subject);
  free(reply.Text);
  free(reply.Category);
  free(reply.Password);
}

void write_form(SysConfig *sys, DB *db, DBConfig *conf, Reply *reply){
  char *Mode;
  char *Name, *Email, *Category, *Password, *Subject, *AttachText, *AttachFile, *Sign;
  int  uploadOverflow = FALSE, uploadSize = 0;
  Entry *catep;
  int  categoryEnable;

  /* ¿˙¿Â «¸Ωƒ ¡ˆ¡§ */
  if(!strcmp(db->mode, "modify")) Mode = "modifysave";
  else                            Mode = "writesave";     /* write or reply */

  /* ææÓ ∫Ø»Ø */
  if(!strcmp(conf->Language, "korean")) {
    Name         = "<font face='±º∏≤'>º∫ ∏Ì</font>";
    Email        = "<font face='±º∏≤'>¿¸¿⁄øÏ∆Ì</font>";
    Category     = "<font face='±º∏≤'>-∫–∑˘º±≈√-</font>";
    Subject      = "<font face='±º∏≤'>¡¶ ∏Ò</font>";
    AttachText   = "<font face='±º∏≤'>πÆº≠√∑∫Œ</font>";
    AttachFile   = "<font face='±º∏≤'>∆ƒ¿œ¿¸º€</font>";
    if(!strcmp(db->mode, "modify")) {
      Sign     = "                   ºˆ  ¡§                   ";
      Password = "<font face='±º∏≤'>ªı∫Òπ–π¯»£</font>";
    }
    else {
      Sign     = "                   ¿˙  ¿Â                   ";
      Password = "<font face='±º∏≤'>∫Òπ–π¯»£</font>";
    }
  }
  else if(!strcmp(conf->Language, "japanese")) {
    Name         = "ÉlÅ[ÉÄ";
    Email        = "ÉÅÅ[Éã";
    Category     = "-Category-";
    Subject      = "ëËÅ@ñ⁄";
    AttachText   = "ï∂èëéÊÇËçûÇ›";
    AttachFile   = "ÉtÉ@ÉCÉãìYït";
    if(!strcmp(db->mode, "modify")) {
      Sign     = "                   èCÅ@ê≥                  ";
      Password = "êVÉpÉXÉèÅ[Éh";
    }
    else {
      Sign     = "                   ìäÅ@çe                  ";
      Password = "ÉpÉXÉèÅ[Éh";
    }
  }
  else {
    Name         = "<font face='Arial'>Name</font>";
    Email        = "<font face='Arial'>E-Mail</font>";
    Category     = "-Category-";
    Subject      = "<font face='Arial'>Subject</font>";
    AttachText   = "<font face='Arial'>Attach Text</font>";
    AttachFile   = "<font face='Arial'>Upload Binary</font>";
    if(!strcmp(db->mode, "modify")) {
      Sign     = "                M o d i f y                ";
      Password = "<font face='Arial'>New Passwd</font>";
    }
    else {
      Sign     = "                S u b m i t                ";
      Password = "<font face='Arial'>Password</font>";
    }
  }

  /* Subject Text∞° ¿÷¿∏∏È ±◊∞Õ¿ª ªÁøÎ«—¥Ÿ */
  if(strcmp(conf->SubjectText, "")) Subject = conf->SubjectText;

  /* æ˜∑Œµ˘ ¡¶«— ∞ÀªÁ */
  if(atoi(sys->UploadSoftLimit) > 0 || atoi(sys->UploadHardLimit) > 0){
    char dbpath[0xff];

    sprintf(dbpath, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
    uploadSize = uploadSum(dbpath, conf->LastNum) / 1024;

    /* Soft æ˜∑Œµ˘ ¡¶«— √ ∞˙ */
    if(atoi(sys->UploadSoftLimit) < uploadSize) uploadOverflow = TRUE;

    /* Hard æ˜∑Œµ˘ ¡¶«— √ ∞˙ */
    if(atoi(sys->UploadHardLimit) < uploadSize) {
      headHtml(sys, conf);
      printf("<p>\n");
      printf("<font size=2 face='Arial' color='gray'><b>\n");
      printf("  <font size=4 color=red>Warning : Downsizing under %s Kbytes</font><p>\n", sys->UploadHardLimit);
      printf("  This database(%d Kbytes used) is allowed %d Kbytes for maximum uploading space.<br>\n", uploadSize, atoi(sys->UploadHardLimit));
      printf("  Posting will be locked until downsizing under %d Kbytes.<p>\n", atoi(sys->UploadHardLimit));
      printf("  <a href='javascript:history.back()' onMouseOver=\"window.status=('Go back and remove some articles'); return true;\"><img src='%s/%s' border=0 alt='Back'></a>\n", sys->HtmlBasePath, sys->IconBack);
      printf("</b></font>\n");
      printf("<p>\n");
      tailHtml(sys, conf);
      return;
    }
  }

  /* ƒ´≈◊∞Ì∏Æ ªÁøÎø©∫Œ ∆«∫∞ */
  catep = qsDecoder(conf->Category);
  if(!strcmp(conf->UseCategory, "true") && catep != NULL) categoryEnable = TRUE;
  else categoryEnable = FALSE;

  headHtml(sys, conf);

  printf("\n<!-- Form -->\n");
  printf("<form method='post' action='%s' enctype='multipart/form-data'>\n", sys->ProgramName);

  printf("  <input type=hidden name='db'       value='%s'>\n", db->db);
  printf("  <input type=hidden name='mode'     value='%s'>\n", Mode);
  printf("  <input type=hidden name='num'      value='%d'>\n", db->num);
  printf("  <input type=hidden name='page'     value='%d'>\n", db->page);
  printf("  <input type=hidden name='category' value='%s'>\n", db->category);
  printf("  <input type=hidden name='ftype'    value='%d'>\n", db->ftype);
  printf("  <input type=hidden name='fval'     value='%s'>\n", db->fval);
  if(db->password != NULL) printf("  <input type=hidden name='password' value='%s'>\n", db->password);


  printf("\n<!-- Table -->\n");
  printf("<table border=0>\n");

  printf("\n  <!-- HR -->\n");
  printf("  <tr><td colspan=3><hr size=1 noshade></td></tr>\n");

  /* Name */
  printf("\n  <!-- Name field -->\n");
  printf("  <tr><th bgcolor='%s'><font size='%s' color='%s'>%s</font></th>\n", conf->WriteTitleBgColor, conf->FontSize, conf->WriteTitleFontColor, Name);
  printf("      <td><font size=3><input type='text' name='pName' size=30 maxlength=20 value=\""); qPrintf(1, "%s", reply->Name); printf("\"></font></td>\n");
  if(categoryEnable == FALSE) printf("      <td><br></td>\n");
  else                        printf("      <th align=left><font size='%s'>%s</font></th>\n", conf->FontSize, Password);
  printf("  </tr>\n");

  /* E-mail */
  printf("\n  <!-- Email field -->\n");
  printf("  <tr><th bgcolor='%s'><font size='%s' color='%s'>%s</font></th>\n", conf->WriteTitleBgColor, conf->FontSize, conf->WriteTitleFontColor, Email);
  printf("      <td><font size=3><input type='text' name='pEmail' size=30 maxlength=40 value=\""); qPrintf(1, "%s", reply->Email); printf("\"></font></td>\n");
  if(categoryEnable == FALSE) printf("      <th align=left><font size='%s'>%s</font></th>\n", conf->FontSize, Password);
  else                        printf("      <th align=left><font size=3><input type='password' name='pPassword' size=8  maxlength=%d  value=\"%s\"></font></th>\n", MAX_PASSWORD_LEN, reply->Password);
  printf("  </tr>\n");

  /* Subject */
  printf("\n  <!-- Subject field -->\n");
  printf("  <tr><th bgcolor='%s'><font size='%s' color='%s'>%s</font></th>\n", conf->WriteTitleBgColor, conf->FontSize, conf->WriteTitleFontColor, Subject);
  printf("      <td><font size=3><input type='text' name='pSubject' size=40 maxlength=50 value=\""); qPrintf(1, "%s", reply->Subject); printf("\"></font></td>\n");
  if(categoryEnable == FALSE) { /* ∫Òπ–π¯»£ ¿‘∑¬ √¢ */
    printf("      <th align=left><font size=3><input type='password' name='pPassword' size=8  maxlength=%d  value=\"%s\"></font></th>\n", MAX_PASSWORD_LEN, reply->Password);
  }
  else { /* ƒ´≈◊∞Ì∏Æ∞° ¿÷¥Ÿ, ƒ´≈◊∞Ì∏Æ √¢ */
    Entry *entries;
    char *title, *tmp;

    if((title = qsValue(catep, "_TITLE")) != NULL) Category = title;

    printf("      <td align=left><font size=2>\n");
    printf("      <select name='pCategory'>\n");
    printf("      <option value=''>%s\n", Category);
    for(entries = catep; entries; entries = entries->next){
      if(entries->name[0] == '_') continue;
      if(!strcmp(reply->Category, entries->name)) tmp = "selected";
      else tmp = "";

      printf("      <option value='%s' %s>%s\n", entries->name, tmp, entries->value);
    }
    printf("      </select>\n");
    printf("      </font></td>\n");
    qsFree(catep);
  }
  printf("  </tr>\n");

  /* Text File */
  printf("\n  <!-- Text field -->\n");
  printf("  <tr>\n");
  printf("    <td colspan=3 align=left><font size=3>\n");
  printf("<textarea name='pText' cols='%s' rows='15' wrap='virtual'>", sys->TextFieldLength);
  qPuts(1, reply->Text);
  printf("</textarea>\n");
  printf("    </font></td>\n");
  printf("  </tr>\n");

  /* Attach Text File */
  printf("\n  <!-- Attachment field -->\n");
  printf("  <tr><th bgcolor='%s'><font size='%s' color='%s'>%s</font></th>\n", conf->WriteTitleBgColor, conf->FontSize, conf->WriteTitleFontColor, AttachText);
  printf("      <td colspan=2><font size=3><input type='file' name='pAttachText' size=46></font></td>\n");
  printf("  </tr>\n");

  /* Attach Binary File : Ω√Ω∫≈€¿Ã æ˜∑Œµ˘¿ª «„øÎ«œ∞Ì, mode∞° modify∞° æ∆¥œ∏Á, DB∞° æ˜∑Œµ˘ º≥¡§¿Ã µ«æÓ¿÷∞Ì, ø¿πˆ«√∑ŒøÏ∞° æ¯¿ª∂ß */
  if(!strcmp(conf->UploadEnable, "true") && strcmp(db->mode, "modify") && !strcmp(sys->AllowUpload, "true") && uploadOverflow == FALSE) {
    printf("\n  <!-- Binary uploading field -->\n");
    printf("  <tr><th bgcolor='%s'><font size='%s' color='%s'>%s</font></th>\n", conf->WriteTitleBgColor, conf->FontSize, conf->WriteTitleFontColor, AttachFile);
    printf("      <td colspan=2><font size=3><input type='file' name='pAttachFile' size=46></font></td>\n");
    printf("  </tr>\n");
  }

  if(uploadOverflow == TRUE) {
    printf("  <tr><td colspan=3><font color=red size='%s' face='Arial'><b>\n", conf->FontSize);
    printf("      This database(%d Kbytes used) is allowed %d Kbytes for uploading space.<br>\n", uploadSize, atoi(sys->UploadSoftLimit));
    printf("      Uploading will be locked until downsizing under %d Kbytes.\n", atoi(sys->UploadSoftLimit));
    printf("      </b></font></td>\n");
    printf("  </tr>\n");
  }

  printf("\n  <!-- HR -->\n");
  printf("  <tr><td colspan=3><hr size=1 noshade></td></tr>\n");

  printf("\n  <!-- Icon List Link -->\n");
  printf("  <tr>\n");
  printf("    <td><br></td>\n");
  printf("    <td align=center><font size=2><input type=submit value='%s'></font></td>\n", Sign);
  printf("    <td align=right>\n");

  if(db->backdepth == 0) { /* ªÁøÎ¿⁄∞° ºˆµø¿˚¿∏∑Œ µÈæÓø‘¥Ÿ */
    printf("    <a href='%s?db=%s' onMouseOver=\"window.status=('Back to the list'); return true;\"><img src='%s/%s' border=0 alt='List'></a>\n", sys->ProgramName, db->db, sys->HtmlBasePath, sys->IconList);
  }
  else  printf("    <a href='javascript:history.go(-%d)' onMouseOver=\"window.status=('Back to the list'); return true;\"><img src='%s/%s' border=0 alt='List'></a>\n", db->backdepth, sys->HtmlBasePath, sys->IconList);

  printf("    </td>\n");
  printf("  </tr>\n");

  printf("</form>\n");
  printf("</table>\n");

  tailHtml(sys, conf);
}

void cwb_writesave(SysConfig *sys, DB *db, DBConfig *conf){
  DBArticle article;
  Cgienv env;

  char buf[0xff], date[14+1];
  char *attachText, *o_poster;
  int  dataExist = FALSE;

  if(!strcmp(conf->WriteMode, "admin")) check_password(sys, db, conf);

  /* Fetch Query */
  qCgienv(&env);
  if((article.Name     = qRemoveSpace(qValue("pName"))) == NULL)    qError("Only reserved query can be accepted.");
  if((article.Email    = qRemoveSpace(qValue("pEmail"))) == NULL)   qError("Only reserved query can be accepted.");
  if((article.Subject  = qRemoveSpace(qValue("pSubject"))) == NULL) qError("Only reserved query can be accepted.");
  if((article.Password = qValue("pPassword")) == NULL)              article.Password = "";
  if((article.Category = qValue("pCategory")) == NULL)              article.Category = "";
  if((article.Text     = qValue("pText")) == NULL)                  qError("Only reserved query can be accepted.");

  if((attachText       = qValue("pAttachText")) == NULL)          attachText = "";
  if((article.Filename = qValue("pAttachFile.filename")) == NULL) article.Filename = "";
  if((article.Filesize = qValue("pAttachFile.length")) == NULL)   article.Filesize = "";

  sprintf(date, "%04d%02d%02d%02d%02d%02d", env.year, env.mon, env.day, env.hour, env.min, env.sec);
  article.Date   = date;
  if(!strcmp(conf->DisplayAddress, "true")) article.Domain = env.remote_host;
  else article.Domain = "";
  article.Lines    = getlines(article.Text);
  article.Access   = 0;
  article.Thread   = db->num;

  /* Data ∞ÀªÁ */
  if(!strcmp(article.Name,     ""))     qError("The name field must be specified.");
  if(strcmp(article.Email,     "") != 0) {
    if(qCheckEmail(article.Email) == 0) qError("The E-mail address(%s) is incorrect.", article.Email);
  }
  if(!strcmp(article.Subject,  ""))     qError("The subject field must be specified.");
  if(strcmp(article.Password,  "") != 0) article.Password = crypt(article.Password, sys->CryptSalt);

  if(strlen(article.Text) > 0) dataExist = TRUE;

  /* Attach Text */
  if(strcmp(attachText, "") != 0) {
    article.Text  = joinText(article.Text, attachText);
    article.Lines = getlines(article.Text);
    if(strlen(article.Text) > 0) dataExist = TRUE;
  }

  /* Attach File */
  if(strcmp(article.Filename, "") != 0) {
    int fileLength;

    if((fileLength = atoi(article.Filesize)) > 0) { /* upload∞° µ«æ˙¥Ÿ */
      FILE *fp;
      char *binaryp, binaryName[0xff], binaryPath[0xff];
      int  i;

      dataExist = TRUE;

      /* Hard æ˜∑Œµ˘ ¡¶«— ∞ÀªÁ */
      if(atoi(sys->UploadHardLimit) > 0) {
        char dbpath[0xff];
        int uploadSize, usedSpace, requestedSpace;

        sprintf(dbpath, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
        usedSpace = uploadSum(dbpath, conf->LastNum) / 1024;
        requestedSpace = fileLength / 1024;
        uploadSize = usedSpace + requestedSpace;
        if(atoi(sys->UploadHardLimit) < uploadSize) {
          qError(" Sorry, this database is allowed %d Kbytes for maximum uploading space. Your operation is canceled. Used space(%d Kbytes) + Your request(%d Kbytes) > Allowed space(%d Kbytes)", atoi(sys->UploadHardLimit), usedSpace, requestedSpace, atoi(sys->UploadHardLimit));
        }
      }

      /* filename¡ﬂ ' '¿Ã ¿÷¿∏∏È '_'∑Œ πŸ≤€¥Ÿ. */
      for(i = 0; article.Filename[i] != '\0'; i++) {
        if(article.Filename[i] == ' ') article.Filename[i] = '_';
      }

      /* µ∑∫≈‰∏Æ ¡∏¿Á ∞ÀªÁ */
      sprintf(binaryPath, "%s/%s/%s", sys->RealBasePath, sys->DataDirectory, db->db);
      if(qCheckFile(binaryPath) == 0) { /* æ˜∑ŒµÂøÎ µ∑∫≈‰∏Æ ª˝º∫ */
        if(mkdir(binaryPath, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
          qError("Upload directory(%s) creation fail. Please check the permissions.", binaryPath);
        }
      }

      /* µø¿œ∆ƒ¿œ ∞ÀªÁ */
      sprintf(binaryPath, "%s/%s/%s/%s", sys->RealBasePath, sys->DataDirectory, db->db, article.Filename);
      if(qCheckFile(binaryPath) == 1) { /* µø¿œ∆ƒ¿œ¿Ã ¡∏¿Á«œ∏È, √ ±‚ sequencial number = 2 */
        for(i = 1; ; i++) {
          sprintf(binaryPath, "%s/%s/%s/%d-%s", sys->RealBasePath, sys->DataDirectory, db->db, i, article.Filename);
          if(qCheckFile(binaryPath) == 0) { /* µø¿œ∆ƒ¿œ æ¯¿Ω */
            /* ªı∑ŒøÓ ∆ƒ¿œ∏Ì¿ª ∏∏µÎ */
            sprintf(binaryName, "%d-%s", i, article.Filename);
            article.Filename = binaryName;
            break;
          }
        }
      }

      /* ¿˙¿Â */
      binaryp = qValue("pAttachFile");
      fp = fopen(binaryPath, "wb");
      if(fp == NULL) qError("File creation(%s) fail.", binaryPath);
      for(i = 0; i < fileLength; i++) fprintf(fp, "%c", binaryp[i]);
      fclose(fp);
    }
  }

  if(dataExist == FALSE) qError("There are no datas to post.");

  /* Insert into DB */
  sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
  DBOpen(buf, GDBM_WRITER);

  /* DB¿« ¥‹¿œ¡¢±Ÿ¿Ã ∫∏¿Âµ«æ˙¥Ÿ, ∏∂¡ˆ∏∑ ∞‘Ω√π∞ π¯»£∏¶ ¥ŸΩ√ ∞°¡Æø¬¥Ÿ */
  conf->LastNum = DBiFetch("conf.LastNum");
  article.Num = ++conf->LastNum;

#define StoreStr(NAME) { sprintf(buf, "%d." #NAME, article.Num); DBStore(buf, article.NAME, GDBM_REPLACE); }
#define StoreInt(NAME) { sprintf(buf, "%d." #NAME, article.Num); DBiStore(buf, article.NAME, GDBM_REPLACE); }

  StoreStr(Name);
  StoreStr(Email);
  StoreStr(Subject);
  StoreStr(Text);
  if(strcmp(article.Filename, "")) StoreStr(Filename);
  if(strcmp(article.Filesize, "")) StoreStr(Filesize);
  StoreStr(Date);
  StoreStr(Domain);
  StoreInt(Lines);
  StoreInt(Access);
  StoreInt(Thread);
  StoreStr(Password);
  if(strcmp(article.Category, "") != 0) StoreStr(Category);
  DBiStore("conf.LastNum", conf->LastNum, GDBM_REPLACE);

#undef StoreStr
#undef StoreInt

  /* ∞¸∑√±€¿œ ∞ÊøÏ ø¿∏Æ¡ˆ≥Œ±€¿« ¿€º∫¿⁄ E-mail¿ª ∞°¡Æ ø¬¥Ÿ */
  if((o_poster = DBFetch("%d.Email", article.Thread)) == NULL) o_poster = strdup("");

  DBClose();

  /* ∆Ì¡ˆ ∫∏≥ª±‚ */
  if(!strcmp(conf->MailToAdmin, "true")) {
    sendmail(sys, conf->AdminEmail, &article, db, conf);
  }

  if(!strcmp(conf->MailToSender, "true")) {
    /* ∞¸∏Æ¿⁄ø°∞‘ ∆Ì¡ˆ∏¶ ∫∏≥¬∞Ì, o_posterøÕ ∞¸∏Æ¿⁄∞° ∞∞¿∏∏È ∆Ì¡ˆ∏¶ ∫∏≥ª¡ˆ æ ¥¬¥Ÿ */
    if(!(!strcmp(conf->MailToAdmin, "true") && !strcmp(conf->AdminEmail, o_poster))) {
      sendmail(sys, o_poster, &article, db, conf);
    }
  }
  free(o_poster);
  free(article.Text); /* joinText∑Œ mallocµ» ¿⁄∑· free */
}

void cwb_modifysave(SysConfig *sys, DB *db, DBConfig *conf){
  DBArticle article;
  char *attachText, buf[0xff];
  int  pass;
  int  dataExist = FALSE;

  if(db->num == 0) qError("Do not use manually.");

  pass = check_password(sys, db, conf);
  if(!(pass == PASS_ADMIN || pass == PASS_POSTER)) qError("Only administrator(%s) or Poster can modify.", conf->AdminName);

  /* Fetch Query */
  if((article.Name     = qRemoveSpace(qValue("pName"))) == NULL)    qError("Only reserved query can be accepted.");
  if((article.Email    = qRemoveSpace(qValue("pEmail"))) == NULL)   qError("Only reserved query can be accepted.");
  if((article.Subject  = qRemoveSpace(qValue("pSubject"))) == NULL) qError("Only reserved query can be accepted.");
  if((article.Text     = qValue("pText")) == NULL)                  qError("Only reserved query can be accepted.");
  if((article.Password = qValue("pPassword")) == NULL)              article.Password = "";
  if((article.Category = qValue("pCategory")) == NULL)              article.Category = "";
  if((attachText       = qValue("pAttachText")) == NULL)            attachText = "";
  article.Lines = getlines(article.Text);

  /* Data ∞ÀªÁ */
  if(!strcmp(article.Name,     ""))     qError("The name field must be specified.");
  if(strcmp(article.Email,     "") != 0) {
    if(qCheckEmail(article.Email) == 0) qError("The E-mail(%s) address is incorrect.", article.Email);
  }
  if(!strcmp(article.Subject,  ""))     qError("The subject field must be specified.");

  if(strlen(article.Text) > 0) dataExist = TRUE;

  /* Attach Text */
  if(strcmp(attachText, "") != 0) {
    article.Text  = joinText(article.Text, attachText);
    article.Lines = getlines(article.Text);
    if(strlen(article.Text) > 0) dataExist = TRUE;
  }

  if(dataExist == FALSE) qError("There is no data to post.");

  /* Modify DB */
  sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
  DBOpen(buf, GDBM_WRITER);

  /* ∞‘Ω√π∞¿Ã ¡∏¿Á«œ¥¬¡ˆ »Æ¿Œ«—¥Ÿ */
  if(!DBIsKeyExist("%d.Name", db->num)) qError("There is no article to modify in the database.");

#define StoreStr2(NAME) { sprintf(buf, "%d." #NAME, db->num); DBStore(buf, article.NAME, GDBM_REPLACE); }
#define StoreInt2(NAME) { sprintf(buf, "%d." #NAME, db->num); DBiStore(buf, article.NAME, GDBM_REPLACE); }

  StoreStr2(Name);
  StoreStr2(Email);
  StoreStr2(Subject);
  StoreStr2(Text);
  StoreInt2(Lines);

  if(strcmp(article.Password, "") != 0) {
    article.Password = crypt(article.Password, sys->CryptSalt);
    StoreStr2(Password);
  }
  if(strcmp(article.Category, "") != 0) StoreStr2(Category);

#undef StoreStr2
#undef StoreInt2

  DBClose();

  free(article.Text);
}

char *joinText(char *txt1, char *txt2) {
  char *joinText;
  int  joinTextLen, txt1Len, txt2Len;
  int  i, j;

  /* Here, we do not use realloc(). Because sometimes it is unstable. */
  txt1Len = strlen(txt1);
  txt2Len = strlen(txt2);

  joinTextLen = txt1Len + txt2Len + strlen("\n");
  joinText = (char *)malloc(sizeof(char) * (joinTextLen + 1));
  if(joinText == NULL) qError("Memory allocation fail.");

  for(i = 0; i < txt1Len; i++) joinText[i] = txt1[i];
  if(i > 0) joinText[i++] = '\n';
  for(j = 0; j < txt2Len; j++, i++) joinText[i] = txt2[j];
  joinText[i]   = '\0';

  return joinText;
}

int getlines(char *string){
  int i = 1, j = 0;

  if(!strcmp(string, "")) return 0;

  while(string[j]){
    if(string[j] == '\n')i++;
    j++;
  }
  return i;
}

/* DBø° æ˜∑Œµ˘µ» ∆ƒ¿œ¿« bytes∏¶ ∫π±Õ«—¥Ÿ */
int uploadSum(char *dbpath, int lastnum) {
  int sum, i;
  DBOpen(dbpath, GDBM_READER);
  for(sum = 0, i = 1; i <= lastnum; i++) {
    sum += DBiFetch("%d.Filesize", i);
  }
  DBClose();

  return sum;
}

int sendmail(SysConfig *sys, char *To, DBArticle *article, DB *db, DBConfig *conf) {
  Cgienv env;
  FILE *fp;
  char smail[0xFF], server_domain[0xFF];

  if(To == NULL)      return FALSE;
  if(!strcmp(To, "")) return FALSE;

  qCgienv(&env);

  if(!strcmp(env.server_port, "80")) sprintf(server_domain, "%s", env.server_name);
  else sprintf(server_domain, "%s:%s", env.server_name, env.server_port);

  /* Make sendmail path */
  sprintf(smail, "%s %s", sys->Sendmail, sys->SendmailOptions);

  /* ªÁøÎ¿⁄∞° AdminEmail¿ª ¿ﬂ∏¯µ» Local Email∑Œ ¡ˆ¡§«œø¥¿ª ∞ÊøÏø° 
     "No recipient addresses found." øÕ ∞∞¿∫ ø¿∑˘ ∏ﬁΩ√¡ˆ∏¶ sendmail¿∫ 
     stream out «—¥Ÿ. µ˚∂Ûº≠ ¿Ã∂ß internal server error∞° πﬂª˝«“ºˆ ¿÷¥Ÿ. */
  fp = popen((const char *)smail, "w");
  if(fp == NULL) qError("Fatal: Sendmail(%s) not found.", smail);

  /* ¿œ∫ªæÓ¿« ∞ÊøÏ ¡¶∏Ò¿Ã ±˙æÓ¡ˆ±‚ø° øµπÆ πÆ¿⁄ø≠∑Œ ¡¶∏Ò¿ª ∞Ì¡§«—¥Ÿ */
  if(!strcmp(conf->Language, "japanese")) {
    fprintf(fp, "Subject: New article is posted\n");
  }
  else {
    fprintf(fp, "Subject: %s\n", article->Subject);
  }

  fprintf(fp, "From: \"%s (%s:%d)\" <%s>\n",   sys->MailName, db->db, article->Num, conf->AdminEmail);
  fprintf(fp, "Reply-To: \"%s\" <%s>\n", exchangeQuot(article->Name), article->Email);
  fprintf(fp, "To: %s\n",              To);
  fprintf(fp, "Organization: %s\n",    sys->MailOrganization);
  fprintf(fp, "X-Mailer: %s %s (%s)\n", OFFICIAL_PROGRAM_NAME, OFFICIAL_PROGRAM_VERSION, PLATFORM);
  fprintf(fp, "\n");

  /* HEADER */
  if(!strcmp(conf->UseSysDefMail, "true") || !strcmp(sys->AllowDBmail, "false")) {
    FILE *dp;
    char buf[0xff];

    sprintf(buf, "%s/%s", sys->RealBasePath, sys->MailHead);
    dp = fopen(buf, "rt");
    if(dp != NULL) {
      int c;
      while((c = fgetc(dp)) != EOF) fprintf(fp, "%c", c);
      fclose(dp);
    }
  }
  else {
    fprintf(fp, "%s", conf->MailHead);
  }
  fprintf(fp, "\n");

  /* TEXT*/
  fprintf(fp, "-- [ Article Information ] -----------------------------------------------\n");
  fprintf(fp, " Name.....: %s", article->Name);
  if(strcmp(article->Email, "")) fprintf(fp, " (mailto:%s)", article->Email);
  fprintf(fp, "\n");
  fprintf(fp, " Subject..: %s\n", article->Subject);
  fprintf(fp, " View.....: http://%s%s?db=%s&mode=read&num=%d\n", server_domain, env.script_name, db->db, article->Num);
  fprintf(fp, " Reply....: http://%s%s?db=%s&mode=reply&num=%d\n", server_domain, env.script_name, db->db, article->Num);
  if(strcmp(article->Filename, "")) {
    fprintf(fp, " Attached.: http://%s%s?db=%s&mode=download&num=%d (%s:%dKB)\n", server_domain, env.script_name, db->db, article->Num, article->Filename, atoi(article->Filesize) / 1024);
  }

  /* Ω∫∆Æ∏≤¿Ã ¡ﬂ¥‹µ«¡ˆ æ ±‚¿ß«ÿ, √π∂Û¿Œ¿« µµ∆Æ∏¶ ƒﬁ∏∂∑Œ πŸ≤€¥Ÿ. sendmail¿Ã -t ø…º«¿∏∑Œ
     invocationµ»¥Ÿ∏È ¿Ã∑Ø«— ∫Ø»Ø¿∫ ∫“« ø‰«œ¥Ÿ. «•¡ÿ sendmail∞˙ ¥Ÿ∏• ¿Ø«¸¿« MTA∏¶ ø¨∞· 
     «“∂ßø°¥¬ ∫ª ∫Ø»Ø¿Ã « ø‰«“¡ˆµµ ∏∏•¥Ÿ. */
  /*
  if(strcmp(article->Text, "") != 0) {
    int i;
    for(i = 0; article->Text[i] != '\0'; i++) {
      if(i == 0) {
        if(article->Text[i] == '.' && article->Text[i+1] == '\r') article->Text[i] = ',';
      }
      else {
        if(article->Text[i-1] == '\n' && article->Text[i] == '.' && article->Text[i+1] == '\r') article->Text[i] = ',';
        if(article->Text[i-1] == '\n' && article->Text[i] == '.' && article->Text[i+1] == '\0') article->Text[i] = ',';
      }
    }
  }
  */

  fprintf(fp, "-- [ Contents ] ----------------------------------------------------------\n");
  fprintf(fp, "%s\n", article->Text);
  fprintf(fp, "---[ End ] ---------------------------------------------------------------\n");

  /* TAILER */
  if(!strcmp(conf->UseSysDefMail, "true") || !strcmp(sys->AllowDBmail, "false")) {
    FILE *dp;
    char buf[0xff];

    sprintf(buf, "%s/%s", sys->RealBasePath, sys->MailTail);
    dp = fopen(buf, "rt");
    if(dp != NULL) {
      int c;
      while((c = fgetc(dp)) != EOF) fprintf(fp, "%c", c);
      fclose(dp);
    }
  }
  else {
    fprintf(fp, "%s", conf->MailTail);
  }

  /*
  fprintf(fp, "\n.\n");
  */

  pclose(fp);
  return TRUE;
}
