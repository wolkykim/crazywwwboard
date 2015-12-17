/*************************************************************
**  Official distribution site : http://cwb98.nobreak.com   **
**           Technical contact : nobreak@nobreak.com        **
**                                                          **
**                        Developed by 'Seung-young Kim'    **
**                        Last updated at July, 7, 1998     **
**                                                          **
**      Copyright (c) 1996-1998 Nobreak Technologies, Inc.. **
*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md5_crypt.h"

int main(void) {
  char buf[0xff], *pt;

  printf("===================================================================\n");
  printf("\n");
  printf("                 MD5 License Key Generator 1.0\n");
  printf("                                         Nobreak Technologies, Inc.\n");
  printf("===================================================================\n");
  printf("o Input string : [________________________________________________]\r");
  printf("o Input string : [");
  fgets(buf, sizeof(buf), stdin);
  for(pt = buf + strlen(buf); pt >= buf && (*pt == '\n' || *pt == '\r' || *pt == '\0'); pt--) *pt = '\0';
  printf("* Key string   : [%s]\n", buf);
  printf("* MD5 result   : [%s]\n", md5_crypt(buf));
  printf("===================================================================\n");
  return (0);
}
