#define PASS_FAIL	(0)
#define PASS_ADMIN	(1)
#define PASS_POSTER	(2)
#define PASS_NOT_DEFINE	(3)

int  check_password(SysConfig *sys, DB *db, DBConfig *conf);
void cwb_admin(SysConfig *sys, DB *db, DBConfig *conf);
void cwb_adminsave(SysConfig *sys, DB *db, DBConfig *conf);