/* 라이센스를 디저블 하기 위해서는 SECURE_LICENSE_KEY 를 uncomment 한다 */
#define SECURE_LICENSE_KEY		"CWB98PE2"
/* DB 생성 겟수를 디저블 하기 위해서는 LICENSED_DB_AMOUNT를 0xffff 로 설정 한다 */
#define LICENSED_DB_AMOUNT		(10)

#ifndef TRUE
#define TRUE		(1)
#endif
#ifndef FALSE
#define FALSE		(0)
#endif

#define OFFICIAL_ADMPRG_NAME		"CrazyAdmin"
#define OFFICIAL_ADMCGI_NAME		"CrazyAdmin.cgi"
#define OFFICIAL_ADMPRG_VERSION		"1.2"

#define OFFICIAL_PROGRAM_NAME		"CrazyWWWBoard"
#define OFFICIAL_CGI_NAME		"CrazyWWWBoard.cgi"
#define OFFICIAL_PROGRAM_VERSION	"98 Professional Edition II"
#define PLATFORM			"UNIX"

#define REALTIME_CONFIG_FILE		"CrazyWWWBoard.conf"
#define PASSWORD_FILE			"CrazyAdmin.conf"

#define LIST_WIDTH			"650"

#define DEFAULT_MASTER_PASSWORD		"0000"
#define MAX_PASSWORD_LEN			(8)

typedef struct {
  char *LicenseKey;

  char *AdmPrgName;  /* CrazyAdmin.cgi */

  char *ProgramName; /* CrazyWWWBoard.cgi */

  char *AdminName;
  char *AdminEmail;

  char *RealBasePath;
  char *HtmlBasePath;

  char *DataDirectory;
  char *DataExtension;

  char *StatDirectory;
  char *StatExtension;

  char *LogDirectory;

  char *HtmlHead;
  char *HtmlTail;

  char *MailHead;
  char *MailTail;

  char *CryptSalt;

  char *IconAdminTitle;
  char *IconReload;
  char *IconBack;
}SysConfig;

typedef struct {
  char *db;
  char *mode;
  char *password;
}DB;

/* global values */
extern int license_valid;

/* common user functions */
void  headHtml(SysConfig *sys);
void  tailHtml(void);
void  closeWindow(void);

/* common system functions */
char *crypt(const char *, const char *);
