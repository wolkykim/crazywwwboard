/*************************************************************
** GDBM Library                                             **
**                                                          **
**           Technical contact : nobreak@nobreak.com        **
**                                                          **
**                        Developed by 'Seung-young Kim'    **
**                        Last updated at July, 15, 1998    **
**                                                          **
**      Copyright (c) 1996-1998 Nobreak Technologies, Inc.. **
*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include "qDecoder.h"
#include "gdbm.h"
#include "gdbm_lib.h"

static GDBM_FILE _dbf = NULL;

void DBError(void){
 const char *errstr;

 errstr = gdbm_strerror(gdbm_errno);
 qError("DataBase Access Error : %s", errstr);
}

int DBCreate(char *filename, int mode){
  GDBM_FILE newdb;

  /* 화일이 존재하는지 확인 */
  if(qCheckFile(filename) == TRUE) return FALSE;

  newdb = gdbm_open(filename, BLOCK_SIZE, GDBM_WRCREAT, mode, NULL);

  if(newdb == NULL) return FALSE;

  gdbm_close(newdb);

  return TRUE;
}


/* DB를 READER와 WRITER로 열때 사용 */
void DBOpen(char *filename, int flag){
  int i;
  
  if(_dbf != NULL) qError("There is already opened database.");

  for(i=0; i < DB_TRY; i++){
    _dbf = gdbm_open(filename, 0, flag | GDBM_FAST, 0, NULL);
    if(_dbf != NULL)break;
    if(gdbm_errno != GDBM_CANT_BE_READER && gdbm_errno != GDBM_CANT_BE_WRITER) DBError();
    sleep(1);
  }
  if(_dbf == NULL) qError("Time Out!!! Please, try again.");
}

void DBClose(void){
  if(_dbf == NULL) qError("There is no database to close.");
  gdbm_close(_dbf);
  _dbf = NULL;
}

char *DBFetch(char *format, ...){
  datum buf, ret;

  char key[1000 + 1];
  int status;
  va_list arglist;

  va_start(arglist, format);
  status = vsprintf(key, format, arglist);
  if(strlen(key) > 1000 || status == EOF) qError("DBFetch() : Key is too long or not valid");

  if(_dbf == NULL) qError("There is no opened database.");
  buf.dptr = key;
  buf.dsize = strlen(key) + 1;
  ret = gdbm_fetch(_dbf, buf);
  return ret.dptr;
}

int DBiFetch(char *format, ...){
  char key[1000 + 1];
  int status;
  va_list arglist;

  char *str;
  int  i;

  va_start(arglist, format);
  status = vsprintf(key, format, arglist);
  if(strlen(key) > 1000 || status == EOF) qError("DBiFetch() : Key is too long or not valid");

  str = DBFetch(key);

  if(str == NULL) return 0;
  else {
    i = atoi(str);
    free(str);
  }
  return i;
}

void DBStore(char *key, char *str, int flag){
  datum key_data, data_data;

  if(key == NULL) qError("Key string is null. Null pointer can not be stored.");
  if(str == NULL) qError("Value string is null on key %s. Null pointer can not be stored.", key);
  if(_dbf == NULL) qError("There is no opened database.");

  key_data.dptr   = key;
  key_data.dsize  = strlen(key) + 1;
  data_data.dptr  = str;
  data_data.dsize = strlen(str) + 1;

  if(gdbm_store(_dbf, key_data, data_data, flag) != 0)DBError();  
}

void DBiStore(char *key, int value, int flag){
  char buf[0xFF];

  sprintf(buf, "%d", value);
  DBStore(key, buf, flag);
}

int DBDelete(char *format, ...){
  datum buf;
  int ret;

  char key[1000 + 1];
  int status;
  va_list arglist;

  va_start(arglist, format);
  status = vsprintf(key, format, arglist);
  if(strlen(key) > 1000 || status == EOF) qError("DBDelete() : Key is too long or not valid");

  if(_dbf == NULL) qError("There is no database to fetch.");
  buf.dptr  = key;
  buf.dsize = strlen(key) + 1;
  ret = gdbm_delete(_dbf, buf);
  return ret;
}

int DBIsKeyExist(char *format, ...){
  char key[1000 + 1];
  int status;
  va_list arglist;
  char *str;

  va_start(arglist, format);
  status = vsprintf(key, format, arglist);
  if(strlen(key) > 1000 || status == EOF) qError("DBIsKeyExist() : Key is too long or not valid");

  if((str = DBFetch(key)) == NULL) return FALSE;

  free(str);
  return TRUE;
}

int DBReorganize(char *filename){
  GDBM_FILE dbf;

  dbf = gdbm_open(filename, 0, GDBM_WRITER, 0, NULL);
  if(dbf == NULL) return FALSE;

  if(gdbm_reorganize(dbf) != 0) {
    gdbm_close(dbf);
    return FALSE;
  }

  gdbm_close(dbf);
  return TRUE;
}
