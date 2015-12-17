#ifndef TRUE
#define TRUE	(1)
#endif

#ifndef FALSE
#define FALSE	(0)
#endif

#ifndef _GDBM_LIB
#define _GDBM_LIB

#define DB_TRY		(10)
#define BLOCK_SIZE	(1024 * 1)

#endif

void DBError(void);

int  DBCreate(char *filename, int mode);

void DBOpen(char *filename, int flag);
void DBClose(void);

char *DBFetch(char *format, ...);
int  DBiFetch(char *format, ...);

void DBStore(char *key, char *str, int flag);
void DBiStore(char *key, int value, int flag);

int  DBDelete(char *format, ...);

int  DBReorganize(char *filename);
