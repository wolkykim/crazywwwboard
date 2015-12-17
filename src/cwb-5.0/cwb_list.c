#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "cwb.h"
#include "cwb_admin.h"
#include "cwb_list.h"

static D_INDEX *dIndex = NULL;
static T_INDEX *tIndex = NULL;

static int totalArticle = 0;

/********* Index Stack *************/
#define STACK_SIZE	(100)  /* «— ∞‘Ω√π∞ø° ¡˜∑ƒ∑Œ ¡∏¿Á«“ºˆ ¿÷¥¬ æ≤∑πµÂ¿« ∞Ÿºˆ */
typedef struct INDEX_STACK {
  int num1[STACK_SIZE+1];
  int num2[STACK_SIZE+1];
}INDEX_STACK;

static INDEX_STACK stack;
static int sp = 0;

/********** Common Functions **************/
int  main_index(DB *db, DBConfig *conf);
int  get_index(int num, int ftype, char *fval, char *category);
void adjust_index(void);
int  findfval(int num, int ftype, char *fval);

void date_index(void);

void thread_index(void);
int  findThread(int num, int upper);
int  insertIndex(int num, int depth);
int  pushIndex(int num, int i);
int  popIndex(int *num, int *i);

void print_index(Entry *sysp, SysConfig *sys, DB *db, DBConfig *conf, int tArticle);
void listIconLink(SysConfig *sys, DB *db, DBConfig *conf, int pages, char *searchBoundStr);
void shotSearch(SysConfig *sys, DB *db, DBConfig *conf, char *boundstr);
int  getArticleInfo(int num, DBList *head);
void freeArticleInfo(DBList *head);

int daysAgo(int year, int mon, int day);
int yearDays(int year, int mon, int day);
int leapYear(int year);

/*********** Main ***************************/
void cwb_list(Entry *sysp, SysConfig *sys, DB *db, DBConfig *conf){
  char buf[0xFF];
  int  tArticle;

  /* ∞¸∏Æ¿⁄∏∏ ∏Ò∑œ¿ª ∫ººˆ ¿÷¥Ÿ∏È, æœ»£ ∞ÀªÁ */
  if(!strcmp(conf->ListMode, "admin")) check_password(sys, db, conf);

  /* DB Open */
  sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
  DBOpen(buf, GDBM_READER);

  tArticle = main_index(db, conf);

  print_index(sysp, sys, db, conf, tArticle);

  /* DB Close */
  DBClose();

  if(dIndex != NULL) free(dIndex);
  if(tIndex != NULL) free(tIndex);
}

/* ∞‘Ω√π∞¿« ¿ßæ∆∑° π¯»£∏¶ »πµÊ«—¥Ÿ, ∫ª«‘ºˆ¥¬ ∏Ò∑œ ∫∏±‚ ∏µ‚∞˙ ªÛ∞¸¿Ã æ¯¿∏∏Á
   cwb_read() ø°º≠ »£√‚µ»¥Ÿ. DB¥¬ ø¿«¬µ«æÓ ƒ›µ»¥Ÿ.*/
void findUPDNnum(char *dbpath, DB *db, DBConfig *conf, UpDnInfo *updn) {
  int  i;

  DBOpen(dbpath, GDBM_READER);

  main_index(db, conf);

  for(i = 0; tIndex[i].num != 0; i++) {
    if(tIndex[i].num == db->num) {
      if(i - 1 < 0) updn->upNum = 0;
      else updn->upNum = tIndex[i-1].num;

      updn->dnNum = tIndex[i+1].num;
    }
  }

  if(updn->upNum == 0) strcpy(updn->upSubject, "");
  else strcpy(updn->upSubject, qRemoveSpace(DBFetch("%d.Subject", updn->upNum)));
  if(updn->dnNum == 0) strcpy(updn->dnSubject, "");
  else strcpy(updn->dnSubject, qRemoveSpace(DBFetch("%d.Subject", updn->dnNum)));

  DBClose();
}


/*************************************
** DB∏¶ ø≠∞Ì ∫ª«‘ºˆ∏¶ »£√‚«œ∏È, ∏µÁ ªÛ»≤¿ª ¡∂«’«œø© 
** √÷¡æ ∏ÆΩ∫∆Æ∏¶ ∏∏µÈ∞Ì, ±◊ ∞Ÿºˆ∏¶ ∏Æ≈œ«—¥Ÿ.
** ∏Æ≈œ∞™¿∫ ∆–≈œ∞Àªˆø° º∫∞¯«— ∞‘Ω√π∞¿« ∞πºˆ¿Ã∏Á 
** ±◊∞Õ∏∏¿Ã ∏ÆΩ∫∆Æø° ¿˙¿Âµ»¥Ÿ.
** ∏µÁ ¿Œµ¶Ω∫ «‘ºˆ∏¶ »£√‚«œ¥¬ ∏ﬁ¿Œ¿Ã¥Ÿ.
**************************************/
int main_index(DB *db, DBConfig *conf){
  int  tArticle;

  tArticle = get_index(conf->LastNum, db->ftype, db->fval, db->category);
  if(tArticle == -1) qError("There is a fatal error in database(%s). You should better call the administrator.", db->db);

  adjust_index();

  if(!strcmp(conf->ListOrder, "date")) date_index();
  else if(!strcmp(conf->ListOrder, "thread")) thread_index();
  else qError("There is a fatal error in database(%s). You should better call the administrator.", db->db);

  return tArticle;
}

/*************************************
** DBø°º≠ ∏Ò∑œ¿ª ∏∏µÁ¥Ÿ, ∆–≈œ∞Àªˆ«œø© ∞…∑Ø¡¯ ∞‘Ω√π∞¿Ã ¿Œµ¶ΩÃµ»¥Ÿ.
** [0] -> 10, 0 (num, thread)
** [1] ->  9, 1
** [2] ->  8, 0
** [n] ->  0, 0
** ∞‘Ω√π∞¿« ∞πºˆ∏¶ ∫π±Õ«—¥Ÿ. ø°∑ØΩ√ -1
**************************************/
int get_index(int num, int ftype, char *fval, char *category) {
  int i, j;
  char *str, *tmp;

  if(num < 0) return -1;

  dIndex = (D_INDEX *)malloc(sizeof(D_INDEX) * (num + 1));
  if(dIndex == NULL) qError("Memory allocation fail.");
  tIndex = (T_INDEX *)malloc(sizeof(T_INDEX) * (num + 1));
  if(tIndex == NULL) qError("Memory allocation fail.");

  /* DB Fetch */
  for(i = num, j = 0; i > 0; i--){

    /* Thread∞° DBø° ¡∏¿Á«œ¡ˆ æ ¿∏∏È, ∞‘Ω√π∞¿Ã ªË¡¶µ«æ˙¥Ÿ∞Ì ∞°¡§«—¥Ÿ */
    if((str = DBFetch("%d.Thread", i)) != NULL) {  /* ∞‘Ω√π∞ ¡∏¿Á */
      totalArticle++;  /* ¿¸√º ∞≥Ω√π∞ºˆ ¡ı∞° */

      /* ƒ´≈◊∞Ì∏Æ ∫–∑˘ */
      if(*category != '\0') {
        if((tmp = DBFetch("%d.Category", i)) == NULL) tmp = strdup("null"); /* Not selected ∞Àªˆ */
        if(strcmp(tmp, category)) { free(tmp), free(str); continue; }
        free(tmp);
      }

      /* ∆–≈œ ∞ÀªÁ */
      if(findfval(i, ftype, fval) == TRUE) {
        dIndex[j].num    = i;
        dIndex[j].thread = atoi(str);
        j++;
      }
      free(str);
    }
  }
  dIndex[j].num = dIndex[j].thread = 0;

  return j;
}

