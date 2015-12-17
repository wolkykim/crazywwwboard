#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "cwb.h"
#include "cwb_admin.h"
#include "cwb_sync.h"

typedef struct R_INDEX {
  int num;
  int thread;
}R_INDEX;

R_INDEX *makeIndex(void);

void cwb_sync(SysConfig *sys, DB *db, DBConfig *conf){
  char buf[0xff];
  int  status;

  check_password(sys, db, conf);

  sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);

  status = DBReorganize(buf);

  if(status == FALSE) qError("Reorganization fail.");

  headHtml(sys, conf);
  printf("<p>\n");
  printf("<center><font size=3 face='Arial'>\n");
  printf("  Successfully reorganized.<p>");
  /* Reload List */
  printf("  <a href='%s?db=%s' onMouseOver=\"window.status=('Back to the list'); return true;\"><img src='%s/%s' border=0 alt='Reload'></a><p>\n", sys->ProgramName, db->db, sys->HtmlBasePath, sys->IconList);
  printf("</font></center>\n");
  tailHtml(sys, conf);
}
