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

  /* 호스트명 기록 여부 해석*/
  if((DisplayAddress = DBFetch("conf.DisplayAddress")) == NULL) DisplayAddress = "true";
  if(!strcmp(DisplayAddress, "false")) strcpy(article->Domain, "");

  /* DB의 단일접근이 보장되었다, 마지막 게시물 번호를 가져온다 */
  lastnum = DBiFetch("conf.LastNum");
  lastnum++;

  StoreStr(Name);
  StoreStr(Email);
  StoreStr(Subject);
  StoreStr(Text);

/* 본 버젼에서 어태치 파일은 해석하지 못하기 때문에, 본 필드는 저장하지 않는다. */
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