/*************************************
** index∏¶ ¡∂¡§«—¥Ÿ. ∆–≈œ∞ÀªÁ∏¶ ∏∂ƒ£ ¿Œµ¶Ω∫¥¬ ∏æ≤∑πµÂ∞° ¡∏¿Á«œ¡ˆ æ ¥¬ 
** ∞‘Ω√π∞¿Ã ¿÷¿ªºˆ ¿÷¥Ÿ.
** ∏ æ≤∑πµÂ∞° ¿Œµ¶Ω∫ø° æ¯¿∏∏È ¬¸¡∂π¯»£∏¶ 0 ¿∏∑Œ «œø© Thread list order
** ¿œ∂ß ∞‘Ω√π∞¿Ã »Æ¿Œµ«µµ∑œ «—¥Ÿ.
**************************************/
void adjust_index(void) {
  int i, j;

  /* DB Fetch */
  for(i = 0; dIndex[i].num != 0; i++){
    if(dIndex[i].thread == 0) continue;

    /* ∏ æ≤∑πµÂ∞° ¡∏¿Á«œ¥¬¡ˆ ∞ÀªÁ */
    for(j = i + 1; dIndex[j].num != 0; j++) {
      if(dIndex[j].num == dIndex[i].thread) break;
    }
    /* ¬¸¡∂ ∞‘Ω√π∞ æ¯¿Ω */
    if(dIndex[j].num == 0) dIndex[i].thread = 0;
  }
}

/* ∆–≈œ∞ÀªÁ«œø© ∞‘Ω√π∞¿Ã «’¥Á«—¡ˆ ∆«∫∞«—¥Ÿ.*/
int findfval(int num, int ftype, char *fval) {
  char *name, *filename, *subject, *text;
  int  findflag;

  /* ∞ÀªˆπÆ¿⁄ø≠¿Ã æ¯¿ª∞ÊøÏ¥¬ √£¿∫∞…∑Œ ∞£¡÷ */
  if(!strcmp(fval, "") || ftype == 0) return TRUE;
  
  /* √ ±‚»≠ */
  name = filename = subject = text = NULL;

  /* Fetch */
  if((ftype & FNAME)    != 0) {
    name     = DBFetch("%d.Name", num);
    filename = DBFetch("%d.Filename", num);
  }
  if((ftype & FSUBJECT) != 0) subject = DBFetch("%d.Subject", num);
  if((ftype & FTEXT)    != 0) text    = DBFetch("%d.Text", num);

  /* NULL¿Ã æ∆¥—∞Õ¿ª √£¥¬¥Ÿ*/
  if(qStrStr(name, fval)          != NULL) findflag = TRUE;
  else if(qStrStr(filename, fval) != NULL) findflag = TRUE;
  else if(qStrStr(subject, fval)  != NULL) findflag = TRUE;
  else if(qStrStr(text, fval)     != NULL) findflag = TRUE;
  else findflag = FALSE;

  /* Free */
  if(name     != NULL) free(name);
  if(filename != NULL) free(filename);
  if(subject  != NULL) free(subject);
  if(text     != NULL) free(text);

  return findflag;
}

/* Date List */
void date_index(void) {
  int i;
  for(i = 0; dIndex[i].num != 0; i++){
    insertIndex(dIndex[i].num, 0);
  }
  insertIndex(0, 0);
}

/* Thread List */
void thread_index(void) {
  int di;  /* dIndex¿« index ∞™ */

  int threadNum;  /* æ≤∑πµÂ ∞‘Ω√π∞¿« æˆ∏∂ ∞‘Ω√π∞ π¯»£ */
  int threadI;    /* æ≤∑πµÂ ∞‘Ω√π∞¿« dIndex ∞™ */
  int depth;
  int exit;       /* ∑Á«¡ ≈ª√‚ «¡∑π±◊ */

  for (di = 0; dIndex[di].num != 0; di++){
    /* thread∞° 0¿Œ ∞‘Ω√π∞¿ª √£¥¬¥Ÿ. */
    if(dIndex[di].thread != 0) continue;

    /* √≥∏Æ∞° ø©±‚ø° µµ¥ﬁ«‘¿∫, √÷ªÛ¿ß ∞‘Ω√π∞¿ª √£æ“¿Ω¿ª ¿«πÃ«—¥Ÿ */

    /* tIndexø° depth 0¿∏∑Œ ∞‘Ω√π∞¿ª √ﬂ∞°«—¥Ÿ. */
    insertIndex(dIndex[di].num, 0);

    /* √£¿ª æ≤∑πµÂ¿« æˆ∏∂ num ∞™∞˙, upper ∞™¿ª º≥¡§«—¥Ÿ */
    threadNum = dIndex[di].num, threadI = di;

    exit = FALSE;
    do {
      /* æ≤∑πµÂ∏¶ √£¥¬¥Ÿ */
      threadI = findThread(threadNum, threadI);

      if(threadI >= 0) { /* æ≤∑πµÂ∏¶ √£æ“¿Ω */
        /* Stackø° æ≤∑πµÂ æˆ∏∂ num∞˙ √£¿∫ æ≤∑πµÂ¿« index∏¶ «™Ω√«—¥Ÿ */
        depth = pushIndex(threadNum, threadI);

        /* ∞‘Ω√π∞ √ﬂ∞° */
        insertIndex(dIndex[threadI].num, depth);

        /* ¥Ÿ¿Ωπ¯ √£¿ª æ≤∑πµÂ∞™¿ª º≥¡§«—¥Ÿ */
        threadNum = dIndex[threadI].num;
      }
      else { /* æ≤∑πµÂ ∏¯ √£æ“¿Ω */
        if(popIndex(&threadNum, &threadI) == FALSE) exit = TRUE;
      }
    }while(exit == FALSE);
  }
  insertIndex(dIndex[di].num, 0);
}

/* num¿ª ¬¸¡∂ «œ¥¬ æ≤∑πµÂ ∞‘Ω√π∞¿ª dIndex[upper-1] ∫Œ≈Õ √£¥¬¥Ÿ */
int findThread(int num, int upper){
  int i;

  for(i = upper - 1; i >= 0; i--){
    if(dIndex[i].thread == num) return i;
  }

  return -1;
}

/* tIndexø° ∞‘Ω√π∞¿ª √ﬂ∞°«—¥Ÿ. ∫π±Õ∞™¿∫ ∞‘Ω√π∞¿« «ˆ¿Á ∞πºˆ */
int insertIndex(int num, int depth) {
  static int Ip = 0;

  tIndex[Ip].num   = num;
  tIndex[Ip].depth = depth;
  Ip++;

  return Ip;
}

/* æ≤∑πµÂ¿« æˆ∏∂ num∞˙, æ≤∑πµÂ ¿⁄Ω≈¿« Index∏¶ «™Ω√«—¥Ÿ */
/* ∫π±Õ ∞™¿∫ «ˆ¡¶ Ω∫≈ÿ¿« ∞πºˆ */
int pushIndex(int num, int i) {
  if(sp > STACK_SIZE) return FALSE;
  stack.num1[sp] = num;
  stack.num2[sp] = i;
  sp++;
  return sp;
}

