/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh 
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(char *method,int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(char *method, int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(1);
    }
    printf("argc 개수: %d\n", argc);
    printf("argv[0] 확인: %s\n", argv[0]);
    printf("argv[1] 확인: %s\n", argv[1]);

    listenfd = Open_listenfd(argv[1]); //Open_listenfd 함수는 argv[1]에 지정된 포트번호를 바인딩
                                       //클라 요청을 대기하기 위한 소켓파일 디스크립터 listenfd 반환
    while (1) {
	    clientlen = sizeof(clientaddr);
        printf("듣고 있는 중... (%d)\n",clientlen);
        printf("while문 안에서 argv[0] 확인: %d\n", argv[0]);
        printf("while문 안에서 argv[1] 확인: %d\n", argv[1]);
        //Accept 함수는 listenfd에 대한 연결 요청이 있을 때까지 블록킹하며 대기
        //클라이언트의 연결 요청이 있으면 클라와 연결된 새로운 소켓파일 디스크립터 connfd를 반환합니다.
        //clientaddr에 클라이언트의 주소 정보를 저장합니다.
	    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        //clientaddr에 저장된 클라이언트의 주소 정보에서 호스트 이름과 포트 번호를 추출하여 hostname과 port에 저장
        //연결된 클라이언트의 주소 정보에서 호스트 이름과 포트 번호를 추출하여 hostname과 port 변수에 저장합니다.
        printf("Accepted connection from (클라 주소에서 뽑은 호스트이름 %s, 클라 주소에서 뽑은 포트번호 %s)\n", hostname, port);
	    doit(connfd);   //doit 함수는 connfd에 대한 클라와의 통신을 처리
	    Close(connfd);  //Close 함수는 connfd를 닫습니다.
    }
}
/* $end tinymain */

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) //connfd
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd); // 소켓 파일 디스크립터 fd에 대한 입력 스트림을 초기화
    if (!Rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
        return;
    printf("buf 확인! %s", buf); // GET /gizmo.mp4 HTTP/1.1
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("method 확인! %s \n", method); //GET
    printf("uri 확인! %s \n", uri); ///gizmo.mp4 /godzilla.gif 
    printf("version 확인! %s \n", version); //HTTP/1.1
    //sscanf() 함수는 C 표준 라이브러리 함수 중의 하나로, 문자열에서 서식에 맞게 값을 읽어와 변수에 저장하는 함수입니다.
    //strcasecmp() 함수는 문자열을 대소문자 구분없이 비교하는 함수
    //둘중 하나가 GET이거나 HEAD라면 실행하지 않는다
    if (strcasecmp(method, "GET")!=0 && strcasecmp(method, "HEAD")!=0 ) {
        clienterror(fd, method, "501", "Not Implemented",
        "Tiny does not implement this method");
        return; 
    }
    //위에서 리턴을 안 했다면 아래줄을 타고 들어가서 헤더 읽어옴
    read_requesthdrs(&rio);
    /*
    rio_t는 버퍼링된 입력을 처리하기 위한 구조체(struct)
    구조체 멤버
    rio_fd: 현재 버퍼링된 입력을 제공하는 파일 디스크립터
    rio_cnt: 버퍼 내의 아직 읽지 않은 바이트 수
    rio_bufptr: 버퍼 내에서 현재 위치를 가리키는 포인터
    rio_buf: 실제로 입력이 버퍼링되는 배열
    rio_t 구조체는 csapp.h 라이브러리에서 사용되며, Robust I/O(Rio) 라이브러리를 위한 자료형
    Rio_readinitb() 함수를 통해 rio_t 구조체를 초기화하고, Rio_readlineb() 함수를 통해 rio_t 구조체 내의 버퍼를 사용하여 파일 디스크립터로부터 입력을 읽어온다. I/O 버퍼링을 구현하고, 일반적인 I/O 함수들보다 안전하게 I/O를 처리할 수 있다.
    */
    /* Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);       //line:netp:doit:staticcheck
    if (stat(filename, &sbuf) < 0) {                     //line:netp:doit:beginnotfound
	    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
	    return;
    } 

    if (is_static) { /* Serve static content */          
	    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { //line:netp:doit:readable
	        clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't read the file");
	        return;
	    }
	    serve_static(method, fd, filename, sbuf.st_size);        //line:netp:doit:servestatic
    }
    else { /* Serve dynamic content */
	    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { //line:netp:doit:executable
	        clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't run the CGI program");
	        return;
	}
	    serve_dynamic(method, fd, filename, cgiargs);            //line:netp:doit:servedynamic
    }
}
/* $end doit */

/*
 * read HTTP request headers
 */
