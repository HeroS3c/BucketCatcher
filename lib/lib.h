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
//bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define BWHT "\e[1;37m"


/*Prototypes*/
static unsigned lineCounter(FILE *file); // count number of lines of the wordlist
static void progressBar(size_t count, size_t max); // prints the progress bar
static int brute(unsigned maxLen); // bruteforce subroutine
static int wordlistAttack(char *wordlistFile); // wordlist attack subroutine
static short makeConnection(char *subdomain, int percent); // called by bruteforce() to try make a connection (return 1 if connection is done)
static char *linkcreator(const char* s1, const char* s2); // append `.s3.amazonaws.com` to generated string
static int dbInsert(char *name, char *link); // insert successul link in database
static void end(); // called what user do CTRL+C, stop the bruteforce and asks to print results
static void report(void); // print content of strikedS3 (called by end())
