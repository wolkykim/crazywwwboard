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
  int  upNum, dnNum;                     /* 현재 게시물에 대한 위,아래 게시물 번호 */
  char upSubject[0xff], dnSubject[0xff]; /* 위아래 게시물의 제목 */
} UpDnInfo;


void cwb_list(Entry *sysp, SysConfig *sys, DB *db, DBConfig *conf);
int  findExtIcon(Entry *sysp, SysConfig *sys, char *filename, char *icon);
void highlightStr(int mode, char *orgstr, char *orgtok);

void findUPDNnum(char *dbpath, DB *db, DBConfig *conf, UpDnInfo *updn);

int daysAgo(int year, int mon, int day);
