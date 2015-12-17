#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "cwb.h"
#include "cwb_admin.h"

int  analysisLog(char *access_log, Stat *stat);
void displayLog(SysConfig *sys,  DB *db, Stat *stat, char *width, int fontsize);

void cwb_stat(SysConfig *sys, DB *db, DBConfig *conf) {
  Stat stat;
  char *date, buf[0xff];

  if(!strcmp(conf->StatMode, "admin")) check_password(sys, db, conf);

  if((date = qValue("date")) == NULL) {  /* 날짜가 지정되지 않았다면 이번 달로 설정한다. */
    struct tm *currentTime;

    currentTime = qGetTime();
    stat.Year = currentTime->tm_year;
    stat.Mon  = currentTime->tm_mon;
  }
  else {
    /* date string에는 YYYYMMDD가 있지만 DD는 다음 버젼을 위해 예약만 해둔다 */
    sscanf(date, "%4d%2d", &stat.Year, &stat.Mon);
  }

  sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->StatDirectory, db->db, sys->StatExtension);
  analysisLog(buf, &stat);

  headHtml(sys, conf);
  printf("<br>\n");
  displayLog(sys, db, &stat, conf->TableWidth, atoi(conf->FontSize));
  tailHtml(sys, conf);
}

/* 로그 파일을 열어 해당 달(Stat 구조체에 넣어서 전달)의 로그를 분석한다.
   복귀값으로는 로그 파일에 기록된 로그의 총 갯수를 리턴한다. */
int analysisLog(char *dbpath, Stat *stat) {
  struct tm *time;
  int  year, mon, day;
  int  i, j, stats;
  char runDate[8+1], eDate[8+1], *tmp;

  if(qCheckFile(dbpath) == FALSE) qError("Statistic logging is disabled.");

  DBOpen(dbpath, GDBM_READER);

  /* 구조체 초기화 */
  for(i = 0; i <= 31; i++) stat->Day[i] = 0;

  /* 로그 시작일 해석 */
  if((tmp = DBFetch("stat.Started")) == NULL) qError("Statistic logging is disabled.");
  sscanf(tmp, "%04d%02d%02d", &stat->sYear, &stat->sMon, &stat->sDay);
  free(tmp);

  /* 로그 종료일 해석, 금일로 */
  time = qGetTime();
  /* 로그 종료일 기록 */
  stat->eYear = time->tm_year;
  stat->eMon  = time->tm_mon;
  stat->eDay  = time->tm_mday;
  sprintf(eDate, "%04d%02d%02d", stat->eYear, stat->eMon, stat->eDay);

  /* 로그 분석 */
  stat->TotalAccess = 0;
  
  for(year = stat->sYear, mon = stat->sMon, day = stat->sDay; ; day++) {
    if(day == 32) mon++, day = 1;
    if(mon == 13) year++, mon = 1;
    sprintf(runDate, "%04d%02d%02d", year, mon, day);
    if(strcmp(runDate, eDate) > 0) break;

    stats = DBiFetch("stat.%s", runDate);
    stat->TotalAccess += stats;

    /* 해당 달일경우 */
    if(year == stat->Year && mon == stat->Mon) stat->Day[day] = stats;
  }

  /* 달 총 평균, 최고 접근횟수 계산 */
  for(i = 1, j = 0, stat->Total = stat->Avg = stat->Max = 0; i <= 31; i++) {
    if(stat->Day[i] > stat->Max) stat->Max = stat->Day[i];

    if(stat->Day[i] > 0) { 
      stat->Total += stat->Day[i];
      j++; /* 억세스가 0이 아닌 달의 갯수 */
    }
  }
  if(j > 0) stat->Avg = stat->Total / j;
  else stat->Avg = 0;

  DBClose();
  return TRUE;
}