int popIndex(int *num, int *i) {
  if(sp == 0) return FALSE;
  sp--;
  *num = stack.num1[sp];
  *i   = stack.num2[sp];
  return TRUE;
}

void print_index(Entry *sysp, SysConfig *sys, DB *db, DBConfig *conf, int tArticle){
  Entry *catep;
  DBList head;
  char *Number, *Type, *Filename, *Subject, *Name, *Date, *Access, *Download, *Articles, *Pages;
  char *Category, *NotSelected;
  int categoryEnable;

  char fileicon[0xff];

  int startNum,  endNum, pages;
  int startPage, endPage;

  int  i, virtualnum;

  char status_msg[0xff], *tmp; /* Ω∫≈©∏∞ «œ¥‹¿« ªÛ≈¬πŸø° ≥™≈∏≥æ ∏ﬁΩ√¡ˆ */

  char *abbMonth[12+1] = {"",
                         "Jan",
                         "Feb",
                         "Mar",
                         "Apr",
                         "May",
                         "Jun",
                         "Jul",
                         "Aug",
                         "Sep",
                         "Oct",
                         "Nov",
                         "Dec"};

  if(!strcmp(conf->Language, "korean")) {
    Number      = "<font face='±º∏≤'>π¯»£</font>";
    Type        = "<font face='±º∏≤'>®œ</font>";
    Filename    = "<font face='±º∏≤'>∆ƒ¿œ∏Ì</font>";
    Subject     = "<font face='±º∏≤'>¡¶ ∏Ò</font>";
    Name        = "<font face='±º∏≤'>º∫ ∏Ì</font>";
    Category    = "<font face='±º∏≤'>∫– ∑˘</font>";
    NotSelected = "<font face='±º∏≤'>±‚≈∏</font>";
    Date        = "<font face='±º∏≤'>¿€º∫¿œ</font>";
    Access      = "<font face='±º∏≤'>¡∂»∏</font>"; 
    Download    = "<font face='±º∏≤'>¿¸º€</font>"; 
    Articles    = "<font face='±º∏≤'>∞Àªˆµ» ∞‘Ω√π∞</font>";
    Pages       = "<font face='±º∏≤'>¬ π¯»£</font>";
  }
  else if(!strcmp(conf->Language, "japanese")) {
    Number      = "î‘çÜ";
    Type        = "(C)";
    Filename    = "ÉtÉ@ÉCÉãñº";
    Subject     = "ëËÅ@ñ⁄";
    Name        = "ÉlÅ[ÉÄ";
    Category    = "ÉJÉeÉSÉä";
    NotSelected = "ÉmÅ[É}Éã";
    Date        = "ì˙ït";
    Access      = "ÉqÉbÉg"; 
    Download    = "ì]ëó";
    Articles    = "ÉAÉEÉgÉvÉbÉg";
    Pages       = "ÉyÅ[ÉW";
  }
  else {
    Number      = "<font face='Arial'>No</font>";
    Type        = "<font face='Arial'>&copy;</font>";
    Filename    = "<font face='Arial'>Filename</font>";
    Subject     = "<font face='Arial'>S u b j e c t</font>";
    Name        = "<font face='Arial'>Name</font>";
    Category    = "<font face='Arial'>Category</font>";
    NotSelected = "<font face='Arial'>Etc.</font>";
    Date        = "<font face='Arial'>Date</font>";
    Access      = "<font face='Arial'>Hits</font>";
    Download    = "<font face='Arial'>Down</font>";
    Articles    = "<font face='Arial'>Searched Articles</font>";
    Pages       = "<font face='Arial'>Pages</font>";
  }

  /* Subject Text∞° ¿÷¿∏∏È ±◊∞Õ¿ª ªÁøÎ«—¥Ÿ */
  if(strcmp(conf->SubjectText, "")) Subject = conf->SubjectText;

  /* ƒ´≈◊∞Ì∏Æ ªÁøÎø©∫Œ ∆«∫∞ & ƒ´≈◊∞Ì∏Æ ∆ƒΩÃ */
  catep = qsDecoder(conf->Category);
  if(!strcmp(conf->UseCategory, "true") && catep != NULL) {
    categoryEnable = TRUE;
    if((tmp = qsValue(catep, "_TITLE")) != NULL) Category = tmp;
    if((tmp = qsValue(catep, "_NOTSELECTED")) != NULL) NotSelected = tmp;
  }
  else categoryEnable = FALSE;

  /* Page ∞πºˆ ∞≥ªÍ */
  if(tArticle == 0) pages = 0;
  else {
    int pof;

    if((tArticle % atoi(conf->ArticlesPerPage)) == 0) pof = 0;
    else pof = 1;

    pages = (tArticle / atoi(conf->ArticlesPerPage)) + pof;
  }

  /* ∏µ≈© Page π¸¿ß ∞ËªÍ */
  if(pages == 0) startPage = endPage = 0;
  else {
    int pof;

    if((db->page % atoi(sys->PageLinkNum)) == 0) pof = 0;
    else pof = 1;

    endPage   = ((db->page / atoi(sys->PageLinkNum)) + pof) * atoi(sys->PageLinkNum);
    startPage = endPage - (atoi(sys->PageLinkNum) - 1);
  }

  /* ∫∏ø©¡Ÿ ∆‰¿Ã¡ˆ ±≥¡§ */
  if(db->page > pages) db->page = pages;

  /* ¬Ô¿ª Index π¯»£ ∞ËªÍ */
  if(db->page == 0) startNum = endNum = 0;
  else {
    startNum = (db->page - 1) * atoi(conf->ArticlesPerPage);
    endNum   = startNum + (atoi(conf->ArticlesPerPage) - 1 );
  }

  /* Header √‚∑¬ */
  headHtml(sys, conf);

  /* ICON Link */
  if(!strcmp(conf->ListIconLocation, "top") || !strcmp(conf->ListIconLocation, "both")) {
    char *boundStr;

    if(!strcmp(conf->ListIconLocation, "both")) boundStr = NULL; /* ∞Àªˆ√¢ «•Ω√ æ»«‘ */
    else boundStr = " ";
    listIconLink(sys, db, conf, pages, boundStr);
  }

  printf("\n<!-- HR Line -->\n");
  printf("<table border=0 width=%s cellpadding=0 cellspacing=0>\n", conf->TableWidth);
  printf("  <tr><td><hr size=1 noshade></td></tr>\n");
  printf("</table>\n");

  /* DB Information */
  printf("\n<!-- DB Information -->\n");
  printf("<table border=0 width=%s><tr>\n", conf->TableWidth);
  printf("  <td align=left>\n");
  if(strcmp(sys->HiddenAdminLink, "true") != 0) {
    strcpy(status_msg, "Change database configurations, only administrator can access");
    printf("  <a href='%s?db=%s&mode=admin&backdepth=1' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Administration'></a>\n", sys->ProgramName, db->db, status_msg, sys->HtmlBasePath, sys->IconAdmin);
  }
  if(strcmp(sys->HiddenStatLink, "true") && strcmp(sys->StatDirectory, "")) {
    strcpy(status_msg, "Analysis statistics");
    printf("  <a href='%s?db=%s&mode=stat&backdepth=1' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Statistics'></a>\n", sys->ProgramName, db->db, status_msg, sys->HtmlBasePath, sys->IconStat);
  }
  printf("  </td>\n");
  printf("  <td align=right><font size='%d'>%s : %d / %d , %s : %d / %d</font></td>\n", atoi(conf->FontSize) - 1, Articles, tArticle, totalArticle, Pages, db->page, pages);
  printf("</tr></table>\n");

  /* Body */
  printf("\n<!-- List Body -->\n");
  printf("<table border='%s' cellspacing=1 width=%s>\n", conf->ListBorderSize, conf->TableWidth);
  printf("  <tr bgcolor='%s'>\n", conf->ListTitleBgColor);
  if(strcmp(conf->FieldNumber, "hidden"))  printf("    <th><font size='%s' color='%s'>%s</font></th>\n", conf->FontSize, conf->ListTitleFontColor, Number);
  if(!strcmp(conf->FieldName, "before"))   printf("    <th><font size='%s' color='%s'>%s</font></th>\n", conf->FontSize, conf->ListTitleFontColor, Name);
  if(!strcmp(conf->FieldFiletype, "show")) printf("    <th><font size='%s' color='%s'>%s</font></th>\n", conf->FontSize, conf->ListTitleFontColor, Type);
  if(!strcmp(conf->FieldFilename, "show")) printf("    <th><font size='%s' color='%s'>%s</font></th>\n", conf->FontSize, conf->ListTitleFontColor, Filename);
  printf("    <th><font size='%s' color='%s'>%s</font></th>\n", conf->FontSize, conf->ListTitleFontColor, Subject);
  if(!strcmp(conf->FieldName, "after"))    printf("    <th><font size='%s' color='%s'>%s</font></th>\n", conf->FontSize, conf->ListTitleFontColor, Name);
  if(categoryEnable == TRUE) {
    Entry *entries;
    printf("  <th>\n");
    printf("  <table border=0 cellspacing=0 cellpadding=0><form><tr><td><font size=2>\n");
    printf("    <select name=category onChange=\"window.location='%s?db=%s&mode=list&category='+form.category.options[form.category.selectedIndex].value\">\n", sys->ProgramName, db->db);
    printf("    <option value=''>%s\n", Category);
    for(entries = catep; entries; entries = entries->next){
      if(entries->name[0] == '_') continue;
      if(!strcmp(db->category, entries->name)) tmp = "selected";
      else tmp = "";
      printf("    <option value='%s' %s>%s\n", entries->name, tmp, entries->value);
    }
    if(!strcmp(db->category, "null")) tmp = "selected";
    else tmp = "";
    printf("    <option value='null' %s>%s\n", tmp, NotSelected);
    printf("    </select>\n");
    printf("  </font></td></tr></form></table>\n");
    printf("  </th>\n");
  }
  if(!strcmp(conf->FieldDate, "show"))     printf("    <th><font size='%s' color='%s'>%s</font></th>\n", conf->FontSize, conf->ListTitleFontColor, Date);
  if(!strcmp(conf->FieldHits, "show"))     printf("    <th><font size='%s' color='%s'>%s</font></th>\n", conf->FontSize, conf->ListTitleFontColor, Access);
  if(!strcmp(conf->FieldDownload, "show")) printf("    <th><font size='%s' color='%s'>%s</font></th>\n", conf->FontSize, conf->ListTitleFontColor, Download);
  printf("  </tr>\n\n");

  /* ∏Ò∑œ∫∏±‚ πÊΩƒ¿Ã thread¿œ∂ßø°¥¬ ∞¸∑√±€µÈ∂ßπÆø° ∏Ò∑œ¿« ∞‘Ω√π∞ π¯»£∞°
     æ˚ƒ—¿÷¥Ÿ. µ˚∂Ûº≠ ¿Ã∂ßø°¥¬ ¿¸√º∞ËΩ√π∞ø° ¥Î«— ªÛ¥Îπ¯»£∏¶ ∞ËªÍ«œø© 
     ¿œ∑¸¿˚¿∏∑Œ ∫∏ø©¡Ÿºˆ¿÷µµ∑œ Virtual Num ø…º«¿ª ¡÷æ˙¥Ÿ.
     ¿Ã πÊΩƒ¿« ¥‹¡°¿∫ «œ¿ß ∞‘Ω√π∞¿Ã ªË¡¶µ«∞≈≥™, «œ¿ßø° ∞¸∑√±€¿Ã ¿€º∫µ«∏È 
     ∞‘Ω√π∞¿« π¯»£∞° ∞°∫Ø¿˚¿∏∑Œ ∫Ø«—¥Ÿ¥¬ ∞Õ¿Ã¥Ÿ.
     ±◊∑Ø≥™, ∞¸∑√±€¿œ∂ß ∞‘Ω√π∞π¯»£∏¶ ª˝∑´«œ¥¬ πÊΩƒ∂««— ¿Ãª°¿Ã ∫¸¡¯µÌ«œø© 
     ø‹∞¸ªÛ ∫∞∑Œ æ»¡¡¥Ÿ.
     ∞¸∏Æ∏µÂø°º≠ ªÁøÎ¿⁄∞° ¿Ã∏¶ º±≈√«œø©, π∞∏Æ¿˚ π¯»£∏¶ «•Ω√«“¡ˆ ∞°ªÛπ¯»£∏¶ 
     «•Ω√«“¡ˆ º±≈√≈‰∑œ«—¥Ÿ. */

  virtualnum = tArticle - ((db->page - 1) * atoi(conf->ArticlesPerPage));

  /* tIndex ∏Ò∑œ¿« ≥°ø£ 0 « µÂ∞° ª¿‘µ«æÓ ¿÷¥Ÿ */
  for(i = startNum; (i <= endNum) && (tIndex[i].num != 0); i++, virtualnum--){
    getArticleInfo(tIndex[i].num, &head);

    printf("  <tr bgcolor='%s'><!-- Article %d -->\n", conf->ListArticleBgColor, tIndex[i].num);

    /* Article Number */
    if(strcmp(conf->FieldNumber, "hidden")) {
      printf("    <td align=center><font size='%s' color='%s' face='Arial'>", conf->FontSize, conf->ListArticleFontColor);
      if(!strcmp(conf->FieldNumber, "virtual")) printf("%d", virtualnum);
      else printf("%d", tIndex[i].num);
      printf("</font></td>\n");
    }

    /* Name First*/
    if(!strcmp(conf->FieldName, "before")) {
      printf("    <td align=center><font size='%s' color='%s'>", conf->FontSize, conf->ListArticleFontColor);
      if(!strcmp(conf->FieldSubject, "cut")) printf("<nobr>");
      if(!strcmp(head.Email, "")) highlightStr(1, head.Name, db->fval);
      else {
        sprintf(status_msg, "Send mail to %s (%s)", head.Name, head.Email);
        printf("<a href='mailto:\"%s\" <%s>' onMouseOver=\"window.status=('%s'); return true;\">", head.Name, head.Email, status_msg);
        printf("<font color='%s'>", conf->ListArticleFontColor);
        highlightStr(1, head.Name, db->fval);
        printf("</font>");
        printf("</a>");
      }
      if(!strcmp(conf->FieldSubject, "cut")) printf("</nobr>");
      printf("</font></td>\n");
    }

    /* File Type Icon */
    if(!strcmp(conf->FieldFiletype, "show")) {
      printf("    <td align=center>");
      switch(findExtIcon(sysp, sys, head.Filename, fileicon)) {
        case TYPE_UNLINK : {
          printf("<img src='%s' width=%s height=%s alt='' border=0>", fileicon, sys->IconWidth, sys->IconHeight);
          break;
        }
        default : { /* æ˜∑Œµ˘ ∆ƒ¿œ¿Ã ¿÷¿Ω */
          sprintf(status_msg, "Download : %s (%d Kbytes) , Downloaded %d times", head.Filename, atoi(head.Filesize) / 1024, head.Download);
          printf("<a href='%s?db=%s&mode=download&num=%d' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s' width=%s height=%s border=0 alt='%s'></a>", sys->ProgramName, db->db, tIndex[i].num, status_msg, fileicon, sys->IconWidth, sys->IconHeight, status_msg);
          break;
        }
      }
      printf("</td>\n");
    }

    /* Filename */
    if(!strcmp(conf->FieldFilename, "show")) {
      printf("    <td align=center>");
      if(!strcmp(head.Filename, "")) printf("<br>");
      else {
        sprintf(status_msg, "Download : %s (%d Kbytes) , Downloaded %d times", head.Filename, atoi(head.Filesize) / 1024, head.Download);
        if(!strcmp(conf->FieldSubject, "cut")) printf("<nobr>");
        printf("<a href='%s?db=%s&mode=download&num=%d' onMouseOver=\"window.status=('%s'); return true;\">", sys->ProgramName, db->db, tIndex[i].num, status_msg);
        printf("<font size='%s' color='%s' face='Arial'>", conf->FontSize, conf->ListArticleFontColor);
        highlightStr(11, head.Filename, db->fval);
        printf("</font>");
        printf("</a>");
        if(!strcmp(conf->FieldSubject, "cut")) printf("</nobr>");
      }
      printf("</font></td>\n");
    }

    /* Subject */
    printf("    <td align=left  >");
    if(!strcmp(conf->FieldSubject, "cut")) printf("<nobr>");

    /* ¡¶∏Ò ∂«¥¬ ∆˙¥ı æ’ø° Width (10 + Thread∞πºˆ * 40)¿Œ blank image∏¶ ª¿‘«—¥Ÿ */
    printf("<img src='%s/%s' width=%d height=1 alt='' border=0>", sys->HtmlBasePath, sys->IconBlank, 10 + (tIndex[i].depth * 40));

    /* ICON OPEN, CLOSE */
    if(!strcmp(conf->FolderIcon, "true")) {
      char *icon, *alt;
      if(tIndex[i+1].depth > tIndex[i].depth) {
        /* daysAgo «‘ºˆ¥¬ «ˆ¿Á≥Ø¬• - ∞‘Ω√π∞≥Ø¬•¿« ¿œºˆ∏¶ ∫π±Õ«—¥Ÿ. HighlightArticleø°¥¬ High Light«“ ¿œºˆ ¬˜∞°
           µÈæÓ¿÷¥¬µ• ¿Ã∫∏¥Ÿ ∞∞∞≈≥™ ¿€¿∏∏È HighLight ICON¿ª ∫∏ø©¡ÿ¥Ÿ. */
        if(atoi(conf->HighlightArticle) >= head.DaysAgo) icon = sys->IconOpenNew, alt = "+*"; /* High Light Article π¸¡÷ø° µÈæÓ∞°∏È */
        else icon = sys->IconOpen, alt = "+";
      }
      else {
        if(atoi(conf->HighlightArticle) >= head.DaysAgo) icon = sys->IconCloseNew, alt = "-*"; /* High Light Article π¸¡÷ø° µÈæÓ∞°∏È */
        else icon = sys->IconClose, alt = "-";
      }
      printf("<img src='%s/%s' width=%s height=%s alt='%s' border=0>", sys->HtmlBasePath, icon, sys->IconWidth, sys->IconHeight, alt);
      printf("<img src='%s/%s' width=10 height=1 alt='' border=0>", sys->HtmlBasePath, sys->IconBlank);
    }

    sprintf(status_msg, "Physical number %d , Virtual number %d, Included %d lines", head.Num, virtualnum, head.Lines);
    printf("<a href='%s?db=%s&mode=read&num=%d&page=%d&category=%s&ftype=%d&fval=%s&backdepth=1' onMouseOver=\"window.status=('%s'); return true;\">", sys->ProgramName, db->db, tIndex[i].num, db->page, db->category, db->ftype, db->fencval, status_msg);
    printf("<font size='%s' color='%s'>", conf->FontSize, conf->ListArticleFontColor);
    highlightStr(11, head.Subject, db->fval);
    printf("</font>");
    printf("</a>");

    if(!strcmp(conf->FieldSubject, "cut")) printf("</nobr>");
    printf("</td>\n");

    /* Name Second */
    if(!strcmp(conf->FieldName, "after")) {
      printf("    <td align=center><font size='%s' color='%s'>", conf->FontSize, conf->ListArticleFontColor);
      if(!strcmp(conf->FieldSubject, "cut")) printf("<nobr>");
      if(!strcmp(head.Email, "")) highlightStr(1, head.Name, db->fval);
      else {
        sprintf(status_msg, "Send mail to %s (%s)", head.Name, head.Email);
        printf("<a href='mailto:\"%s\" <%s>' onMouseOver=\"window.status=('%s'); return true;\">", head.Name, head.Email, status_msg);
        printf("<font color='%s'>", conf->ListArticleFontColor);
        highlightStr(1, head.Name, db->fval);
        printf("</font>");
        printf("</a>");
      }
      if(!strcmp(conf->FieldSubject, "cut")) printf("</nobr>");
      printf("</font></td>\n");
    }

    /* Category */
    if(categoryEnable == TRUE) {
      if((tmp = qsValue(catep, head.Category)) == NULL) tmp = NotSelected;
      printf("    <td align=center><font size='%s' color='%s'>%s</font></td>\n", conf->FontSize, conf->ListArticleFontColor, tmp);
    }

    /* Date */
    if(!strcmp(conf->FieldDate, "show")) {
      printf("    <td align=center><font size='%s' color='%s' face='Arial'>", conf->FontSize, conf->ListArticleFontColor);
      if(!strcmp(conf->FieldSubject, "cut")) printf("<nobr>");
      if(!strcmp(conf->Language, "english")) printf("%s %02d, %02d", abbMonth[head.Mon], head.Day, head.Year);
      else printf("%02d/%02d/%02d", head.Year, head.Mon, head.Day);
      if(!strcmp(conf->FieldSubject, "cut")) printf("</nobr>");
      printf("</font></td>\n");
    }

    /* Access */
    if(!strcmp(conf->FieldHits, "show")) {
      printf("    <td align=center><font size='%s' color='%s' face='Arial'>%d</font></td>\n", conf->FontSize, conf->ListArticleFontColor, head.Access);
    }

    /* Download */
    if(!strcmp(conf->FieldDownload, "show")) {
      printf("    <td align=center>");
      if(!strcmp(head.Filename, "")) printf("<br>");
      else {
      printf("<font size='%s' color='%s' face='Arial'>%d</font>", conf->FontSize, conf->ListArticleFontColor, head.Download);
      }
      printf("</td>\n");
    }

    printf("  </tr>\n");
    freeArticleInfo(&head);
  }
  printf("</table>\n");

  /* Page Link */
  printf("\n<!-- Page Link -->\n");
  printf("<table border=0 width=%s>\n", conf->TableWidth);
  printf("  <tr><td align=center><font size='%d' face='Arial'>\n", atoi(conf->FontSize) - 1 );

  if(startPage > 1) {
    strcpy(status_msg, "Jump to first page");
    printf("    <a href='%s?db=%s&mode=list&page=%d&category=%s&ftype=%d&fval=%s' onMouseOver=\"window.status=('%s'); return true;\">[1]</a>\n", sys->ProgramName, db->db, 1, db->category, db->ftype, db->fencval, status_msg);
    strcpy(status_msg, "Jump to previous page");
    printf("    <a href='%s?db=%s&mode=list&page=%d&category=%s&ftype=%d&fval=%s' onMouseOver=\"window.status=('%s'); return true;\">[Prev]</a> -\n", sys->ProgramName, db->db, startPage-1, db->category, db->ftype, db->fencval, status_msg);
  }
  for(i = startPage; (i <= endPage) && (i <= pages) && (i > 0); i++) {
    if(i == db->page) {
      printf("    [<font color=red>%d</font>]\n", i);
    }
    else {
      sprintf(status_msg, "Jump to page %d", i);
      printf("    <a href='%s?db=%s&mode=list&page=%d&category=%s&ftype=%d&fval=%s' onMouseOver=\"window.status=('%s'); return true;\">[%d]</a>\n", sys->ProgramName, db->db, i, db->category, db->ftype, db->fencval, status_msg, i);
    }
  }
  if(endPage < pages) {
    strcpy(status_msg, "Jump to next page");
    printf("  - <a href='%s?db=%s&page=%d&category=%s&ftype=%d&fval=%s' onMouseOver=\"window.status=('%s'); return true;\">[Next]</a>\n", sys->ProgramName, db->db, endPage+1, db->category, db->ftype, db->fencval, status_msg);
    if(endPage + 1 < pages) {
      strcpy(status_msg, "Jump to last page");
      printf("    <a href='%s?db=%s&page=%d&category=%s&ftype=%d&fval=%s' onMouseOver=\"window.status=('%s'); return true;\">[%d]</a>\n", sys->ProgramName, db->db, pages, db->category, db->ftype, db->fencval, status_msg, pages);
    }
  }
  printf("  </font></td></tr>\n");
  printf("</table>\n");

  printf("\n<!-- HR Line -->\n");
  printf("<table border=0 width=%s cellpadding=0 cellspacing=0>\n", conf->TableWidth);
  printf("  <tr><td><hr size=1 noshade></td></tr>\n");
  printf("</table>\n");

  /* ICON Link */
  if(!strcmp(conf->ListIconLocation, "bottom") || !strcmp(conf->ListIconLocation, "both")) {
    listIconLink(sys, db, conf, pages, "<br>");
  }

  tailHtml(sys, conf);
  qsFree(catep);
}

