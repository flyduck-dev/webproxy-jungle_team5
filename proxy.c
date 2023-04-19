#include <stdio.h>
#include "csapp.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

typedef struct Cache {
    char *path; // : home.html
    char *contents_buf; // <html><homr>~~~
    struct Cache *next_cache; // next Cache_node
    struct Cache *prev_cache; // prev Cache_node
    int contents_length;
} Cache;

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void parse_uri(char *uri, char *host, char *port, char *path);
void serve_static(char *method,int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(char *method, int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void sendHeadertoTiny(int fd, char *uri);
void *thread(void *vargp);
Cache *find_node(char *path);
void insert_copy_first(Cache *c, int content_length);
void *erase_node(Cache *del_node);
void *delete_node();
void cache_init(char *buf, int content_length, char* path);

static Cache *cache_list_head = NULL;

//tiny의 main 그대로 
int main(int argc, char **argv) { //argv[1] 확인: 8000 //argv[1] 확인: 8080
  int listenfd, *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  if (argc != 2) {
	  fprintf(stderr, "usage: %s <port>\n", argv[0]);
	  exit(1);
  }

  listenfd = Open_listenfd(argv[1]); 

  while (1) {
    clientlen = sizeof(clientaddr);
    /*task1*/
    //connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    //Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    //doit(connfd);
    //Close(connfd);
    
    /*task2*/
    connfdp = Malloc(sizeof(int)); // int를 동적 할당
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Pthread_create(&tid, NULL, thread, connfdp); //새 스레드 생성
  }
}

//교재 echo -> doit
void *thread(void *vargp){
  int connfd = *((int *)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);
  doit(connfd);
  Close(connfd);
  return NULL;
}

void doit(int fd)
{
  rio_t crio, srio;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  char host[MAXLINE], port[MAXLINE], path[MAXLINE];
  int serverfd;

  /* Read client request */
  Rio_readinitb(&crio, fd);
  Rio_readlineb(&crio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version);
  //parse_uri 테스트 완료
  parse_uri(uri, host, port, path);
  //printf("uri은 %s\n", uri); //uri은 http://43.201.38.164:8000/home.html
  //printf("host은 %s\n", host);//host은 43.201.38.164
  //printf("post은 %s\n", port);//post은 8000
  //printf("path은 %s\n", path);//path은 /home.html


  /* 캐시 로직 시작 Find cache object*/
  Cache *c = find_node(path);
  printf("  /* 캐시 로직 시작 Find cache object*/%s\n",c);
//   if (c == cache_list_head){
//     printf(" 11111\n");
//     Rio_writen(fd, c->contents_buf, c->contents_length);
//     return;
//   }
//   else 
  if (c != NULL) {
    // 캐시에서 객체를 찾았을 경우
    Rio_writen(fd, c->contents_buf, c->contents_length);
    insert_copy_first(c, c->contents_length);
    erase_node(c);
    printf("캐시가 있습니다\n");
    return;
  }
  /* 캐시가 없을 경우, Send request to server */
  serverfd = Open_clientfd(host, port);
  sendHeadertoTiny(serverfd, uri);

  /* Receive response from server and forward it to the client */
  Rio_readinitb(&srio, serverfd);
  //Rio_readlineb(&srio, buf, MAXLINE);
  //Rio_writen(fd, buf, strlen(buf));
  //read_requesthdrs(&srio);
  size_t n;
  int c_length = 0;
  while ((n = Rio_readlineb(&srio, buf, MAXLINE)) != 0) {
    printf("%s",buf);
    Rio_writen(fd, buf, n);
    c_length += n;
  }
  printf("캐시가 없습니다\n");
  cache_init(buf, c_length, path);
  printf("이젠 있지 않아?\n");
  Close(serverfd);
}

void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE); // 데이터를 읽어옵니다.
    printf("read_requesthdrs에서 buf 확인 %s", buf); // 클라이언트가 보낸 HTTP 요청 메시지의 첫 번째 줄과 이어지는 헤더 정보를 저장하는 버퍼
    int i =0;
    while(strcmp(buf, "\r\n")) {
	    Rio_readlineb(rp, buf, MAXLINE); //파일 디스크립터나 소켓으로부터 입력 스트림을 읽고, 개행 문자(\n)전까지 버퍼에 저장
	    printf("%d 번째 %s", i, buf);
        i++;
    }
    return;
}

