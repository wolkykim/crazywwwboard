#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "qDecoder.h"
#include "arc.h"

#define BUF_LEN			(1000)
#define HEADER_FROM		"From:"
#define HEADER_SUBJECT		"Subject:"
#define HEADER_MESSAGEID	"Message-ID:"

char *pick_between(char *buf, char *string, char *quot1, char *quot2);
int getlines(char *string);

#define BUF_LEN	(1000)

int arc_parse(DBArticle *article) {
  struct tm *nowlocaltime;
  char buf[BUF_LEN+1];
  char *value;
  int  c, c_count, valuelen;

  /* 초기화 */
  nowlocaltime = qGetTime();

  strcpy(article->Name, "");
  strcpy(article->Email, "");
  strcpy(article->Subject, "");
  article->Text = "";

  strcpy(article->Filename, "");
  strcpy(article->Filesize, "");

  sprintf(article->Date, "%04d%02d%02d%02d%02d%02d", nowlocaltime->tm_year, nowlocaltime->tm_mon, nowlocaltime->tm_mday, nowlocaltime->tm_hour, nowlocaltime->tm_min, nowlocaltime->tm_sec);
  strcpy(article->Domain, "");

  article->Lines = 0;
  article->Access = 0;
  article->Thread = 0;

  strcpy(article->Password, "");

  /* Header 분석 */
  while(TRUE) {
    if( fgets(buf, sizeof(buf), stdin) ==  NULL) return FALSE;
    qRemoveSpace(buf);
    if(!strcmp(buf, "")) break; /* Header End */

    if(!strncmp(buf, HEADER_FROM, strlen(HEADER_FROM))) { /* From: */
      /* Name */
      if(pick_between(article->Name, buf, "\"", "\"") != NULL);
      else if(pick_between(article->Name, buf, "'", "'") != NULL);
      else if(pick_between(article->Name, buf, HEADER_FROM, "<") != NULL);
      else if(pick_between(article->Name, buf, HEADER_FROM, " ") != NULL);
      else {
        strcat(buf, "@#$%");
        pick_between(article->Name, buf, HEADER_FROM, "@#$%");
      }
      qRemoveSpace(article->Name);

      /* Email */
      if(pick_between(article->Email, buf, "<", ">") == NULL) {
        strcat(buf, "@#$%");
        pick_between(article->Email, buf, HEADER_FROM, "@#$%");
      }
      if(qCheckEmail(qRemoveSpace(article->Email)) == FALSE) strcpy(article->Email, "");

      if(!strcmp(article->Name, article->Email)) strcpy(article->Name, "");
    }
    else if(!strncmp(buf, HEADER_SUBJECT, strlen(HEADER_SUBJECT))) { /* Subject: */
      strcpy(article->Subject, buf + strlen(HEADER_SUBJECT));
      qRemoveSpace(article->Subject);
    }
    else if(!strncmp(buf, HEADER_MESSAGEID, strlen(HEADER_MESSAGEID))) { /* Message-ID */
      pick_between(article->Domain, buf, "@", ">");
      qRemoveSpace(article->Domain);
    }
  }

  /* Body 분석 */
  for(value = "", valuelen = BUF_LEN, c_count = 0; (c = fgetc(stdin)) != EOF; ) {
    if(c_count == 0) {
      value = (char *)malloc(sizeof(char) * (valuelen + 1));
      if(value == NULL) qError("%s : Memory allocation fail.", OFFICIAL_GATEPRG_NAME);
    }
    else if(c_count == valuelen - 1) {
      char *valuetmp;
      int  i;

      valuelen *= 2;

      /* Here, we do not use realloc(). Because sometimes it is unstable. */
      valuetmp = (char *)malloc(sizeof(char) * (valuelen + 1));
      if(valuetmp == NULL) qError("%s : Memory allocation fail.", OFFICIAL_GATEPRG_NAME);
      for(i = 0; i < c_count; i++) valuetmp[i] = value[i];
      free(value);
      value = valuetmp;
    }
    value[c_count++] = (char)c;
  }
  value[c_count] = '\0';
  article->Text = value;
  article->Lines = getlines(article->Text);

  return TRUE;
}

char *pick_between(char *buf, char *string, char *quot1, char *quot2) {
  char *quotp1, *quotp2;
  int i;

  if((quotp1 = strstr(string, quot1)) == NULL) return NULL;
  if((quotp2 = strstr(quotp1+1, quot2)) == NULL) return NULL;

  if(!strncmp(quotp1, quot1, strlen(quot1))) quotp1 += strlen(quot1);
  if(*quotp2 == *quot2) quotp2--;
  if(quotp1 > quotp2) return NULL;

  for(i = 0;quotp1 <= quotp2; quotp1++, i++) buf[i] = *quotp1;
  buf[i] = '\0';

  return buf;
}

int getlines(char *string){
  int i = 1, j = 0;

  if(!strcmp(string, "")) return 0;

  while(string[j]){
    if(string[j] == '\n')i++;
    j++;
  }
  return i;
}