/* æ∆¿Ãƒ‹¿ª «•Ω√ «—¥Ÿ. ¿Ã∂ß searchBoundStr¿∫ ∞Àªˆ√¢¿« ∫–∏ÆπÆ¿⁄∏¶ º≥¡§«œø© ºˆ∆Ú, ºˆ¡˜ √º≈©π⁄Ω∫∏¶ 
** «•«ˆ«“ ºˆ ¿÷¿∏≥™, searchBoundStr¿Ã NULL¿œ ∞ÊøÏ ∞Àªˆ√¢¿ª «•Ω√«œ¡ˆ æ ¥¬¥Ÿ.
** ¡Ô æ∆¿Ãƒ‹ ∏µ≈©∞° both ¿œ ∞ÊøÏ ∏Ò∑œ¿« «œ¿ß æ∆¿Ãƒ‹ ∏µ≈©ø°∏∏ ∞Àªˆ√¢¿Ã ≥™≈∏≥™∞‘ «—¥Ÿ.*/
void listIconLink(SysConfig *sys, DB *db, DBConfig *conf, int pages, char *searchBoundStr) {
  char status_msg[0xff];

  printf("\n<!-- ICON Link -->\n");
  printf("<table border=0 width=%s><tr>\n", conf->TableWidth);
  printf("  <td align=left>\n");
  /* HomePage */
  if(strcmp(conf->HomeUrl, "")) {
    printf("    <a href='%s' target='%s'><img src='%s/%s' border=0 alt='Home'></a>\n", conf->HomeUrl, conf->HomeTarget, sys->HtmlBasePath, sys->IconHome);
  }
  /* BackPage */
  if(strcmp(conf->BackUrl, "")) {
    printf("    <a href='%s' target='%s'><img src='%s/%s' border=0 alt='Back'></a>\n", conf->BackUrl, conf->BackTarget, sys->HtmlBasePath, sys->IconBack);
  }
  /* Help */
  if(strcmp(sys->HiddenHelpLink, "true") != 0) {
    strcpy(status_msg, "Help message about this billboard");
    printf("    <a href='%s/%s' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Help'></a>\n", sys->HtmlBasePath, sys->Help, status_msg, sys->HtmlBasePath, sys->IconHelp);
  }
  printf("  </td>\n");

  /* Search */
  printf("\n<!-- Search -->\n");
  printf("  <td align=center valign=center>\n");
  if(!strcmp(conf->HideSearch, "true") || searchBoundStr == NULL) printf("<br>");
  else shotSearch(sys, db, conf, searchBoundStr);
  printf("  </td>\n");

  printf("\n<!-- ICON Link -->\n");
  printf("  <td align=right>\n");
  /* Up */
  if(db->page > 1) {
    strcpy(status_msg, "Previous page");
    printf("    <a href='%s?db=%s&mode=list&page=%d&category=%s&ftype=%d&fval=%s' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Backward'></a>\n", sys->ProgramName, db->db, db->page-1, db->category, db->ftype, db->fencval, status_msg, sys->HtmlBasePath, sys->IconUp);
  }
  /* Down */
  if(db->page < pages) {
    strcpy(status_msg, "Next page");
    printf("    <a href='%s?db=%s&mode=list&page=%d&category=%s&ftype=%d&fval=%s' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Forward'></a>\n", sys->ProgramName, db->db, db->page+1, db->category, db->ftype, db->fencval, status_msg, sys->HtmlBasePath, sys->IconDown);
  }
  /* Write */
  strcpy(status_msg, "Write a new message");
  printf("    <a href='%s?db=%s&mode=write&category=%s&backdepth=1' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Post'></a>\n", sys->ProgramName, db->db, db->category, status_msg, sys->HtmlBasePath, sys->IconWrite);
  /* Reload List */
  strcpy(status_msg, "Reload list");
  printf("    <a href='%s?db=%s' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Reload'></a>\n", sys->ProgramName, db->db, status_msg, sys->HtmlBasePath, sys->IconReload);

  printf("  </td>\n");
  printf("</tr></table>\n");
}