//http://43.201.38.164:8000/home.html
void parse_uri(char *uri, char *host, char *port, char *path) {
    char *host_start = strstr(uri, "://");
    if (host_start != NULL) {
        host_start += 3;
    } else {
        host_start = uri;
    }
    char *host_end = strchr(host_start, ':');
    if (host_end == NULL) {
        strcpy(host, host_start);
        strcpy(port, "80"); // 기본 HTTP 포트 설정
    } else {
        strncpy(host, host_start, host_end - host_start);
        host[host_end - host_start] = '\0';
        char *port_start = host_end + 1;
        char *port_end = strchr(port_start, '/');
        if (port_end == NULL) {
            strcpy(port, port_start);
        } else {
            strncpy(port, port_start, port_end - port_start);
            port[port_end - port_start] = '\0';
        }
    }
    char *path_start = strchr(host_end, '/');
    if (path_start == NULL) {
        strcpy(path, "/");
    } else {
        strcpy(path, path_start);
    }
    printf("path은 %s\n", path);//path은 /home.html
}

void sendHeadertoTiny(int fd, char *uri) { 
    char buf[MAXLINE], host[MAXLINE], port[MAXLINE], path[MAXLINE];
    parse_uri(uri, host, port, path);
    sprintf(buf, "GET %s HTTP/1.1\r\n", path);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%sHost: %s:%d\r\n", buf, host, port);
    Rio_writen(fd, buf, strlen(buf));
    // sprintf(buf, "%s%s", buf, user_agent_hdr);
    // Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, user_agent_hdr, strlen(user_agent_hdr));
    sprintf(buf, "%sConnection: close\r\n", buf);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);
    Rio_writen(fd, buf, strlen(buf));
}

//cache_list_head = insert_first(cache_list_head, c, c->contents_length);
void insert_copy_first(Cache *c, int content_length){
 printf("인서트 진입\n");
  Cache *p = (Cache *)malloc(sizeof(Cache));
  p->path = strdup(c->path);
  p->contents_buf = (char*) malloc(content_length+1);
  memcpy(p->contents_buf, c->contents_buf, content_length);
  p->contents_buf[content_length] = '\0';
  p->contents_length = content_length;
  p->next_cache = cache_list_head;
  p->prev_cache = NULL;
  if (cache_list_head != NULL) {
    cache_list_head->prev_cache = p;
  }
  cache_list_head = p;
}

//정리 
void *erase_node(Cache *del_node) {
    if (del_node == cache_list_head) return;

    if (del_node->prev_cache != NULL) {
        del_node->prev_cache->next_cache = del_node->next_cache;
    }

    if (del_node->next_cache != NULL) {
        del_node->next_cache->prev_cache = del_node->prev_cache;
    }
    free(del_node->path);
    free(del_node->contents_buf);
    free(del_node);
}


//cache_list_head = insert_first(cache_list_head, c, c->contents_length);
void cache_init(char *buf, int content_length, char* path){
  Cache *p = (Cache *)malloc(sizeof(Cache));
  p->path = (char*)malloc(strlen(path) + 1); // 동적으로 메모리 할당
  strcpy(p->path, path);
  p->contents_buf = (char*) malloc(content_length+1);
  memcpy(p->contents_buf, buf, content_length);
  p->contents_buf[content_length] = '\0'; // 문자열 끝에 널 문자 추가
  p->contents_length = content_length;
  p->next_cache = cache_list_head;
  p->prev_cache = NULL;
  cache_list_head = p;
}

/* Find node with given path */
Cache *find_node(char *path) {
    Cache *current = cache_list_head;
    while (current != NULL) {
        if (strcmp(current->path, path) == 0) {
            return current;
        }
        current = current->next_cache;
    }
    return NULL;
}