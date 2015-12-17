/********* Index Sturcture *************/
typedef struct {
  int num;
  int thread;
}D_INDEX;

typedef struct {
  int num;
  int depth;
}T_INDEX;

typedef struct {
  int  upNum, dnNum;                     /* ���� �Խù��� ���� ��,�Ʒ� �Խù� ��ȣ */
  char upSubject[0xff], dnSubject[0xff]; /* ���Ʒ� �Խù��� ���� */
} UpDnInfo;


void cwb_list(Entry *sysp, SysConfig *sys, DB *db, DBConfig *conf);
int  findExtIcon(Entry *sysp, SysConfig *sys, char *filename, char *icon);
void highlightStr(int mode, char *orgstr, char *orgtok);

void findUPDNnum(char *dbpath, DB *db, DBConfig *conf, UpDnInfo *updn);

int daysAgo(int year, int mon, int day);