void shotSearch(SysConfig *sys, DB *db, DBConfig *conf, char *boundstr){
  char *Name, *Subject, *Text, *Search;
  char *checkflag;

  if(!strcmp(conf->Language, "korean")) {
    Name     = "<font face='±º∏≤'>º∫∏Ì</font>";
    Subject  = "<font face='±º∏≤'>¡¶∏Ò</font>";
    Text     = "<font face='±º∏≤'>∫ªπÆ</font>";
    Search   = "∞Àªˆ";
  }
  else if(!strcmp(conf->Language, "japanese")) {
    Name     = "ÉlÅ[ÉÄ";
    Subject  = "ëËÅ@ñ⁄";
    Text     = "ì‡Å@óe";
    Search   = "ÉSÅ[ÅI";
  }
  else {
    Name     = "<font face='Arial'>Name</font>";
    Subject  = "<font face='Arial'>Subject</font>";
    Text     = "<font face='Arial'>Body</font>";
    Search   = "Go!!";
  }

  /* Subject Text∞° ¿÷¿∏∏È ±◊∞Õ¿ª ªÁøÎ«—¥Ÿ */
  if(strcmp(conf->SubjectText, "")) Subject = conf->SubjectText;

  printf("<table border=0 cellpadding=0 cellspacing=0>\n");
  printf("<form method='post' action='%s'>\n", sys->ProgramName);
  printf("  <input type='hidden' name='db'   value='%s'>\n", db->db);
  printf("  <input type='hidden' name='mode' value='list'>\n");
  printf("  <input type='hidden' name='category' value='%s'>\n", db->category);

  printf("  <tr>\n");

  printf("    <td align=right><font size='%s'>\n", conf->FontSize);

  if((db->ftype & (int)FNAME)    != 0) checkflag = "checked";
  else                             checkflag = "";
  printf("      %s <input type='checkbox' name='fname' value='checked' %s>%s\n", Name, checkflag, boundstr);

  if((db->ftype & (int)FSUBJECT) != 0) checkflag = "checked";
  else                             checkflag = "";
  printf("      %s <input type='checkbox' name='fsubject' value='checked' %s>%s\n", Subject, checkflag, boundstr);

  if((db->ftype & (int)FTEXT)    != 0) checkflag = "checked";
  else                             checkflag = "";
  printf("      %s <input type='checkbox' name='ftext' value='checked' %s>%s\n", Text, checkflag, boundstr);
  printf("    </font></td>\n");


  printf("    <td align=left>\n");
  printf("      <font size=3><input type='text' name='fval' value='%s' size=14 maxlength=20></font>\n", db->fval);
  printf("      <font size=2><input type='submit' value='%s'></font>\n", Search);
  printf("    </td>\n");
  printf("  </tr>\n");
  printf("</form>\n");
  printf("</table>\n");
}

