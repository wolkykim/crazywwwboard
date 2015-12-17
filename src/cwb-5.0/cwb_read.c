#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "cwb.h"
#include "cwb_admin.h"
#include "cwb_list.h"
#include "cwb_read.h"

#define AUTO_MAX_LINES	(20)

void readIconLink(SysConfig *sys, DB *db, DBConfig *conf, UpDnInfo *updn);
int  getArticle(char *dbpath, int num, DBArticle *article);
void freeArticle(DBArticle *article);
int  previewFile(char *filename, int lines, int filter);

void cwb_read(Entry *sysp, SysConfig *sys, DB *db, DBConfig *conf){
  DBArticle article;
  char dbpath[0xFF], status_msg[0xff], buf[0xff];
  int  filter;
  char *Written, *Hits, *Lines, *Download, *Category, *NotSelected, *Prev, *Next;
  UpDnInfo updn;
  Entry *catep;
  int  categoryEnable;
  char *month[12+1] = {"",
                       "January",
                       "February",
                       "March",
                       "April",
                       "May",
                       "Jun",
                       "July",
                       "August",
                       "September",
                       "October",
                       "November",
                       "December"};

  if(db->num == 0) qError("Do not use manually.");

  /* ∞¸∏Æ¿⁄∏∏ ∞‘Ω√π∞¿ª ¿–¿ªºˆ ¿÷¥Ÿ∏È, æœ»£ ∞ÀªÁ */
  if(!strcmp(conf->ReadMode, "admin")) check_password(sys, db, conf);

  if(!strcmp(conf->Language, "korean")) {
    Written     = "<font face='±º∏≤'>¿€º∫¿⁄ : </font>";
    Hits        = "<font face='±º∏≤'>¡∂»∏ºˆ</font>";
    Lines       = "<font face='±º∏≤'>¡Ÿºˆ</font>";
    Download    = "<font face='±º∏≤'>≥ª∑¡πﬁ±‚</font>";
    Category    = "<font face='±º∏≤'>∫–∑˘</font>";
    NotSelected = "<font face='±º∏≤'>±‚≈∏</font>";
    Prev        = "<font face='±º∏≤'>¿Ã¿¸</font>";
    Next        = "<font face='±º∏≤'>¥Ÿ¿Ω</font>";
  }
  else if(!strcmp(conf->Language, "japanese")) {
    Written     = "ìäçeé“ : ";
    Hits        = "ÉqÉbÉg";
    Lines       = "ÉâÉCÉì";
    Download    = "ì]ëó";
    NotSelected = "ÉmÅ[É}Éã";
    Category    = "ÉJÉeÉSÉä";
    Prev        = "ëO";
    Next        = "éü";
  }
  else {
    Written     = "<font face='Arial'>Written by</font>";
    Hits        = "<font face='Arial'>Hits</font>";
    Lines       = "<font face='Arial'>Lines</font>";
    Download    = "<font face='Arial'>Download</font>";
    NotSelected = "<font face='Arial'>Etc.</font>";
    Category    = "<font face='Arial'>Category</font>";
    Prev        = "<font face='Arial'>Prev</font>";
    Next        = "<font face='Arial'>Next</font>";
  }

  /* DB∞° ¿˙¿Âµ» Ω«¡¶ µ∑∫≈‰∏Æ ∞Ê∑Œ∏¶ ¿˙¿Â«—¥Ÿ */
  sprintf(dbpath, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);

  /* ∞‘Ω√π∞¿ª ∞°¡Æø¬¥Ÿ */
  if(getArticle(dbpath, db->num, &article) == FALSE){
    qError("Brocken or Deleted Article : %d", db->num);
  }

  /* «ˆ¿Á ∞‘Ω√π∞π¯»£ø° ¥Î«— ¿ß æ∆∑° ∞‘Ω√π∞ π¯»£∏¶ √£¥¬¥Ÿ. */
  findUPDNnum(dbpath, db, conf, &updn);

  /* « ≈Õ∞™¿ª ∏∏µÁ¥Ÿ */
  if(!strcmp(conf->HtmlAccept, "true")) filter = 10;
  else if(!strcmp(conf->LinkTarget, "")) filter = 12;
  else filter = 13;

  /* ƒ´≈◊∞Ì∏Æ ªÁøÎø©∫Œ ∆«∫∞ */
  catep = qsDecoder(conf->Category);
  if(!strcmp(conf->UseCategory, "true") && catep != NULL) categoryEnable = TRUE;
  else categoryEnable = FALSE;

  /* ∏”∏Æ∏ª ∫∏≥ø */
  headHtml(sys, conf);

    /* ICON Link */
    if(!strcmp(conf->ReadIconLocation, "top") || !strcmp(conf->ReadIconLocation, "both") || (!strcmp(conf->ReadIconLocation, "auto") && article.Lines >= AUTO_MAX_LINES)) {
      readIconLink(sys, db, conf, &updn);
    }

    printf("\n<!-- HR Line -->\n");
    printf("<table border=0 width=%s cellpadding=0 cellspacing=0>\n", conf->TableWidth);
    printf("  <tr><td><hr size=1 noshade></td></tr>\n");
    printf("</table>\n");

    printf("\n<!-- Article Information -->\n");
    printf("<table border=0 width=%s>\n", conf->TableWidth);
    /* Name */
    printf("    <td align=left>\n");
    strcpy(buf, article.Name); /* highlightStr()ø°º≠ Ω∫∆Æ∏µ¿ª ∫Ø«¸«œ±‚ ∂ßπÆø° ¿Ã∏ß¿ª ∫∏¡∏«—¥Ÿ */
    printf("      <font size='%s'>%s ", conf->FontSize, Written); highlightStr(1, article.Name, db->fval); printf("</font>\n");
    if(strcmp(article.Email, "") != 0){  /* Email ¿Ã ¡∏¿Á«œ∏È */
      sprintf(status_msg, "Send mail to %s (%s)", buf, article.Email);
      printf("      <font size='%s' face='Arial'>(<a href='mailto:\"%s\" <%s>' onMouseOver=\"window.status=('%s'); return true;\">%s</a>)</font>\n", conf->FontSize, buf, article.Email, status_msg, article.Email);
    }
    printf("    </td>\n");
    /* Etc. Information */
    printf("    <td align=right>\n");
    printf("      <font size='%s'>\n", conf->FontSize);
    printf("      %s: %d", Hits, article.Access);
    if(strcmp(article.Filename, "")) { /* æ˜∑ŒµÂµ» ∆ƒ¿œ¿Ã ¿÷¿Ω */
      printf(" , %s: %d", Download, article.Download);
    }
    printf(" , %s: %d", Lines, article.Lines);
    if(categoryEnable == TRUE) { /* ƒ´≈◊∞Ì∏Æ∞° ¿÷¿Ω */
      char *tmp;
      if((tmp = qsValue(catep, article.Category)) == NULL) tmp = NotSelected;
      printf(" , %s: %s", Category, tmp);
    }
    printf("\n");
    printf("      </font>\n");
    printf("    </td>\n");
    printf("  </tr>\n");
    printf("</table>\n");

    printf("\n<!-- Subject -->\n");
    printf("<table border='%s' cellspacing=1 cellpadding=1 width=%s><tr bgcolor='%s'>\n", conf->ReadTitleBorderSize, conf->TableWidth, conf->ReadTitleBgColor);
    printf("  <th><font size='%d' color='%s'>", atoi(conf->FontSize) + 1, conf->ReadTitleFontColor); highlightStr(11, article.Subject, db->fval); printf("</font></th>\n");
    printf("</tr></table>\n");

    if(strcmp(article.Filename, "")) { /* æ˜∑ŒµÂµ» ∆ƒ¿œ¿Ã ¿÷¿Ω */
      char fileicon[0xff], filepath[0xff];
      int  filetype;

      filetype = findExtIcon(sysp, sys, article.Filename, fileicon);
      sprintf(filepath, "%s/%s/%s/%s", sys->HtmlBasePath, sys->DataDirectory, db->db, article.Filename);

      printf("\n<!-- Download -->\n");
      sprintf(status_msg, "Download %s (%d Kbytes)", article.Filename, atoi(article.Filesize) / 1024);
      printf("<table cellspacing=1 border='%s' width=%s><tr bgcolor='%s'><td>\n", conf->ReadTextBorderSize, conf->TableWidth, conf->ReadTextBgColor);
      printf("  <font size='%s' color='%s'>\n", conf->FontSize, conf->ReadTextFontColor);
      printf("  <img src='%s' width=%s height=%s border=0> %s : \n", fileicon, sys->IconWidth, sys->IconHeight, Download);
      printf("  <a href='%s?db=%s&mode=download&num=%d' onMouseOver=\"window.status=('%s'); return true;\">", sys->ProgramName, db->db, db->num, status_msg);
      highlightStr(1, article.Filename, db->fval);
      printf("</a>\n");
      printf("  (%d Kbytes)\n", atoi(article.Filesize) / 1024);
      printf("  </font><br>\n");

      /* ∆ƒ¿œ¿Ã TEXT ¿Ã∞≈≥™ ±◊∏≤¿Ã∏È ±◊≥… ª—∑¡¡‹ */
      switch(filetype) {
        case TYPE_TEXT : {
          char pfilepath[0xff];
          sprintf(pfilepath, "%s/%s/%s/%s", sys->RealBasePath, sys->DataDirectory, db->db, article.Filename);

          printf("  <font size='%s' color='%s'>\n", conf->FontSize, conf->ReadTextFontColor);
          printf("  <hr size=1 noshade width=250 align=left>\n");

          printf("<!---------- Cut here ---------->\n");
          previewFile(pfilepath, atoi(sys->PreviewLines), filter);
          printf("\n<!---------- Cut here ---------->\n");

          printf("  </font>\n");
          break;
        }
        case TYPE_IMAGE : {
          printf("  <hr size=1 noshade width=250 align=left>\n");
          printf("  <img src='%s' border=0 alt='%s'>\n", filepath, article.Filename);
          break;
        }
        case TYPE_SOUND : {
          printf("  <hr size=1 noshade width=250 align=left>\n");
          printf("  <embed src='%s' width=146 height=60 autostart=true loop=false>\n", filepath);
          break;
        }
        case TYPE_MOVIE : {
          printf("  <hr size=1 noshade width=250 align=left>\n");
          printf("  <embed src='%s' width=200 height=150 autostart=true loop=false>\n", filepath);
          break;
        }
      }

      printf("</td></tr></table>\n");
    }

    if(strcmp(article.Text, "") != 0) {
      printf("\n<!-- Text -->\n");
      printf("<table border='%s' cellspacing=1 width=%s>\n", conf->ReadTextBorderSize, conf->TableWidth);
      printf("<tr bgcolor='%s'><td><font size='%s' color='%s'>\n", conf->ReadTextBgColor, conf->FontSize, conf->ReadTextFontColor);
      printf("<!---------- Cut here ---------->\n");

      /* ∂Û¿ÃºæΩ∫∞° ∫Œ¡§»Æ«œ∏Á, ¿Ø»ø±‚∞£¿Ã ¡ˆ≥µ¿ª ∞ÊøÏ */
      if(license_valid == FALSE && daysAgo(article.Year, article.Mon, article.Day) > INVALID_LICENSE_LIVE_DAYS) {
        printf("<center><font size=3 face='Arial' color='red'><b><br><br>\n");
        printf("You need a 'LICENSE KEY' to read the messages after %d days of writing.<br><br>", INVALID_LICENSE_LIVE_DAYS + 1);
        printf("<blink>INVALID LICENSE KEY</blink><br><br><br>");
        printf("</b></font></center>\n");
      }
      else { /* ∂Û¿ÃºæΩ∫∞° ¡§»Æ«œ∞≈≥™, ¿Ø»ø±‚∞£¿Ã æ»¡ˆ≥µ¿ª ∞Ê¿Ø */
        highlightStr(filter, article.Text, db->fval);
      }

      printf("\n<!---------- Cut here ---------->\n");
      printf("</font></td></tr>\n");
      printf("</table>\n");
    }

    printf("\n<!-- Date & From -->\n");
    printf("<table border=0 width=%s><tr>\n", conf->TableWidth);
    /* æ’µ⁄ ∞‘Ω√π∞ ¡¶∏Ò */
    printf("  <td align=left><font size='%s'>\n", conf->FontSize);
    /* Up */
    if(updn.upNum != 0) {
      char *tmp;
      tmp = strdup(updn.upSubject);
      sprintf(status_msg, "Previous article : %s", exchangeQuot(tmp));
      printf("    %s : <a href='%s?db=%s&mode=read&num=%d&page=%d&category=%s&ftype=%d&fval=%s&backdepth=%d' onMouseOver=\"window.status=('%s'); return true;\">", Prev, sys->ProgramName, db->db, updn.upNum, db->page, db->category, db->ftype, db->fencval, db->backdepth + 1, status_msg);
      highlightStr(11, tmp, db->fval);
      printf("</a><br>\n");
      free(tmp);
    }
    /* Down */
    if(updn.dnNum != 0) {
      char *tmp;
      tmp = strdup(updn.dnSubject);
      sprintf(status_msg, "Next article : %s", exchangeQuot(tmp));
      printf("    %s : <a href='%s?db=%s&mode=read&num=%d&page=%d&category=%s&ftype=%d&fval=%s&backdepth=%d' onMouseOver=\"window.status=('%s'); return true;\">", Next, sys->ProgramName, db->db, updn.dnNum, db->page, db->category, db->ftype, db->fencval, db->backdepth + 1, status_msg);
      highlightStr(11, tmp, db->fval);
      printf("</a><br>\n");
      free(tmp);
    }
    printf("  </font></td>\n");

    /* Date */
    printf("  <td align=right><font size='%s' face='Arial'>", conf->FontSize);
    if(!strcmp(conf->Language, "english")) printf("%s %d, %04d (%02d:%02d)", month[article.Mon], article.Day, article.Year, article.Hour, article.Min);
    else printf("%04d/%02d/%02d (%02d:%02d)", article.Year, article.Mon, article.Day, article.Hour, article.Min);
    if(!strcmp(conf->DisplayAddress, "true")) printf(" from %s", article.Domain);
    printf("<br><font size='%d' face='Arial' color='gray'><b>%s %s</b></font>\n", atoi(conf->FontSize) - 1, OFFICIAL_PROGRAM_NAME, OFFICIAL_PROGRAM_VERSION);
    printf("</td>\n");

    printf("</tr></table>\n");

    printf("\n<!-- HR Line -->\n");
    printf("<table border=0 width=%s cellpadding=0 cellspacing=0>\n", conf->TableWidth);
    printf("  <tr><td><hr size=1 noshade></td></tr>\n");
    printf("</table>\n");

  /* ICON Link */
  if(!strcmp(conf->ReadIconLocation, "bottom") || !strcmp(conf->ReadIconLocation, "both") || !strcmp(conf->ReadIconLocation, "auto")) {
    readIconLink(sys, db, conf, &updn);
  }

  /* ≤ø∏Æ∏ª ∫∏≥ø */
  tailHtml(sys, conf);

  freeArticle(&article);
}

