#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "cwb.h"
#include "cwb_admin.h"
#include "cwb_delete.h"

void cwb_delete(SysConfig *sys, DB *db, DBConfig *conf){
  char dbPath[0xFF], uploadDir[0xff];
  
  if(db->num == 0) qError("Handling number must be specified.");

  check_password(sys, db, conf);

  sprintf(dbPath,    "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
  sprintf(uploadDir, "%s/%s/%s",    sys->RealBasePath, sys->DataDirectory, db->db);

  DBOpen(dbPath, GDBM_REPLACE);

  if(deleteArticle(db->num, uploadDir) == FALSE) qError("Article not found.");

  DBClose();
}

void cwb_batchdelete(SysConfig *sys, DB *db, DBConfig *conf){
  char dbPath[0xFF], uploadDir[0xff];
  int  num1, num2, i, flag, deleted;
  char *sync;

  check_password(sys, db, conf);

  /* Query Fetch */
  num1 = qiValue("dNum1");
  num2 = qiValue("dNum2");
  if((sync = qValue("dSync")) == NULL) sync = "";

  if((num1 == 0 || num2 == 0) || (num1 > num2) || (num1 <= 0) || (num2 > conf->LastNum)) qError("Out of range. (1-%d available)", conf->LastNum);

  sprintf(dbPath,    "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
  sprintf(uploadDir, "%s/%s/%s",    sys->RealBasePath, sys->DataDirectory, db->db);

  headHtml(sys, conf);

  printf("\n<!-- HR Line -->\n");
  printf("<table border=0 width=%s>\n", conf->TableWidth);
  printf("  <tr><td><hr size=1 noshade></td></tr>\n");
  printf("</table>\n");

  printf("\n<!-- Body -->\n");
  printf("<table border=0 cellspacing=1 width=%s>\n", conf->TableWidth);
  printf("  <tr bgcolor='%s'>\n", conf->ListTitleBgColor);
  printf("    <th><font size='%s' color='%s'>Batch-delete: %d - %d</font></th>\n", conf->FontSize, conf->ListTitleFontColor, num1, num2);
  printf("  </tr>\n\n");
  printf("</table>\n");

  printf("\n<!-- Message -->\n");
  printf("<table border=0 cellspacing=1 width=%s>\n", conf->TableWidth);
  printf("  <tr bgcolor='%s'>\n", conf->ListArticleBgColor);
  printf("    <td><font size='%s' color='%s'>\n", conf->FontSize, conf->ListArticleFontColor);

  DBOpen(dbPath, GDBM_REPLACE);
  printf("<font color='red'>\n");
  for(i = num2, deleted = 0; i >= num1; i--) {
    flag = deleteArticle(i, uploadDir);
    if(flag) {
      deleted++;
      printf("%d ", i);
    }
  }
  printf("</font><br>\n");
  printf("Succefully deleted %d articles.<p>\n", deleted);

  DBClose();

  if(!strcmp(sync, "true")) {
    DBReorganize(dbPath);
    printf("Database reorganized.\n");
    free(sync);
  }

  printf("    </td>\n");
  printf("  </tr>\n");
  printf("</table>\n");

  printf("\n<!-- HR Line -->\n");
  printf("<table border=0 width=%s>\n", conf->TableWidth);
  printf("  <tr><td><hr size=1 noshade></td></tr>\n");
  printf("</table>\n");

  printf("<table border=0 width=%s>\n", conf->TableWidth);
  printf("  <tr><td align=right>\n");
  printf("      <a href='%s?db=%s' onMouseOver=\"window.status=('Back to the list'); return true;\"><img src='%s/%s' border=0 alt='List'></a>\n", sys->ProgramName, db->db, sys->HtmlBasePath, sys->IconList);
  printf("      </td>\n");
  printf("  </tr>\n");
  printf("</table>\n");

  tailHtml(sys, conf);
}

int deleteArticle(int num, char *uploadDir) {
  int newThread, lastNum, i;
  char *filename, buf[0xff];

  /* 게시물이 존재 확인 */
  if(DBFetch("%d.Thread", num) == NULL) return FALSE;

  /* 게시물의 Thread번호 획득 */
  newThread = DBiFetch("%d.Thread", num);

  /* 단일 접근하에서 마지막 번호를 다시한번 확인한다 */
  lastNum = DBiFetch("conf.LastNum");   

  /* 게시물에 관련된글을 게시물의 Thread번호로 바꾼다 */
  for(i = num + 1; i <= lastNum; i++) {
    if(DBiFetch("%d.Thread", i) == num) {
      sprintf(buf, "%d.Thread", i);
      DBiStore(buf, newThread, GDBM_REPLACE);
    }
  }

  /* 업로딩된 파일명 획득 */
  filename = DBFetch("%d.Filename", num);

  /* 게시물 삭제 */
  DBDelete("%d.Name",    num);
  DBDelete("%d.Email",   num);
  DBDelete("%d.Subject", num);
  DBDelete("%d.Category", num);
  DBDelete("%d.Text",    num);

  DBDelete("%d.Filename", num);
  DBDelete("%d.Filesize", num);

  DBDelete("%d.Date",   num);
  DBDelete("%d.Domain", num);

  DBDelete("%d.Lines",  num);
  DBDelete("%d.Access", num);
  DBDelete("%d.Thread", num);

  DBDelete("%d.Password", num);

  /* 업로드 파일 삭제 */
  if(filename != NULL) {
    if(strcmp(filename, "") != 0) {
      sprintf(buf, "%s/%s", uploadDir, filename);
      unlink(buf);
    }
  }

  return TRUE;
}