/* ∏Ò∑œ ∫∏±‚ø°º≠ »£√‚µ«¥¬ getArticleinfo()¥¬ ¿Œµ¶Ω∫ ≈◊¿Ã∫Ìø° ¿««ÿ ∞À¡ıµ» ¿⁄∑·∞° »£√‚µ«π«∑Œ,
«ÿ¥Á ±€¿∫ π›µÂΩ√ ¡∏¿Á «œø©æﬂ «—¥Ÿ. ∏∏æ‡ DB¿« ≥ªøÎ¿Ã ±˙æÓ¡Æ ¿÷¥ı∂Ûµµ ø¿∑˘∏¶ «•Ω√«œ¥¬ πÊπ˝∫∏¥‹,
∞¸∏Æ¿⁄≥™ ªÁøÎ¿⁄∞° «ÿ¥Á ±€¿ª ≈¨∏Ø «“ºˆ ¿÷µµ∑œ µ∆˙∆Æ ≈ÿΩ∫∆Æ∏¶ √‚∑¬«œø© ¡ÿ¥Ÿ.
π∞∑– ¿Ã∂ß Ω«¡¶ ±€¿ª ∫∏¥¬ «‡¿ß∏¶ «œ∏È 'Brocken or deleted article'¿Ã∂Û¥¬ ∏ﬁΩ√¡ˆ∏¶ ∫∏ø©¡÷∏Á 
«ÿ¥Á ∞‘Ω√π∞¿Ã ±˙æÓ¡≥¿Ω¿ª «•Ω√«—¥Ÿ. ±◊∑Ø≥™ ∞Ê«Ë¿˚¿∏∑Œ gdbm¿Ã ±˙æÓ¡ˆ¥¬ ¿œ¿∫ æ¯±‚ ∂ßπÆø°, ∫ª ±∏«ˆ¿∫ 
±∏πˆ¡Ø∞˙¿« »£»Øº∫(√ﬂ∞° ªË¡¶µ» « µÂ)¿ª ¡÷ ∏Ò¿˚¿∏∑Œ «—¥Ÿ. */
int getArticleInfo(int num, DBList *head){
  head->Num = num;

  /* Fetch */
  if((head->Name     = qRemoveSpace(DBFetch("%d.Name", num))) == NULL) head->Name = strdup("");
  if((head->Email    = qRemoveSpace(DBFetch("%d.Email", num))) == NULL) head->Email = strdup("");
  if((head->Subject  = qRemoveSpace(DBFetch("%d.Subject", num))) == NULL) head->Subject = strdup("");
  if((head->Category = DBFetch("%d.Category", num)) == NULL) head->Category = strdup("");
  if((head->Date     = DBFetch("%d.Date", num)) == NULL) head->Date = strdup("");
  if((head->Filename = DBFetch("%d.Filename", num)) == NULL) head->Filename = strdup("");
  if((head->Filesize = DBFetch("%d.Filesize", num)) == NULL) head->Filesize = strdup("");
  head->Download = DBiFetch("%d.Download", num);
  head->Lines    = DBiFetch("%d.Lines", num);
  head->Access   = DBiFetch("%d.Access", num);

  /* Adjust Empty Field */
  if(!strcmp(head->Name, "")) {
     free(head->Name);
     head->Name = strdup("(anonymous)");
  }
  if(!strcmp(head->Subject, "")) {
     free(head->Subject);
     head->Subject = strdup("(no subject)");
  }

  /* º∫∏Ì∂ı¿« quot ∏∂≈©∏¶ ºˆ¡§«—¥Ÿ, Javascript Error∏¶ ∏∑±‚¿ß«ÿ */
  exchangeQuot(head->Name);

  /* YYYYMMDD∏¶ ∫–∏Æ«—¥Ÿ */
  sscanf(head->Date, "%4d%2d%2d", &head->Year, &head->Mon, &head->Day);

  /* ±›¿œ∑Œ∫Œ≈Õ π◊¿œ¿¸¿« ∞‘Ω√π∞¿Œ¡ˆ ∆«∫∞«—¥Ÿ */
  head->DaysAgo = daysAgo(head->Year, head->Mon, head->Day);

  /* 4¿⁄∏Æ ≥‚µµø°º≠ æ’¿« 2¿⁄∏Æ∏¶ ¡¶∞≈«—¥Ÿ, 1998 -> 98 */
  for(; head->Year >= 100; head->Year -= 100);

  return TRUE;
}

