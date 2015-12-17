/* ���̼����� ������ �ϱ� ���ؼ��� SECURE_LICENSE_KEY �� uncomment �Ѵ� */
#define SECURE_LICENSE_KEY		"CWB98PE2"
#define INVALID_LICENSE_LIVE_DAYS	(6) /* today, 1, 2 available */

#ifndef TRUE
#define TRUE		(1)
#endif
#ifndef FALSE
#define FALSE		(0)
#endif

#define FNAME		(1)      /* �̸��� ���ϸ� */
#define FSUBJECT	(2)      /* ���� */
#define FTEXT		(4)      /* ���� */

#define OFFICIAL_PROGRAM_NAME		"CrazyWWWBoard"
#define OFFICIAL_CGI_NAME		"CrazyWWWBoard.cgi"
#define OFFICIAL_PROGRAM_VERSION	"98 Professional Edition II"
#define PLATFORM			"UNIX"
#define REALTIME_CONFIG_FILE		"CrazyWWWBoard.conf"

#define TYPE_UNLINK	(0)
#define TYPE_LINK	(1)
#define TYPE_TEXT	(2)
#define TYPE_IMAGE	(3)
#define TYPE_SOUND	(4)
#define TYPE_MOVIE	(5)

#define MAX_PASSWORD_LEN	(8)

typedef struct {
  char *db;
  char *mode;
  int  page;
  int  num;
  char *password;

  char *category;
  int  ftype;   /* 3���� And ó�� �ȴ�, �̸�:00000001, ����:00000010, ����:00000100 */
  char *fval;
  char *fencval; /* Encoding�� fval, Link�� */

  int  backdepth; /* ������ back ���� ���ؼ� ����Ѵ�. ������ back�� ������ ����Ʈ�� �ü� �ֵ��� */
}DB;

typedef struct {
  char *LicenseKey;

  char *ProgramName;

  char *AdminName;
  char *AdminEmail;

  char *RealBasePath;
  char *HtmlBasePath;

  char *DefaultTitle;
  char *DefaultCharSet;

  char *UploadSoftLimit;
  char *UploadHardLimit;

  char *DataDirectory;
  char *DataExtension;

  char *StatDirectory;
  char *StatExtension;

  char *LogDirectory;

  char *HtmlHead;
  char *HtmlTail;

  char *Help;

  char *IconAdminTitle;
  char *IconAdmin;
  char *IconStat;
  char *IconHome;
  char *IconBack;
  char *IconHelp;

  char *IconList;
  char *IconReload;
  char *IconWrite;
  char *IconModify;
  char *IconReply;
  char *IconDelete;

  char *IconUp;
  char *IconDown;

  char *IconBlank;

  char *IconWidth;
  char *IconHeight;

  char *IconOpen;
  char *IconClose;
  char *IconOpenNew;
  char *IconCloseNew;
  char *IconTypeDefault;
  char *IconTypeUnknown;

  char *StatBarWidth;
  char *StatBarMaxHeight;

  char *StatBarUnder;
  char *StatBar90;
  char *StatBar80;
  char *StatBar70;
  char *StatBar60;
  char *StatBar50;
  char *StatBar40;
  char *StatBar30;
  char *StatBar20;
  char *StatBar10;
  char *StatBar00;

  char *CryptSalt;

  char *Sendmail;
  char *SendmailOptions;

  char *MailHead;
  char *MailTail;

  char *MailName;
  char *MailOrganization;

  char *CookieEnable;
  char *CookieExpireDays;

  char *PageLinkNum;

  char *TextFieldLength;

  char *ReSubject;
  char *ReText;
  char *ReLine;

  char *PreviewLines;

  char *HiddenAdminLink;
  char *HiddenStatLink;
  char *HiddenHelpLink;

  char *AllowDBhtml;
  char *AllowDBmail;
  char *AllowUpload;
}SysConfig;

typedef struct {
  int  LastNum;

  char *Password;
  char *Title;
  char *Message;

  char *HomeUrl;
  char *HomeTarget;
  char *BackUrl;
  char *BackTarget;

  char *AdminName;
  char *AdminEmail;
  char *MailToAdmin;
  char *MailToSender;

  /* common */
  char *Language;
  char *FontSize;
  char *UnderlinedLink;
  char *TableWidth;
  char *CharSet;
  char *UseSysDefHtml;
  char *HtmlHead;
  char *HtmlTail;

  /* list */
  char *ListOrder;
  char *ArticlesPerPage;
  char *FieldNumber;
  char *FieldFiletype;
  char *FieldFilename;
  char *FieldName;
  char *FieldSubject;
  char *FieldDate;
  char *FieldHits;
  char *FieldDownload;
  char *FolderIcon;
  char *HighlightArticle;
  char *HideSearch;
  char *ListIconLocation;
  char *ListBorderSize;
  char *ListTitleFontColor;
  char *ListTitleBgColor;
  char *ListArticleFontColor;
  char *ListArticleBgColor;

  /* read */
  char *HtmlAccept;
  char *LinkTarget;
  char *DisplayAddress;
  char *ReadIconLocation;
  char *ReadLineHeight;
  char *ReadTitleBorderSize;
  char *ReadTextBorderSize;
  char *ReadTitleFontColor;
  char *ReadTitleBgColor;
  char *ReadTextFontColor;
  char *ReadTextBgColor;

  /* write */
  char *UploadEnable;
  char *SubjectText;
  char *DefaultSubject;
  char *DefaultText;
  char *UseCategory;
  char *Category;
  char *WriteTitleFontColor;
  char *WriteTitleBgColor;

  /* email */
  char *UseSysDefMail;
  char *MailHead;
  char *MailTail;

  /* permissions */
  char *ListMode;
  char *WriteMode;
  char *ReadMode;
  char *StatMode;

}DBConfig;

typedef struct {
  /* Article Info �ӽ� */  
  int  Num;
  int  Year, Mon, Day, Hour, Min, Sec;

  /* ���� DB�� ����Ǵ� Article Skeleton Full Field */
  char *Name;
  char *Email;
  char *Subject;
  char *Text;

  char *Category;

  char *Filename;
  char *Filesize;
  int  Download;

  char *Date;
  char *Domain;

  int  Lines;
  int  Access;
  int  Thread;

  char *Password;
}DBArticle;

/*** ��Ϻ��� ��⿡�� ����ϴ� ���� Article Skeleton ***/
typedef struct {
  int  Num;
  int  Year, Mon, Day;
  int  DaysAgo; /* �Խù��� ���糯¥�� �������� ���� ���� �ԽõǾ����� ����ȴ� */

  /* DB���� ���� */
  char *Name;
  char *Email;
  char *Subject;

  char *Category;

  char *Filename;
  char *Filesize;
  int  Download;

  char *Date;
  int  Lines;
  int  Access;
}DBList;

/*** ��� ���� ����ü ***/
typedef struct {
  int sYear, sMon, sDay; /* �α� ���� �� �� */
  int eYear, eMon, eDay; /* �α� ���� �� �� */
  int Year,  Mon,  Day[31+1], Max, Total, Avg;
  int TotalAccess;
}Stat;

/* global values */
extern int license_valid;

/* common user functions */
void headHtml(SysConfig *sys, DBConfig *conf);
void tailHtml(SysConfig *sys, DBConfig *conf);
void headAdmin(void);
void tailAdmin(void);
void runTime(void);
char *exchangeQuot(char *str);

/* common system functions */
char *crypt(const char *, const char *);
FILE *popen(const char *, const char *);
int  pclose(FILE *);
