#ifndef TRUE
#define TRUE		(1)
#endif
#ifndef FALSE
#define FALSE		(0)
#endif

#define OFFICIAL_GATEPRG_NAME		"CrazyArchive"
#define OFFICIAL_GATEPRG_VERSION	"1.0"

#define REALTIME_CONFIG_FILE		"CrazyWWWBoard.conf"

typedef struct {
  char *RealBasePath;

  char *DataDirectory;
  char *DataExtension;

  char *LogDirectory;

  char *MailName;
}SysConfig;

typedef struct {
  /* 실제 DB에 저장되는 Article Skeleton Full Field */
  char Name[0xff];  /* Parsed in Module arc_parse() */
  char Email[0xff]; /* Parsed in Module arc_parse() */
  char Subject[0xff]; /* Parsed in Module arc_parse() */
  char *Text; /* Parsed in Module arc_parse() */

  char Filename[0xff];
  char Filesize[0xff];

  char Date[0xff]; 
  char Domain[0xff]; /* Parsed in Module arc_parse() */

  int  Lines;
  int  Access;
  int  Thread;

  char Password[0xff];
}DBArticle;

/* common system functions */
FILE *popen(const char *, const char *);
int  pclose(FILE *);