void freeArticleInfo(DBList *head){
#define Free(NAME) if(head->NAME != NULL) free(head->NAME);
  Free(Name);
  Free(Email);
  Free(Subject);
  Free(Category);
  Free(Date);
  Free(Filename);
  Free(Filesize);
#undef Free
}

/* ∆ƒ¿œ∏Ì¿ª ¡÷∏È ±◊ø°«ÿ¥Á«œ¥¬ √‚∑¬«¸≈¬∏¶ ∫π±Õ«œ∏Á, ∆ƒ¿œø° «ÿ¥Á«œ¥¬ ICON¿ª ≥—∞‹¡ÿ¥Ÿ */
int findExtIcon(Entry *sysp, SysConfig *sys, char *filename, char *icon) {
  char ext[0xff], buf[0xff];
  int  i, finded;

  /* ∆ƒ¿œ∏Ìø°º≠ »Æ¿Â¿⁄∏∏¿ª ∫–∏Æ«‘ */
  for(strcpy(ext, filename), finded = FALSE, i = strlen(filename) - 1; i >= 0; i--) {
    if((ext[i] == '.') || (finded == TRUE)) {
      finded = TRUE;
      ext[i] = ' ';
    }
    qRemoveSpace(ext);
    qStrBig(ext); /* »Æ¿Â¿⁄∏¶ ¥ÎπÆ¿⁄∑Œ ∫Ø»Ø */
  }

  /* ∆ƒ¿œ∏Ì¿Ã æ¯¿Ω */
  if(strlen(filename) == 0) {
    sprintf(icon, "%s/%s", sys->HtmlBasePath, sys->IconTypeDefault);
    return TYPE_UNLINK;
  }

  /* Check general binary */
  sprintf(buf, "IconType-%s", ext);
  if(qfValue(sysp, buf) != NULL) {
    sprintf(icon, "%s/%s", sys->HtmlBasePath, qfValue(sysp, buf));
    return TYPE_LINK;
  }

  /* Check general text */
  sprintf(buf, "IconTypeText-%s", ext);
  if(qfValue(sysp, buf) != NULL) {
    sprintf(icon, "%s/%s", sys->HtmlBasePath, qfValue(sysp, buf));
    return TYPE_TEXT;
  }

  /* Check general image */
  sprintf(buf, "IconTypeImage-%s", ext);
  if(qfValue(sysp, buf) != NULL) {
    sprintf(icon, "%s/%s", sys->HtmlBasePath, qfValue(sysp, buf));
    return TYPE_IMAGE;
  }

  /* Check general sound */
  sprintf(buf, "IconTypeSound-%s", ext);
  if(qfValue(sysp, buf) != NULL) {
    sprintf(icon, "%s/%s", sys->HtmlBasePath, qfValue(sysp, buf));
    return TYPE_SOUND;
  }

  /* Check general movie */
  sprintf(buf, "IconTypeMovie-%s", ext);
  if(qfValue(sysp, buf) != NULL) {
    sprintf(icon, "%s/%s", sys->HtmlBasePath, qfValue(sysp, buf));
    return TYPE_MOVIE;
  }

  sprintf(icon, "%s/%s", sys->HtmlBasePath, sys->IconTypeUnknown);
  return TYPE_LINK;
}

