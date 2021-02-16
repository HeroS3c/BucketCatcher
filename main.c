#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <expat.h>
#include <sqlite3.h>
#include <signal.h>
#include "lib/bruteforce.h"
#include "lib/lib.h"

/*DA FARE:
-1 implementare il multithread, il numero di tread viene passato come parametro al main()
*/
static bool sizeInfo = false;
static bool bRunning = true;
static char symbols[] = "abcdefghijklmnopqrstuvwxyz0123456789-.";
static double size = 0;

int main(int argc, char const *argv[]) {
  bool wordlist = false;
  char *wordlistFile = argv[1];
  unsigned maxLen = 0;
  if (argv[1] && strstr(argv[1], ".txt")) {
    wordlist = true;
  }else{
    if (argv[1])
    maxLen = atoi(argv[1]);
    if (argc != 2 || maxLen <= 0){
      printf("%sInvalid parameter,%s  %splease provide an input like:%s\n %s [length] OR [wordlist.txt]\n", RED, RST, YEL, RST, argv[0]);
      return -1;
    }
  }

  printf("%sDo you want to gather size info (y/N)? %s", CYN, RST);
  switch (getchar()) {
    case 'y':
    case 'Y':
      sizeInfo = true;
    break;
    default:
      sizeInfo = false;
  }

  if (!wordlist) {
    brute(maxLen);
  }else{
    wordlistAttack(wordlistFile);
  }

  return 0;
}

static int wordlistAttack(char *wordlistFile){
  signal(SIGINT, end);
  FILE *file;
  ssize_t read;
  size_t len = 0;
  char *line = NULL;
  unsigned lines = 0;
  file = fopen(wordlistFile, "r"); // open file to count lines
  if ( file == NULL ) {
    return -1;
  }
  lines = lineCounter(file);
  file = fopen(wordlistFile, "r"); // reopen file co read each line
  if ( file == NULL ) {
    return -1;
  }
  unsigned currline = 1;
  while ((read = getline(&line, &len, file)) != -1) {
    char *newline = strchr(line, '\n');
    if (newline)
      *newline = 0;
    unsigned short percent = (currline*100)/lines;
    makeConnection(line, percent);
    currline++;
  }
}


static unsigned lineCounter(FILE *file){
  unsigned lines = 0;
  while (EOF != (fscanf(file, "%*[^\n]"), fscanf(file,"%*c")))
        ++lines;
  return lines;
}

static int brute(unsigned maxLen){ // bruteforce subroutine
  signal(SIGINT, end);
  BRUTEFORCE_HANDLE bfhandler;
  if (!bruteforce_init(&bfhandler, maxLen, BF_FLAG_CUSTOM, symbols, NULL)) {
    printf("%sFailed to initialize bruteforce.%s\n", RED, RST);
    return -1;
  }

  while(bruteforce_update(&bfhandler) && bRunning){
    makeConnection(bfhandler.bfText, -1);
  }

  bruteforce_finalize(&bfhandler);
    return 0;
}


