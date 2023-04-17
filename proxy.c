#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(char *method,int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(char *method, int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
//tiny의 main 그대로 
int main(int argc, char **argv) {
  printf("%s", user_agent_hdr);
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  /* Check command line args */
  if (argc != 3) {
	  fprintf(stderr, "usage: %s <port>\n", argv[0]);
	  exit(1);
  }
  printf("argc 개수: %d\n", argc);
  printf("argv[0] 확인: %s\n", argv[0]); 
  printf("argv[1] 확인: %s\n", argv[1]); //argv[1] 확인: 8000
  printf("argv[1] 확인: %s\n", argv[2]); //argv[1] 확인: 8080
}