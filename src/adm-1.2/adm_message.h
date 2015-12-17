void adm_message(SysConfig *sys, DB *db);
void adm_messagesave(SysConfig *sys, DB *db);

char *readMessage(char *path);
void saveMessage(char *path, char *string);