void readIconLink(SysConfig *sys, DB *db, DBConfig *conf, UpDnInfo *updn) {
  int  replyPage, replyFtype;
  char status_msg[0xff];                 /* ∫Í∂ÛøÏ¿˙ «œ¥‹ ªÛ≈¬πŸø° ≥™≈∏≥™¥¬ ∏ﬁΩ√¡ˆ */

  /* list∞° date¿Ã∏È ∞¸∑√±€ ¿€º∫»ƒ 1∆‰¿Ã¡ˆ∑Œ ∞£¥Ÿ */
  if(!strcmp(conf->ListOrder, "date")) {
    replyPage  = 1;
    replyFtype = 0;
  }
  else {
    replyPage  = db->page;
    replyFtype = db->ftype;
  }

  printf("\n<!-- ICON Link -->\n");
  printf("<table border=0 width=%s><tr>\n", conf->TableWidth);
  printf("  <td align=left>\n");
  /* Modify */
  strcpy(status_msg, "Modify this article");
  printf("    <a href='%s?db=%s&mode=modify&num=%d&page=%d&category=%s&ftype=%d&fval=%s&backdepth=%d' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Modify'></a>\n", sys->ProgramName, db->db, db->num, db->page, db->category, db->ftype, db->fencval, db->backdepth + 1, status_msg, sys->HtmlBasePath, sys->IconModify);
  /* Delete */
  strcpy(status_msg, "Delete this article");
  printf("    <a href='%s?db=%s&mode=delete&num=%d&page=%d&category=%s&ftype=%d&fval=%s&backdepth=%d' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Delete'></a>\n", sys->ProgramName, db->db, db->num, db->page, db->category, db->ftype, db->fencval, db->backdepth + 1, status_msg, sys->HtmlBasePath, sys->IconDelete);
  /* Write */
  strcpy(status_msg, "Write a new message");
  printf("    <a href='%s?db=%s&mode=write&category=%s&backdepth=%d' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Post'></a>\n", sys->ProgramName, db->db, db->category, db->backdepth + 1, status_msg, sys->HtmlBasePath, sys->IconWrite);
  /* Reply */
  strcpy(status_msg, "Reply to this article");
  printf("    <a href='%s?db=%s&mode=reply&num=%d&page=%d&category=%s&ftype=%d&fval=%s&backdepth=%d' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Reply'></a>\n", sys->ProgramName, db->db, db->num, replyPage, db->category, replyFtype, db->fencval, db->backdepth + 1, status_msg, sys->HtmlBasePath, sys->IconReply);
  printf("  </td>\n");

  printf("  <td align=right>\n");
  /* Up */
  if(updn->upNum != 0) {
    sprintf(status_msg, "Previous article : %s", exchangeQuot(updn->upSubject));
    printf("    <a href='%s?db=%s&mode=read&num=%d&page=%d&category=%s&ftype=%d&fval=%s&backdepth=%d' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Backward'></a>\n", sys->ProgramName, db->db, updn->upNum, db->page, db->category, db->ftype, db->fencval, db->backdepth + 1, status_msg, sys->HtmlBasePath, sys->IconUp);
  }
  /* Down */
  if(updn->dnNum != 0) {
    sprintf(status_msg, "Next article : %s", exchangeQuot(updn->dnSubject));
    printf("    <a href='%s?db=%s&mode=read&num=%d&page=%d&category=%s&ftype=%d&fval=%s&backdepth=%d' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Forward'></a>\n",  sys->ProgramName, db->db, updn->dnNum, db->page, db->category, db->ftype, db->fencval, db->backdepth + 1, status_msg, sys->HtmlBasePath, sys->IconDown);
  }
  /* List */
  strcpy(status_msg, "Back to the list");
  if(db->backdepth == 0) { /* ªÁøÎ¿⁄∞° ºˆµø¿˚¿∏∑Œ µÈæÓø‘¥Ÿ. */
    printf("    <a href='%s?db=%s' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='List'></a>\n", sys->ProgramName, db->db, status_msg, sys->HtmlBasePath, sys->IconList);
  }
  else printf("    <a href='javascript:history.go(-%d)' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='List'></a>\n", db->backdepth, status_msg, sys->HtmlBasePath, sys->IconList);

  printf("  </td>\n");
  printf("</tr></table>\n");
}

