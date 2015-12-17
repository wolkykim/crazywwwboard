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

  /* �����ڸ� �Խù��� ������ �ִٸ�, ��ȣ �˻� */
  if(!strcmp(conf->ReadMode, "admin")) check_password(sys, db, conf);

  /* DB�� ����� ���� ���丮 ��θ� �����Ѵ� */
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
