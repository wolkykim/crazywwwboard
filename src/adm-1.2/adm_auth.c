#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "gdbm.h"
#include "gdbm_lib.h"
#include "qDecoder.h"
#include "adm.h"
#include "adm_auth.h"

/* user authentication gateway. 비밀번호가 상이할경우 비밀번호 입력 폼을 출력한다 */
void adm_auth(SysConfig *sys, DB *db){
  FILE *fp;
  char sysPass[0xff];

  strcpy(sysPass, "");

  /* password 획득 */
  if((fp = fopen(PASSWORD_FILE, "rt")) != NULL) {
    fgets(sysPass, sizeof(sysPass), fp);
    fclose(fp);
  }
  else { /* 파일이 없으면, 첫번째 접속 패스워드 파일 생성 */
    char *encPass;

    encPass = crypt(DEFAULT_MASTER_PASSWORD, sys->CryptSalt);
    if((fp = fopen(PASSWORD_FILE, "wt")) == NULL) qError("FATAL : Can not create master password file %s. Check directory permissions.", PASSWORD_FILE);
    fprintf(fp, "%s", encPass);
    fclose(fp);

    chmod(PASSWORD_FILE, 0600);
    strcpy(sysPass, encPass);
  }

  qRemoveSpace(sysPass);
  if(strlen(sysPass) == 0) qError("FATAL : Illegal password modification. Check the file %s.", PASSWORD_FILE);

  /* 비밀번호가 없다면, 확인화면 출력 */
  if(!strcmp(db->password, "")) adm_authform(sys, db);

  /* feedback되었다, System password와 비교한다 */
  if(!strcmp(sysPass, db->password)) return;

  /* 비밀번호 재 확인 */
  adm_authform(sys, db);  
}

void adm_authform(SysConfig *sys, DB *db){
  headHtml(sys);

  printf("<table border=0>\n");

  printf("<form method='post' action='%s'>\n", sys->AdmPrgName);
  printf("  <input type='hidden' name='db'         value='%s'>\n", db->db);
  printf("  <input type='hidden' name='mode'       value='%s'>\n", db->mode);

  if(license_valid == TRUE) {
    printf("  <tr><td colspan=2 align=center><font size=3 face='arial' color=blue><b>\n");
    printf("    LICENSE KEY IS VERIFIED\n");
    printf("  </b></font><br><br></td></tr>\n");
  }

  printf("  <tr>\n");
  printf("    <td align=right><font size=2 face='arial'><b>P a s s w o r d</b></font></td>\n");
  printf("    <td align=left><font size=3><input type='password' name='password' size=20 maxlength=%d></font></td>\n", MAX_PASSWORD_LEN);
  printf("  </tr>\n");

  printf("  <tr><td colspan=2><br></td></tr>\n");

  printf("  <tr>\n");
  printf("    <td align=center colspan=2><font size=2><input type='submit' value='        E N T E R        '></font></td>\n");
  printf("  </tr>\n");

  printf("  <tr><td colspan=2><br></td></tr>\n");

  printf("</form>\n");
  printf("</table>\n");

  tailHtml();

  exit(0);
}

void adm_authsave(SysConfig *sys, DB *db){
}