int getArticle(char *dbpath, int num, DBArticle *article) {
  char buf[20 + 1];

  DBOpen(dbpath, GDBM_WRITER);

  article->Num  = num;

  if((article->Name    = qRemoveSpace(DBFetch("%d.Name", num))) == NULL) return FALSE;
  if((article->Email   = qRemoveSpace(DBFetch("%d.Email", num))) == NULL) return FALSE;
  if((article->Subject = qRemoveSpace(DBFetch("%d.Subject", num))) == NULL) return FALSE;
  if((article->Text    = DBFetch("%d.Text", num)) == NULL) return FALSE;
  if((article->Category= DBFetch("%d.Category", num)) == NULL) article->Category = strdup("");

  if((article->Filename = DBFetch("%d.Filename", num)) == NULL) article->Filename = strdup("");
  if((article->Filesize = DBFetch("%d.Filesize", num)) == NULL) article->Filesize = strdup("");
  article->Download = DBiFetch("%d.Download", num);

  if((article->Date   = DBFetch("%d.Date", num)) == NULL) article->Date = strdup("");
  sscanf(article->Date, "%4d%2d%2d%2d%2d%2d", &article->Year, &article->Mon, &article->Day, &article->Hour, &article->Min, &article->Sec);
  if((article->Domain = DBFetch("%d.Domain", num)) == NULL) article->Domain = strdup("");

  article->Lines  = DBiFetch("%d.Lines", num);
  article->Access = DBiFetch("%d.Access", num);
  article->Thread = DBiFetch("%d.Thread", num);

  /* Access Update */
  article->Access++;
  sprintf(buf, "%d.Access", num);
  DBiStore(buf, article->Access, GDBM_REPLACE);

  DBClose();

  exchangeQuot(article->Name);

  /* Adjust Empty Field */
  if(!strcmp(article->Name, "")) {
     free(article->Name);
     article->Name = strdup("(anonymous)");
  }
  if(!strcmp(article->Subject, "")) {
     free(article->Subject);
     article->Subject = strdup("(no subject)");
  }
  if(!strcmp(article->Domain, "")) {
    free(article->Domain);
    article->Domain = strdup("(anonymous host)");
  }

  return TRUE;
}

void freeArticle(DBArticle *article) {
#define Free(NAME) if(article->NAME != NULL) free(article->NAME);
  Free(Name);
  Free(Email);
  Free(Subject);
  Free(Text);
  Free(Category);
  Free(Filename);
  Free(Filesize);
  Free(Date);
  Free(Domain);
#undef Free
}

int previewFile(char *filename, int lines, int filter) {
  FILE *fp;
  char buf[1000 + 1];
  int i;

  fp = fopen(filename, "rt");
  if(fp == NULL) return FALSE;

  for(i = 1; (i <= lines) || (lines == 0); i++) {
    if(fgets(buf, sizeof(buf), fp) == NULL) break;
    qPuts(filter, buf);
  }

  return TRUE;
}