static short makeConnection(char *subdomain, int percent){ // called by bruteforce() to try make a connection (return 1 if connection is done)
  bool success = false;
  const char domain[17] = ".s3.amazonaws.com";
  char *link = linkcreator(subdomain, domain);
  if (percent >= 0) {
    printf("%sTrying %s... (%d%%)%s\n", YEL, link, percent, RST); // in case of wordlist attack (we have a progress)
  }else{
    printf("%sTrying %s... %s\n", YEL, link, RST); // in case of bruteforce (runs until we stop, so progress to print out)
  }

  /*connection*/
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, link);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    if (sizeInfo) {
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

  if (success){
    printf("%s %s Exist!%s\n", GRN, link, RST);
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
  char *checkExist = "SELECT * FROM links;"; //debug:error no such table links
  rc = sqlite3_exec(db, checkExist, NULL, 0, &zErrMsg);
  if(rc != SQLITE_OK){
     fprintf(stderr, "%sSQL: %s, creating a new table%s\n", MAG, zErrMsg, RST);
     sqlite3_free(zErrMsg);
  }
  if (rc) { // if table does not exist, let it create!
    char *sql = "CREATE TABLE links("  \
     "ID INTEGER PRIMARY KEY     AUTOINCREMENT," \
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

   sqlite3_stmt *stmt;
   sqlite3_prepare_v2(db, "INSERT INTO links(NAME, LINK, SIZE, STARRED)  VALUES (?, ?, ?, 0);", 70, &stmt, NULL);
   if(stmt != NULL) {
      sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, link, -1, SQLITE_TRANSIENT);
      sqlite3_bind_int(stmt, 3, size);
      sqlite3_step(stmt);
      sqlite3_finalize(stmt);
   }else{
     printf("%sError during insertion in the database%s\n", RED, RST);
     return 0;
   }

  sqlite3_close(db);
  return 1;
}

static void end() {
  bRunning = false;
  printf("%s\nProcess ended. Result saved in: strikedS3.db%s\n", BWHT, RST);
  printf("%sWould you like to create an html report (y/N)? %s", BBLU, RST);
  getchar(); // to pause
  switch (getchar()) {
    case 'y':
    case 'Y':
      report();
      endwin();
      exit(0);
    break;
    default:
      endwin();
      exit(0);
  }
}


static void report(void) {
  printf("Function yet to be implemented, you can find the results in strikedS3.db"); // TO FINISH

    
  sqlite3 *db;
  sqlite3_stmt *res;

  int rc = sqlite3_open(":memory:", &db);

  if (rc != SQLITE_OK) {
      fprintf(stderr, "%sCannot open database: %s%s\n", RED, sqlite3_errmsg(db), RST);
      sqlite3_close(db);

      exit(-1);
  }
  char *error;
  char *sql = "SELECT * FROM links;";
  rc = sqlite3_exec(db, sql, NULL, 0, &error);
  printf("%s\n", error);

  if (rc == SQLITE_ROW) { // if there are results
    printf("%s\n", sqlite3_column_text(res, 0));
    FILE *file;
    file = fopen("result.html","r+");
    if(file == NULL){
      printf("%sFailed to create report, try running as sudo next time!%s", RED, RST);
      exit(-1);
    }

    fprintf(file,"%s","<link href=\"//maxcdn.bootstrapcdn.com/bootstrap/3.3.0/css/bootstrap.min.css\" rel=\"stylesheet\" id=\"bootstrap-css\"><script src=\"//maxcdn.bootstrapcdn.com/bootstrap/3.3.0/js/bootstrap.min.js\"></script><script src=\"//code.jquery.com/jquery-1.11.1.min.js\"></script>");
    fprintf(file,"%s","<div id=\"fullscreen_bg\" class=\"fullscreen_bg\"/> <form class=\"form-signin\"><div class=\"container\"><div class=\"row\"><div class=\"col-md-7 col-md-offset-2\"> <div class=\"panel panel-default\"><div class=\"panel panel-primary\">");
    fprintf(file,"%s","<div class=\"text-center\"><h3 style=\"color:#2C3E50\">Report</h3><div class=\"panel-body\"> ");
    fprintf(file,"%s","<table class=\"table table-striped table-condensed\"><thead><tr><th class=\"text-center\" width=\"115px\">ID</th><th class=\"text-center\" width=\"115px\">Name</th><th class=\"text-center\" width=\"115px\">LINK</th><th class=\"text-center\" width=\"115px\">Size</th></tr></thead><tbody>");

    while (sqlite3_step(res) != SQLITE_DONE) {
      int i;
      int num_cols = sqlite3_column_count(res);

      for (i = 0; i < num_cols; i++){
        switch (sqlite3_column_type(res, i)){
          case (SQLITE3_TEXT):
          fprintf(file, "%s", sqlite3_column_text(res, i));
          printf("%s, ", sqlite3_column_text(res, i)); //debug
          break;
          case (SQLITE_INTEGER):
          fprintf(file, "%hhn", sqlite3_column_text(res, i));
          printf("%d, ", sqlite3_column_int(res, i)); //debug
          break;
          default:
          break;
        }
      }
      printf("\n");
    }

    fclose(file);
  }

  sqlite3_close(db);
}