void highlightStr(int mode, char *orgstr, char *orgtok) {
  char *str, *tok, *buf;
  char *freestr, *freetok;
  char *findp;
  int  i;

  if(!strcmp(orgtok, "")) {
    qPuts(mode, orgstr);
    return;
  }

  /* ¥ÎπÆ¿⁄∑Œ ∫Ø»Ø */
  if((str = freestr = strdup(orgstr)) == NULL) qError("Memory allocation fail.");
  if((tok = freetok = strdup(orgtok)) == NULL) qError("Memory allocation fail.");

  qStrBig(str), qStrBig(tok);

  /* √‚∑¬ πˆ∆€ */
  if((buf = (char *)malloc(strlen(orgstr) + 1)) == NULL) qError("Memory allocation fail.");

  /* ∫Ò±≥¥¬ ¥ÎπÆ¿⁄∑Œ ∫Ø»Øµ» ¿”Ω√ πÆ¿⁄ø≠∑Œ «œ∏Á, √‚∑¬¿∫ ø¯∫ª¿ª ªÁøÎ */
  while((findp = strstr(str, tok)) != NULL) {
    for(i = 0; str != findp; str++, orgstr++, i++) buf[i] = *orgstr;
    buf[i] = '\0';
    qPuts(mode, buf);

    for(i = 0; i < strlen(orgtok); str++, orgstr++, i++) buf[i] = *orgstr;
    buf[i] = '\0';
    printf("<font color=red>%s</font>", buf);
  }
  qPuts(mode, orgstr);

  free(buf);
  free(freestr), free(freetok);
}

/* ø¿¥√ ≥Ø¬•∏¶ ±‚¡ÿ¿∏∑Œ «ÿ¥Á ≥‚ ø˘ ¿œ∞˙¿« ¬˜∏¶ ∫π±Õ«—¥Ÿ */
int daysAgo(int year, int mon, int day) {
  struct tm *localtime;
  int days1, days2, diff;

  localtime = qGetTime();
  days1 = yearDays(localtime->tm_year, localtime->tm_mon, localtime->tm_mday);
  days2 = yearDays(year, mon, day);

  diff = days1 - days2;  /* ø¿¥√ - ∞‘Ω√π∞ */

  /* ∞‘Ω√π∞¿Ã ø¿¥√∫∏¥Ÿ ¿¸¿« ≥Ø¿⁄∏¶ ∞Æ∞Ì ¿÷¿Ω¿∫, Ω√Ω∫≈€ Ω√∞£¿Ã ∫Ø∞Êµ«æ˙¿Ω¿ª ¿«πÃ«—¥Ÿ */
  if(diff < 0) diff = 0;

  return diff;
}

/* «ÿ¥Á 1970≥‚ 1ø˘ 1¿œ ∫Œ≈Õ ¿Œ¿⁄∑Œ ¡÷æÓ¡¯ ≥‚, ø˘, ¿œ±Ó¡ˆ ¿±≥‚¿ª ∆˜«‘«— ¿œºˆ∏¶ ∫π±Õ«—¥Ÿ */
int yearDays(int year, int mon, int day) {
  int days[12+1] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int y, m, result;

  /* year - 1≥‚ ±Ó¡ˆ¿« ¿œºˆ∏¶ «’«—¥Ÿ */
  for(result = 0, y = 1970; y < year; y++) {
    if(leapYear(y) == TRUE) result += 366;
    else result += 365;
  }

  /* year≥‚µµ∞° ¿±≥‚¿Ã∏È days[2]∏¶ 29¿œ∑Œ «—¥Ÿ. */
  if(leapYear(year) == TRUE) days[2] = 29;

  /* year≥‚µµ¿« ¿œºˆ∏¶ «’«—¥Ÿ. */
  for(m = 1; m < mon; m++) result += days[m];

  result += day;

  return result;
}

/* ¿±≥‚¿Œ¡ˆ∏¶ ∆«∫∞«—¥Ÿ. ¿±≥‚ TRUE, ¿œπ› FALSE */
/* ¿±≥‚ : 1) 4∑Œ ≥™¥©æÓ ∂≥æÓ¡ˆ∏È ¿±≥‚¿Ã¥Ÿ. */
/*        2) 1)¡ﬂ 100¿∏∑Œ ≥™¥©æÓ ∂≥æÓ¡ˆ∏È ¿±≥‚¿Ã æ∆¥œ¥Ÿ. */
/*        3) 2)¡ﬂ 400¿∏∑Œ ≥™¥©æÓ ∂≥æÓ¡ˆ∏È ¿±≥‚¿Ã¥Ÿ. */
int leapYear(int year) {
  int mp;

  if(year % 4 == 0) {
    if(year % 100 == 0) {
      if(year % 400 == 0) mp = TRUE;
      else mp = FALSE;
    }
    else mp = TRUE;
  }
  else mp = FALSE;

  return mp;
}
