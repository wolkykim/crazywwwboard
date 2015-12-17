/*
Edited by nobreak@nobreak.com at 1998/09/28
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md5_global.h"
#include "md5.h"

char *md5_crypt(char *string) {
  MD5_CTX context;
  unsigned char digest[16], tmphex[2+1];
  static char md5hex[16 * 2 + 1];
  int i, j;

  MD5Init (&context);
  MD5Update (&context, string, strlen(string));
  MD5Final (digest, &context);

  for(i = j = 0; i < 16; i++) {
    sprintf(tmphex, "%02x", digest[i]);
    md5hex[j++] = tmphex[0];
    md5hex[j++] = tmphex[1];
  }
  md5hex[j] = '\0';

/*  printf ("MD5 (%s) : '%s'\n", string, md5hex); */
  return md5hex;
}