//rp는 rio_t 구조체에 대한 포인터입니다. 이 함수는 rp가 가리키는 rio_t 구조체를 사용하여 HTTP 요청 헤더를 읽어오는 함수입니다.
//함수를 사용하여 rp가 가리키는 rio_t 구조체에서 한 줄씩 데이터를 읽어옵니다.
//읽어온 줄이 "\r\n"과 같은 빈 줄이 되면 헤더 읽기를 멈추고 함수를 반환합니다.
//함수는 HTTP 요청 메시지의 첫 번째 줄(Request-Line) 다음부터 나오는 모든 헤더 정보를 읽어오는 역할을 합니다. Rio_readlineb() 함수를 통해 읽어들인 헤더 정보는 printf() 함수를 사용하여 출력합니다.
void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE); // 데이터를 읽어옵니다.
    printf("read_requesthdrs에서 buf 확인 %s", buf); // 클라이언트가 보낸 HTTP 요청 메시지의 첫 번째 줄과 이어지는 헤더 정보를 저장하는 버퍼
    int i =0;
    while(strcmp(buf, "\r\n")) {
	    Rio_readlineb(rp, buf, MAXLINE); //파일 디스크립터나 소켓으로부터 입력 스트림을 읽고, 개행 문자(\n)전까지 버퍼에 저장
        //이 함수는 rio_t 구조체를 사용하여 내부 버퍼링을 수행하며, rio_t 구조체 내부의 버퍼에 데이터를 읽어들인 후 개행 문자가 나올 때까지 버퍼링을 계속하며, 개행 문자가 나타나면 버퍼에 저장된 데이터를 buf에 복사합니다.
        //rp는 입력 스트림을 읽어들일 파일 디스크립터나 소켓에 대한 rio_t 구조체에 대한 포인터입니다. buf는 읽어들인 데이터를 저장할 버퍼이며, MAXLINE은 버퍼의 최대 크기를 나타냅니다.
        //이 함수는 읽어들인 문자열의 길이를 반환하며, 입력 스트림이 종료된 경우에는 0을 반환합니다.
	    printf("%d 번째 %s", i, buf);
        i++;
    }
    return;
}
/* $end read_requesthdrs */
/*
 * parse URI into filename and CGI args
 * return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs) // 클라이언트가 요청한 URI(인터넷에서 자원을 나타내는 고유 식별자), 빈값, 빈값
{
    /*
    /gizmo.mp4는 클라이언트가 요청한 URI입니다. URI(Uniform Resource Identifier)란 인터넷에서 자원을 나타내는 고유 식별자를 의미합니다.
    예를 들어, 클라이언트가 http://www.example.com/gizmo.mp4에 대한 요청을 보낼 경우, http://www.example.com는 서버의 주소를 나타내는 URI의 일부분이며, /gizmo.mp4는 요청한 자원을 나타내는 URI의 일부분
    따라서 GET /gizmo.mp4 HTTP/1.1의 첫 번째 줄에서 /gizmo.mp4는 클라이언트가 요청한 URI\
    */
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */ //line:netp:parseuri:isstatic
	strcpy(cgiargs, "");  
    printf("filename1은 %s\n",filename); 
	strcpy(filename, ".");
    printf("filename2은 %s\n",filename); 
	strcat(filename, uri);                           //line:netp:parseuri:endconvert1
    printf("filename3은 %s\n",filename); 
	if (uri[strlen(uri)-1] == '/')                   //line:netp:parseuri:slashcheck
	    strcat(filename, "home.html");               //line:netp:parseuri:appenddefault
        printf("filename4은 %s\n",filename); 
	return 1;
    }
    else {  /* Dynamic content */                        //line:netp:parseuri:isdynamic
	ptr = index(uri, '?');                           //line:netp:parseuri:beginextract
	if (ptr) {
	    strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
        printf("filename5은 %s\n",filename); 
	}
	else 
	    strcpy(cgiargs, "");  //dest는 복사대상 문자열 가리키는 포인터, src는 복사원본 문자열 가리키는 포인터
        printf("filename6은 %s\n",filename); 
	    strcpy(filename, ".");
        printf("filename7은 %s\n",filename); 
	    strcat(filename, uri);                           //line:netp:parseuri:endconvert2
	printf("filename8은 %s\n",filename); 
    return 0;
    }
}
/* $end parse_uri */

/*
 * serve_static - copy a file back to the client 
 */
/* $begin serve_static */
void serve_static(char* method, int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, filetype);    //line:netp:servestatic:getfiletype
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); //line:netp:servestatic:beginserve
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n", filesize);

    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(fd, buf, strlen(buf));    //line:netp:servestatic:endserve

    if (strcasecmp(method, "HEAD")== 0){
        return;
    }
    if (strcasecmp(method, "GET") == 0) { // 11.11
    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0); // filename의 이름을 갖는 파일을 읽기 권한으로 불러온다.
    // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); -> Mmap방법 : 파일의 메모리를 그대로 가상 메모리에 매핑함.
    srcp = (char *)Malloc(filesize); // 11.9 문제 : mmap()과 달리, 먼저 파일의 크기만큼 메모리를 동적할당 해줌.
    Rio_readn(srcfd, srcp, filesize); // rio_readn을 사용하여 파일의 데이터를 메모리로 읽어옴. ->  srcp에 srcfd의 내용을 매핑해줌
    Close(srcfd); // 파일을 닫는다.
    Rio_writen(fd, srcp, filesize);  // 해당 메모리에 있는 파일 내용들을 fd에 보낸다.
    // Munmap(srcp, filesize); -> Mmap() 방법 : free해주는 느낌
    free(srcp); // malloc~free
  }
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
	strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mp4"))
	strcpy(filetype, "video/mp4");

    else
	strcpy(filetype, "text/plain");
}  
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(char *method, int fd, char *filename, char *cgiargs) 
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (strcasecmp(method, "HEAD")==0){
        return;
    }
    if (Fork() == 0) { /* Child */ //line:netp:servedynamic:fork
	/* Real server would set all CGI vars here */
	setenv("QUERY_STRING", cgiargs, 1); //line:netp:servedynamic:setenv
	Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ //line:netp:servedynamic:dup2
	Execve(filename, emptylist, environ); /* Run CGI program */ //line:netp:servedynamic:execve
    }
    Wait(NULL); /* Parent waits for and reaps child */ //line:netp:servedynamic:wait
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    // sprintf()는 C 언어에서 사용되는 함수 중 하나로, 문자열을 생성하는 함수
    // 다른 문자열과 변수들을 조합하여 새로운 문자열을 생성
    // printf() 함수와 유사한 방법으로 사용
    // sprintf() 함수는 출력할 문자열을 char 형태의 버퍼에 저장하며, 이 버퍼의 시작 주소를 반환
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */