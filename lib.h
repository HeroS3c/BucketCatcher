/*Colors*/
#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"
#define RST "\e[0m"


/*Prototypes*/
static int brute(unsigned maxLen); // bruteforce subroutine
static short makeConnection(char *subdomain); // called by bruteforce() to try make a connection (return 1 if connection is done)
static char *linkcreator(const char* s1, const char* s2); // append `.s3.amazonaws.com` to generated string
static int dbInsert(char *name, char *link); // insert successul link in database
