#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <expat.h>
#include <sqlite3.h>
#include "bruteforce.h"
#include "lib.h"

/*DA FARE:
-1 funzione per brute force (che genera e tenta la connessione)
-2 implementare a quasta funzione il multithread, il numero di tread viene passato come parametro al main()
*/
static bool sizeInfo = false;
static char symbols[] = "abcdefghijklmnopqrstuvwxyz0123456789-.";
static double size = 0;

int main(int argc, char const *argv[]) {
  unsigned maxLen = 0;
  if (argv[1])
    maxLen = atoi(argv[1]);
  if (argc != 2 || maxLen <= 0){
    printf("%sInvalid parameter,%s  %splease provide an input like:%s\n %s [length]\n", RED, RST, YEL, RST, argv[0]);
    return -1;
  }

  printf("%sDo you want to gather size info (y/N)? %s\n", CYN, RST);
  switch (getchar()) {
    case 'y':
    case 'Y':
      sizeInfo = true;
    break;
    default:
      sizeInfo = false;
  }
  brute(maxLen);

  return 0;
}


static int brute(unsigned maxLen){ // bruteforce subroutine
  BRUTEFORCE_HANDLE bfhandler;
  if (!bruteforce_init(&bfhandler, maxLen, BF_FLAG_CUSTOM, symbols, NULL)) {
    printf("%sFailed to initialize bruteforce.%s\n", RED, RST);
    return -1;
  }

  while(bruteforce_update(&bfhandler)){
    makeConnection(bfhandler.bfText);
  }

  bruteforce_finalize(&bfhandler);
    return 0;
}


static short makeConnection(char *subdomain){ // called by bruteforce() to try make a connection (return 1 if connection is done)
  bool success = false;
  const char domain[17] = ".s3.amazonaws.com";
  char *link = linkcreator(subdomain, domain);
  printf("%sTrying %s...%s\n", YEL, link, RST);
  /*connection*/
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://pinterest.s3.amazonaws.com/"/*link*/);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    if (sizeInfo) {
      curl_easy_setopt(curl, CURLOPT_NOBODY, 0);
      if ((res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size))){
        printf("%sFailed to get size information.%s\n", RED, RST);
        exit(-1);
      }
    }

    res = curl_easy_perform(curl);
    if(res == CURLE_OK)
      success = true;

    curl_easy_cleanup(curl);
  }else{
    printf("%sFailed to initialize curl%s", RED, RST);
    exit(-1);
  }

  if (success || size != 0){
    printf("%s %s Exist!%s\n", GRN, link, RST);
    printf("size: %f\n", size);
    dbInsert(subdomain, link);
    return 1;
  }
  free(link);
  return 0;
}


static char *linkcreator(const char* s1, const char* s2){
  char *result = malloc(strlen(s1) + strlen(s2) + 1);
  if (result == NULL) {
    printf("%sOut of memory.%s\n", RED, RST);
    exit(-1);
  }
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}


static int dbInsert(char *name, char *link){
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;
  rc = sqlite3_open("strikedS3.db", &db);
  if(rc) {
     fprintf(stderr, "%sCan't open database:%s %s\n", RED, RST, sqlite3_errmsg(db));
     exit(-1);
  }
  char *checkExist = "SELECT 1 FROM links;";
  rc = sqlite3_exec(db, checkExist, NULL, 0, &zErrMsg);
  if(rc != SQLITE_OK){
     fprintf(stderr, "%sSQL error: %s, creating a new table%s\n", MAG, zErrMsg, RST);
     sqlite3_free(zErrMsg);
  }
  if (rc) { // if table does not exist, let it create!
    char *sql = "CREATE TABLE links("  \
     "ID INT PRIMARY KEY     NOT NULL," \
     "NAME           TEXT    NOT NULL," \
     "LINK           TEXT    NOT NULL," \
     "SIZE           DOUBLE     ," \
     "STARRED        BOOL);" \
     ;
     rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
     if(rc != SQLITE_OK){
        fprintf(stderr, "%sSQL error: %s%s\n", RED, zErrMsg, RST);
        sqlite3_free(zErrMsg);
        exit(-1);
     }
   }

   char *sql = "\0";
   sqlite3_stmt *stmt;
   sqlite3_prepare_v2(db, "INSERT INTO links(ID, NAME, LINK, SIZE, STARRED) VALUES (,?,?,?,);", 41, &stmt, NULL);
   if(stmt != NULL) {
      sqlite3_bind_text(stmt, 1, name, 0, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, link, 0,SQLITE_TRANSIENT);
      sqlite3_bind_int(stmt, 2, size);
      sqlite3_step(stmt);
      sqlite3_finalize(stmt);
   }
   rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
   if(rc != SQLITE_OK){
      fprintf(stderr, "%sSQL error: %s%s\n", RED, zErrMsg, RST);
      sqlite3_free(zErrMsg);
      exit(-1);
   }


  sqlite3_close(db);
}
