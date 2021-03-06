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
#define STACK_SIZE	(100)  /* 한 게시물에 직렬로 존재할수 있는 쓰레드의 겟수 */
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

  /* 관리자만 목록을 볼수 있다면, 암호 검사 */
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

/* 게시물의 위아래 번호를 획득한다, 본함수는 목록 보기 모듈과 상관이 없으며
   cwb_read() 에서 호출된다. DB는 오픈되어 콜된다.*/
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
** DB를 열고 본함수를 호출하면, 모든 상황을 조합하여 
** 최종 리스트를 만들고, 그 겟수를 리턴한다.
** 리턴값은 패턴검색에 성공한 게시물의 갯수이며 
** 그것만이 리스트에 저장된다.
** 모든 인덱스 함수를 호출하는 메인이다.
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
** DB에서 목록을 만든다, 패턴검색하여 걸러진 게시물이 인덱싱된다.
** [0] -> 10, 0 (num, thread)
** [1] ->  9, 1
** [2] ->  8, 0
** [n] ->  0, 0
** 게시물의 갯수를 복귀한다. 에러시 -1
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

    /* Thread가 DB에 존재하지 않으면, 게시물이 삭제되었다고 가정한다 */
    if((str = DBFetch("%d.Thread", i)) != NULL) {  /* 게시물 존재 */
      totalArticle++;  /* 전체 개시물수 증가 */

      /* 카테고리 분류 */
      if(*category != '\0') {
        if((tmp = DBFetch("%d.Category", i)) == NULL) tmp = strdup("null"); /* Not selected 검색 */
        if(strcmp(tmp, category)) { free(tmp), free(str); continue; }
        free(tmp);
      }

      /* 패턴 검사 */
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
** index를 조정한다. 패턴검사를 마친 인덱스는 모쓰레드가 존재하지 않는 
** 게시물이 있을수 있다.
** 모 쓰레드가 인덱스에 없으면 참조번호를 0 으로 하여 Thread list order
** 일때 게시물이 확인되도록 한다.
**************************************/
void adjust_index(void) {
  int i, j;

  /* DB Fetch */
  for(i = 0; dIndex[i].num != 0; i++){
    if(dIndex[i].thread == 0) continue;

    /* 모 쓰레드가 존재하는지 검사 */
    for(j = i + 1; dIndex[j].num != 0; j++) {
      if(dIndex[j].num == dIndex[i].thread) break;
    }
    /* 참조 게시물 없음 */
    if(dIndex[j].num == 0) dIndex[i].thread = 0;
  }
}

/* 패턴검사하여 게시물이 합당한지 판별한다.*/
int findfval(int num, int ftype, char *fval) {
  char *name, *filename, *subject, *text;
  int  findflag;

  /* 검색문자열이 없을경우는 찾은걸로 간주 */
  if(!strcmp(fval, "") || ftype == 0) return TRUE;
  
  /* 초기화 */
  name = filename = subject = text = NULL;

  /* Fetch */
  if((ftype & FNAME)    != 0) {
    name     = DBFetch("%d.Name", num);
    filename = DBFetch("%d.Filename", num);
  }
  if((ftype & FSUBJECT) != 0) subject = DBFetch("%d.Subject", num);
  if((ftype & FTEXT)    != 0) text    = DBFetch("%d.Text", num);

  /* NULL이 아닌것을 찾는다*/
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
  int di;  /* dIndex의 index 값 */

  int threadNum;  /* 쓰레드 게시물의 엄마 게시물 번호 */
  int threadI;    /* 쓰레드 게시물의 dIndex 값 */
  int depth;
  int exit;       /* 루프 탈출 프레그 */

  for (di = 0; dIndex[di].num != 0; di++){
    /* thread가 0인 게시물을 찾는다. */
    if(dIndex[di].thread != 0) continue;

    /* 처리가 여기에 도달함은, 최상위 게시물을 찾았음을 의미한다 */

    /* tIndex에 depth 0으로 게시물을 추가한다. */
    insertIndex(dIndex[di].num, 0);

    /* 찾을 쓰레드의 엄마 num 값과, upper 값을 설정한다 */
    threadNum = dIndex[di].num, threadI = di;

    exit = FALSE;
    do {
      /* 쓰레드를 찾는다 */
      threadI = findThread(threadNum, threadI);

      if(threadI >= 0) { /* 쓰레드를 찾았음 */
        /* Stack에 쓰레드 엄마 num과 찾은 쓰레드의 index를 푸시한다 */
        depth = pushIndex(threadNum, threadI);

        /* 게시물 추가 */
        insertIndex(dIndex[threadI].num, depth);

        /* 다음번 찾을 쓰레드값을 설정한다 */
        threadNum = dIndex[threadI].num;
      }
      else { /* 쓰레드 못 찾았음 */
        if(popIndex(&threadNum, &threadI) == FALSE) exit = TRUE;
      }
    }while(exit == FALSE);
  }
  insertIndex(dIndex[di].num, 0);
}

/* num을 참조 하는 쓰레드 게시물을 dIndex[upper-1] 부터 찾는다 */
int findThread(int num, int upper){
  int i;

  for(i = upper - 1; i >= 0; i--){
    if(dIndex[i].thread == num) return i;
  }

  return -1;
}

/* tIndex에 게시물을 추가한다. 복귀값은 게시물의 현재 갯수 */
int insertIndex(int num, int depth) {
  static int Ip = 0;

  tIndex[Ip].num   = num;
  tIndex[Ip].depth = depth;
  Ip++;

  return Ip;
}

/* 쓰레드의 엄마 num과, 쓰레드 자신의 Index를 푸시한다 */
/* 복귀 값은 현제 스텍의 갯수 */
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

  char status_msg[0xff], *tmp; /* 스크린 하단의 상태바에 나타낼 메시지 */

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
    Number      = "<font face='굴림'>번호</font>";
    Type        = "<font face='굴림'>ⓒ</font>";
    Filename    = "<font face='굴림'>파일명</font>";
    Subject     = "<font face='굴림'>제 목</font>";
    Name        = "<font face='굴림'>성 명</font>";
    Category    = "<font face='굴림'>분 류</font>";
    NotSelected = "<font face='굴림'>기타</font>";
    Date        = "<font face='굴림'>작성일</font>";
    Access      = "<font face='굴림'>조회</font>"; 
    Download    = "<font face='굴림'>전송</font>"; 
    Articles    = "<font face='굴림'>검색된 게시물</font>";
    Pages       = "<font face='굴림'>쪽번호</font>";
  }
  else if(!strcmp(conf->Language, "japanese")) {
    Number      = "붥뜂";
    Type        = "(C)";
    Filename    = "긲�@귽깑뼹";
    Subject     = "묋�@뽞";
    Name        = "긨�[��";
    Category    = "긇긡긕깏";
    NotSelected = "긩�[�}깑";
    Date        = "볷븊";
    Access      = "긭긞긣"; 
    Download    = "�]몭";
    Articles    = "귺긂긣긵긞긣";
    Pages       = "긻�[긙";
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

  /* Subject Text가 있으면 그것을 사용한다 */
  if(strcmp(conf->SubjectText, "")) Subject = conf->SubjectText;

  /* 카테고리 사용여부 판별 & 카테고리 파싱 */
  catep = qsDecoder(conf->Category);
  if(!strcmp(conf->UseCategory, "true") && catep != NULL) {
    categoryEnable = TRUE;
    if((tmp = qsValue(catep, "_TITLE")) != NULL) Category = tmp;
    if((tmp = qsValue(catep, "_NOTSELECTED")) != NULL) NotSelected = tmp;
  }
  else categoryEnable = FALSE;

  /* Page 갯수 개산 */
  if(tArticle == 0) pages = 0;
  else {
    int pof;

    if((tArticle % atoi(conf->ArticlesPerPage)) == 0) pof = 0;
    else pof = 1;

    pages = (tArticle / atoi(conf->ArticlesPerPage)) + pof;
  }

  /* 링크 Page 범위 계산 */
  if(pages == 0) startPage = endPage = 0;
  else {
    int pof;

    if((db->page % atoi(sys->PageLinkNum)) == 0) pof = 0;
    else pof = 1;

    endPage   = ((db->page / atoi(sys->PageLinkNum)) + pof) * atoi(sys->PageLinkNum);
    startPage = endPage - (atoi(sys->PageLinkNum) - 1);
  }

  /* 보여줄 페이지 교정 */
  if(db->page > pages) db->page = pages;

  /* 찍을 Index 번호 계산 */
  if(db->page == 0) startNum = endNum = 0;
  else {
    startNum = (db->page - 1) * atoi(conf->ArticlesPerPage);
    endNum   = startNum + (atoi(conf->ArticlesPerPage) - 1 );
  }

  /* Header 출력 */
  headHtml(sys, conf);

  /* ICON Link */
  if(!strcmp(conf->ListIconLocation, "top") || !strcmp(conf->ListIconLocation, "both")) {
    char *boundStr;

    if(!strcmp(conf->ListIconLocation, "both")) boundStr = NULL; /* 검색창 표시 안함 */
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

  /* 목록보기 방식이 thread일때에는 관련글들때문에 목록의 게시물 번호가
     엉켜있다. 따라서 이때에는 전체계시물에 대한 상대번호를 계산하여 
     일률적으로 보여줄수있도록 Virtual Num 옵션을 주었다.
     이 방식의 단점은 하위 게시물이 삭제되거나, 하위에 관련글이 작성되면 
     게시물의 번호가 가변적으로 변한다는 것이다.
     그러나, 관련글일때 게시물번호를 생략하는 방식또한 이빨이 빠진듯하여 
     외관상 별로 안좋다.
     관리모드에서 사용자가 이를 선택하여, 물리적 번호를 표시할지 가상번호를 
     표시할지 선택토록한다. */

  virtualnum = tArticle - ((db->page - 1) * atoi(conf->ArticlesPerPage));

  /* tIndex 목록의 끝엔 0 필드가 삽입되어 있다 */
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
        default : { /* 업로딩 파일이 있음 */
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

    /* 제목 또는 폴더 앞에 Width (10 + Thread갯수 * 40)인 blank image를 삽입한다 */
    printf("<img src='%s/%s' width=%d height=1 alt='' border=0>", sys->HtmlBasePath, sys->IconBlank, 10 + (tIndex[i].depth * 40));

    /* ICON OPEN, CLOSE */
    if(!strcmp(conf->FolderIcon, "true")) {
      char *icon, *alt;
      if(tIndex[i+1].depth > tIndex[i].depth) {
        /* daysAgo 함수는 현재날짜 - 게시물날짜의 일수를 복귀한다. HighlightArticle에는 High Light할 일수 차가
           들어있는데 이보다 같거나 작으면 HighLight ICON을 보여준다. */
        if(atoi(conf->HighlightArticle) >= head.DaysAgo) icon = sys->IconOpenNew, alt = "+*"; /* High Light Article 범주에 들어가면 */
        else icon = sys->IconOpen, alt = "+";
      }
      else {
        if(atoi(conf->HighlightArticle) >= head.DaysAgo) icon = sys->IconCloseNew, alt = "-*"; /* High Light Article 범주에 들어가면 */
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

/* 아이콘을 표시 한다. 이때 searchBoundStr은 검색창의 분리문자를 설정하여 수평, 수직 체크박스를 
** 표현할 수 있으나, searchBoundStr이 NULL일 경우 검색창을 표시하지 않는다.
** 즉 아이콘 링크가 both 일 경우 목록의 하위 아이콘 링크에만 검색창이 나타나게 한다.*/
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
    Name     = "<font face='굴림'>성명</font>";
    Subject  = "<font face='굴림'>제목</font>";
    Text     = "<font face='굴림'>본문</font>";
    Search   = "검색";
  }
  else if(!strcmp(conf->Language, "japanese")) {
    Name     = "긨�[��";
    Subject  = "묋�@뽞";
    Text     = "볙�@뾢";
    Search   = "긕�[갏";
  }
  else {
    Name     = "<font face='Arial'>Name</font>";
    Subject  = "<font face='Arial'>Subject</font>";
    Text     = "<font face='Arial'>Body</font>";
    Search   = "Go!!";
  }

  /* Subject Text가 있으면 그것을 사용한다 */
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

/* 목록 보기에서 호출되는 getArticleinfo()는 인덱스 테이블에 의해 검증된 자료가 호출되므로,
해당 글은 반드시 존재 하여야 한다. 만약 DB의 내용이 깨어져 있더라도 오류를 표시하는 방법보단,
관리자나 사용자가 해당 글을 클릭 할수 있도록 디폴트 텍스트를 출력하여 준다.
물론 이때 실제 글을 보는 행위를 하면 'Brocken or deleted article'이라는 메시지를 보여주며 
해당 게시물이 깨어졌음을 표시한다. 그러나 경험적으로 gdbm이 깨어지는 일은 없기 때문에, 본 구현은 
구버젼과의 호환성(추가 삭제된 필드)을 주 목적으로 한다. */
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

  /* 성명란의 quot 마크를 수정한다, Javascript Error를 막기위해 */
  exchangeQuot(head->Name);

  /* YYYYMMDD를 분리한다 */
  sscanf(head->Date, "%4d%2d%2d", &head->Year, &head->Mon, &head->Day);

  /* 금일로부터 및일전의 게시물인지 판별한다 */
  head->DaysAgo = daysAgo(head->Year, head->Mon, head->Day);

  /* 4자리 년도에서 앞의 2자리를 제거한다, 1998 -> 98 */
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

/* 파일명을 주면 그에해당하는 출력형태를 복귀하며, 파일에 해당하는 ICON을 넘겨준다 */
int findExtIcon(Entry *sysp, SysConfig *sys, char *filename, char *icon) {
  char ext[0xff], buf[0xff];
  int  i, finded;

  /* 파일명에서 확장자만을 분리함 */
  for(strcpy(ext, filename), finded = FALSE, i = strlen(filename) - 1; i >= 0; i--) {
    if((ext[i] == '.') || (finded == TRUE)) {
      finded = TRUE;
      ext[i] = ' ';
    }
    qRemoveSpace(ext);
    qStrBig(ext); /* 확장자를 대문자로 변환 */
  }

  /* 파일명이 없음 */
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

  /* 대문자로 변환 */
  if((str = freestr = strdup(orgstr)) == NULL) qError("Memory allocation fail.");
  if((tok = freetok = strdup(orgtok)) == NULL) qError("Memory allocation fail.");

  qStrBig(str), qStrBig(tok);

  /* 출력 버퍼 */
  if((buf = (char *)malloc(strlen(orgstr) + 1)) == NULL) qError("Memory allocation fail.");

  /* 비교는 대문자로 변환된 임시 문자열로 하며, 출력은 원본을 사용 */
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

/* 오늘 날짜를 기준으로 해당 년 월 일과의 차를 복귀한다 */
int daysAgo(int year, int mon, int day) {
  struct tm *localtime;
  int days1, days2, diff;

  localtime = qGetTime();
  days1 = yearDays(localtime->tm_year, localtime->tm_mon, localtime->tm_mday);
  days2 = yearDays(year, mon, day);

  diff = days1 - days2;  /* 오늘 - 게시물 */

  /* 게시물이 오늘보다 전의 날자를 갖고 있음은, 시스템 시간이 변경되었음을 의미한다 */
  if(diff < 0) diff = 0;

  return diff;
}

/* 해당 1970년 1월 1일 부터 인자로 주어진 년, 월, 일까지 윤년을 포함한 일수를 복귀한다 */
int yearDays(int year, int mon, int day) {
  int days[12+1] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int y, m, result;

  /* year - 1년 까지의 일수를 합한다 */
  for(result = 0, y = 1970; y < year; y++) {
    if(leapYear(y) == TRUE) result += 366;
    else result += 365;
  }

  /* year년도가 윤년이면 days[2]를 29일로 한다. */
  if(leapYear(year) == TRUE) days[2] = 29;

  /* year년도의 일수를 합한다. */
  for(m = 1; m < mon; m++) result += days[m];

  result += day;

  return result;
}

/* 윤년인지를 판별한다. 윤년 TRUE, 일반 FALSE */
/* 윤년 : 1) 4로 나누어 떨어지면 윤년이다. */
/*        2) 1)중 100으로 나누어 떨어지면 윤년이 아니다. */
/*        3) 2)중 400으로 나누어 떨어지면 윤년이다. */
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
