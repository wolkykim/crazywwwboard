#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "adm.h"
#include "adm_remove.h"

void adm_rm_log(SysConfig *sys, DB *db) {
  char access_log[0xff], error_log[0xff];

  sprintf(access_log, "%s/%s/access_log.%s", sys->RealBasePath, sys->LogDirectory, db->db);
  sprintf(error_log,  "%s/%s/error_log.%s",  sys->RealBasePath, sys->LogDirectory, db->db);

  if(qCheckFile(access_log) == TRUE) {
    if(unlink(access_log) != 0) qError("Access-log(%s) delete fail.", access_log);
  }
  if(qCheckFile(error_log) == TRUE) {
    if(unlink(error_log)  != 0) qError("Error-log(%s) delete fail.", error_log);
  }
}

void adm_rm_stat(SysConfig *sys, DB *db) {
  char statpath[0xff];

  sprintf(statpath, "%s/%s/%s.%s", sys->RealBasePath, sys->StatDirectory, db->db, sys->StatExtension);

  if(qCheckFile(statpath) == TRUE) {
    if(unlink(statpath) != 0) qError("Statistics(%s) delete fail.", statpath);
  }

}

void adm_rm_db(SysConfig *sys, DB *db) {
  char dbpath[0xff], buf[0xff];

  sprintf(dbpath, "%s/%s/%s.%s", sys->RealBasePath, sys->DataDirectory, db->db, sys->DataExtension);

  /* 로그 삭제 */
  adm_rm_log(sys, db);

  /* 통계 삭제 */
  adm_rm_stat(sys, db);

  /* db 삭제 */
  if(qCheckFile(dbpath) == TRUE) {
    if(unlink(dbpath) != 0) qError("Database(%s) delete fail.", dbpath);
  }
  else qError("There is no database named %s(%s).", db->db, dbpath);

  /* 업로드 디렉토리 삭제 */
  sprintf(buf, "rm -rf %s/%s/%s", sys->RealBasePath, sys->DataDirectory, db->db);
  system(buf);
}
