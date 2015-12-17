#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "cwb.h"
#include "cwb_admin.h"
#include "cwb_download.h"

void cwb_download(SysConfig *sys, DB *db, DBConfig *conf){
  char dbpath[0xff], filepath[0xff], *filename, buf[0xff];
  int download;

  if(db->num == 0) qError("Do not use manually.");

  /* 관리자만 게시물을 읽을수 있다면, 암호 검사 */
  if(!strcmp(conf->ReadMode, "admin")) check_password(sys, db, conf);

  /* DB가 저장된 실제 디렉토리 경로를 저장한다 */
  sprintf(dbpath, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);
  DBOpen(dbpath, GDBM_WRITER);
  filename = DBFetch("%d.Filename", db->num);
  if(filename == NULL)      { DBClose(); qError("Do not use manually."); }
  if(!strcmp(filename, "")) { DBClose(); qError("There is no attached file in a article number %d.", db->num); }
  sprintf(filepath, "%s/%s/%s/%s", sys->RealBasePath, sys->DataDirectory, db->db, filename);
  free(filename);

  sprintf(buf, "%d.Download", db->num);
  download = DBiFetch(buf);
  DBiStore(buf, ++download, GDBM_REPLACE);

  DBClose();

  qDownload(filepath);
}
