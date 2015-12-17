#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "arc.h"

void arc_write(SysConfig *sys, DBArticle *article, char *db){
  int lastnum;
  char buf[0xff], *DisplayAddress;

#define StoreStr(NAME) sprintf(buf, "%d." #NAME, lastnum); DBStore(buf, article->NAME, GDBM_REPLACE)
#define StoreInt(NAME) sprintf(buf, "%d." #NAME, lastnum); DBiStore(buf, article->NAME, GDBM_REPLACE)

  /* Insert into DB */
  sprintf(buf, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db, sys->DataExtension);
  DBOpen(buf, GDBM_WRITER);

  /* ȣ��Ʈ�� ��� ���� �ؼ�*/
  if((DisplayAddress = DBFetch("conf.DisplayAddress")) == NULL) DisplayAddress = "true";
  if(!strcmp(DisplayAddress, "false")) strcpy(article->Domain, "");

  /* DB�� ���������� ����Ǿ���, ������ �Խù� ��ȣ�� �����´� */
  lastnum = DBiFetch("conf.LastNum");
  lastnum++;

  StoreStr(Name);
  StoreStr(Email);
  StoreStr(Subject);
  StoreStr(Text);

/* �� �������� ����ġ ������ �ؼ����� ���ϱ� ������, �� �ʵ�� �������� �ʴ´�. */
/*
  StoreStr(Filename);
  StoreStr(Filesize);
*/

  StoreStr(Date);
  StoreStr(Domain);

  StoreInt(Lines);
  StoreInt(Access);
  StoreInt(Thread);

  StoreStr(Password);

  DBiStore("conf.LastNum", lastnum, GDBM_REPLACE);

  DBClose();

  free(article->Text);

#undef StoreStr
#undef StoreInt
}
