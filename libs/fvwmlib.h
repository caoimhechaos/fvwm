int mystrcasecmp(char *a, char *b);
int mystrncasecmp(char *a, char *b,int n);
char *CatString3(char *a, char *b, char *c);
int mygethostname(char *client, int namelen);
void SendText(int *fd,char *message,unsigned long window);
void SendInfo(int *fd,char *message,unsigned long window);
char *safemalloc(int);
char *findIconFile(char *icon, char *pathlist, int type);
int ReadFvwmPacket(int fd, unsigned long *header, unsigned long **body);
void CopyString(char **dest, char *source);
void sleep_a_little(int n);
int GetFdWidth(void);