/* 막대그래프 로그를 그린다. */
void displayLog(SysConfig *sys,  DB *db, Stat *stat, char *width, int fontsize) {
  int  height, percent;
  char *barimg, status_msg[0xff];
  int i, j;
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

  printf("<table border=0 width=%s>\n", width);

  printf("\n<!-- Title -->\n");
  printf("  <tr>\n");
  printf("    <th colspan=2><hr size=5><font size='%d'><font size='%d'>%s</font>, %04d statistics</font><hr size=5></th>\n", fontsize + 2, fontsize + 3, month[stat->Mon], stat->Year);
  printf("  </tr>\n");

  printf("\n<!-- Status Line -->\n");
  printf("  <tr>\n");
  printf("    <td align=left valign=top>\n");
  printf("      <font size='%d' face='Arial'><i>\n", fontsize);
  printf("      Period from <font color=darkgreen>%s, %d</font> to <font color=darkgreen>%s, %d</font><br>\n", month[stat->sMon], stat->sYear, month[stat->eMon], stat->eYear);
  printf("      This month accesses : %d times<br>\n", stat->Total);
  printf("      Total accesses : %d times<br>\n", stat->TotalAccess);
  printf("      </i></font>\n");
  printf("    </td>\n");

  printf("\n<!-- Bar Graph Color Explanation -->\n");
  printf("    <td align=left>\n");
  printf("      <font size='%d' face='Arial'>\n", fontsize);
  printf("      <font color=gray>-</font> Max Hits / Day  : %d<br>", stat->Max);
  printf("      <font color=gray>-</font> Avg Hits / Day  : %d<br>", stat->Avg);
  printf("      <font color=gray>-</font> Avg Hits / Hour : %d<br>", stat->Avg / 24);
  printf("      </font>\n");
  printf("    </td>\n");
  printf("  </tr>\n");

  printf("  <tr>\n");
  printf("    <td align=left valign=bottom>\n");
  printf("    <table border=1 cellpadding=2 cellspacing=0 height=%s><tr><td bgcolor=f0f5fe valign=bottom>\n", sys->StatBarMaxHeight);
  printf("      <font size='%d'>\n", fontsize);
  for(i = 1; i <= 31; i++) {
    if(stat->Max != 0) {
      height  = (stat->Day[i] * atoi(sys->StatBarMaxHeight)) / stat->Max;
      percent = (stat->Day[i] * 100) / stat->Max;
    }
    else {
      height = 0;
      percent = 0;
    }
    if     (percent >= 90) barimg = sys->StatBar90;
    else if(percent >= 80) barimg = sys->StatBar80;
    else if(percent >= 70) barimg = sys->StatBar70;
    else if(percent >= 60) barimg = sys->StatBar60;
    else if(percent >= 50) barimg = sys->StatBar50;
    else if(percent >= 40) barimg = sys->StatBar40;
    else if(percent >= 30) barimg = sys->StatBar30;
    else if(percent >= 20) barimg = sys->StatBar20;
    else if(percent >= 10) barimg = sys->StatBar10;
    else                   barimg = sys->StatBar00;
    printf("<img src='%s/%s' border=1 width=%d height=%d alt='%d hits'>", sys->HtmlBasePath, barimg, atoi(sys->StatBarWidth) - 2, height, stat->Day[i]);
  }
  printf("<br>\n");
  printf("      <img src='%s/%s' border=0 width=%d><br>\n", sys->HtmlBasePath, sys->StatBarUnder, atoi(sys->StatBarWidth) * 31);
  printf("      </font>\n");
  printf("      </td></tr></table>\n");
  printf("    </td>\n");

  printf("\n<!-- Bar Graph Color Explanation -->\n");
  printf("    <td align=left>\n");
  printf("      <font size='%d' face='Arial'>\n", fontsize);
  for(i = 90; i >= 0; i -= 10) {
    if     (i == 90) barimg = sys->StatBar90;
    else if(i == 80) barimg = sys->StatBar80;
    else if(i == 70) barimg = sys->StatBar70;
    else if(i == 60) barimg = sys->StatBar60;
    else if(i == 50) barimg = sys->StatBar50;
    else if(i == 40) barimg = sys->StatBar40;
    else if(i == 30) barimg = sys->StatBar30;
    else if(i == 20) barimg = sys->StatBar20;
    else if(i == 10) barimg = sys->StatBar10;
    else             barimg = sys->StatBar00;
    printf("      <img src='%s/%s' border=1 width=%d height=%d> ", sys->HtmlBasePath, barimg, atoi(sys->StatBarWidth) - 2, atoi(sys->StatBarWidth) - 2);
    printf("&gt; %02d %% : %d hits<br>\n", i, (int)((float)stat->Max * ((float)i / (float)100)));
  }
  printf("      </font>\n");
  printf("    </td>\n");
  printf("  </tr>\n");

  printf("\n<!-- HR Line -->\n");
  printf("  <tr>\n");
  printf("    <td colspan=2>\n");
  printf("      <hr size=1 noshade>\n");
  printf("    </td>\n");
  printf("  </tr>\n");

  printf("\n<!-- Analysis List Box & Link Icon-->\n");
  printf("  <tr>\n");
  printf("    <td align=left>\n");
  printf("      <form>\n");
  printf("        <select name='date' onChange=\"window.location='%s?db=%s&mode=stat&date='+form.date.options[form.date.selectedIndex].value\">\n", sys->ProgramName, db->db);
  for(i = stat->sYear; i <= stat->eYear; i++) {
    char *selected;

    for(j = 1; j <= 12; j++) {
      /* 로그 시작보다 월이 작으면 Skip */
      if(i == stat->sYear && j < stat->sMon) continue;
      /* 정상 처리 */
      if(i == stat->Year && j == stat->Mon) selected = "selected";
      else selected = "";
      printf("        <option value='%04d%02d%02d' %s>%04d, %s</option>\n", i, j, 0, selected, i, month[j]);

      /* 로그 종료 년월 보다 루프가 커지면 블럭 탈출 */
      if(i >= stat->eYear && j >= stat->eMon) break; 
    }
  }
  printf("        </select>\n");
  printf("      </form>\n");
  printf("    </td>\n");

  printf("    <td align=right>\n");
  /* Up */
  if((stat->sYear == stat->Year && stat->sMon < stat->Mon) || (stat->sYear < stat->Year)) {
    int tYear = stat->Year;
    int tMon  = stat->Mon - 1;

    if(tMon == 0) tYear--, tMon = 12;

    sprintf(status_msg, "Analysis previous month : %s, %04d", month[tMon], tYear);
    printf("    <a href='%s?db=%s&mode=stat&date=%04d%02d%02d' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Backward'></a>\n", sys->ProgramName, db->db, tYear, tMon, 0 , status_msg, sys->HtmlBasePath, sys->IconUp);
  }
  /* Down */
  if((stat->eYear == stat->Year && stat->eMon > stat->Mon) || (stat->eYear > stat->Year)) {
    int tYear = stat->Year;
    int tMon  = stat->Mon + 1;

    if(tMon == 13) tYear++, tMon = 1;

    sprintf(status_msg, "Analysis next month : %s, %04d", month[tMon], tYear);
    printf("    <a href='%s?db=%s&mode=stat&date=%04d%02d%02d' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='Backward'></a>\n", sys->ProgramName, db->db, tYear, tMon, 0 , status_msg, sys->HtmlBasePath, sys->IconDown);
  }
  /* List */
  strcpy(status_msg, "Back to the list");
  printf("      <a href='%s?db=%s' onMouseOver=\"window.status=('%s'); return true;\"><img src='%s/%s' border=0 alt='List'></a>\n", sys->ProgramName, db->db, status_msg, sys->HtmlBasePath, sys->IconList);

  printf("    </td>\n");
  printf("  </tr>\n");

  printf("</table>\n");
}
