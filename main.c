#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <expat.h>
#include "bruteforce.h"
#include "lib.h"

/*DA FARE:
-1 funzione per brute force (che genera e tenta la connessione)
-2 implementare a quasta funzione il multithread, il numero di tread viene passato come parametro al main()
*/

struct MemoryStruct {
  char *memory;
  size_t size;
};

struct ParserStruct {
  int ok;
  size_t tags;
  size_t depth;
  struct MemoryStruct characters;
};


char symbols[] = "abcdefghijklmnopqrstuvwxyz0123456789-.";

int main(int argc, char const *argv[]) {
  unsigned maxLen = 0;
  if (argv[1])
    maxLen = atoi(argv[1]);
  if (argc != 2 || maxLen <= 0){
    printf("%sInvalid parameter,%s  %splease provide an input like:%s\n %s [length]\n", RED, RST, YEL, RST, argv[0]);
    return -1;
  }

  brute(maxLen);

  return 0;
}


int brute(unsigned maxLen){
  BRUTEFORCE_HANDLE bfhandler;
  if (!bruteforce_init(&bfhandler, maxLen, BF_FLAG_CUSTOM, symbols, NULL)) {
    printf("%sFailed to initialize bruteforce.%s\n", RED, RST);
    return -1;
  }

  while(bruteforce_update(&bfhandler))
    makeConnection(bfhandler.bfText);

  bruteforce_finalize(&bfhandler);
    return 0;
}


short makeConnection(char *subdomain){
  bool success = false;
  const char domain[17] = ".s3.amazonaws.com";
  char *link = linkcreator(subdomain, domain);
  printf("%sTrying %s...%s\n", YEL, link, RST);
  /*connection*/
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, link);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);

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
    /*inseriscilo nel database*/
    return 1;
  }
  free(link);
  return 0;
}


char *linkcreator(const char* s1, const char* s2){
  char *result = malloc(strlen(s1) + strlen(s2) + 1);
  if (result == NULL) {
    printf("%sOut of memory.%s\n", RED, RST);
    exit(-1);
  }
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}
