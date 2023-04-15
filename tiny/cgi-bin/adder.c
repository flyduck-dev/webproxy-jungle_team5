/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1=0, n2=0;

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL) {
    p = strchr(buf, '&');
    *p = '\0';
    //strcpy(arg1, buf);
    //strcpy(arg2, p+1);
    //n1 = atoi(arg1);
    //n2 = atoi(arg2);

    //tack 11.11
    sscanf(buf, "first=%d", &n1);
    sscanf(p + 1, "second=%d", &n2);

// buf 변수는 getenv("QUERY_STRING") 함수를 통해 받은 문자열 전체를 저장
// 이 문자열을 '&'를 기준으로 분리하여 p 포인터가 가리키는 위치를 널 문자로 바꾸어 앞부분과 뒷부분
// 그러면 buf 포인터가 가리키는 문자열에서 '&' 문자 이전까지가 arg1 변수
// '&' 문자 이후부터 문자열 끝까지가 arg2 변수에 저장
// sscanf(buf, "first=%d", &n1) 코드는 (buf는 널문자까지 읽음)
// buf 변수가 가리키는 문자열에서 "first=" 뒤에 오는 정수 값을 %d 포맷으로 읽어서 n1 변수에 저장
// sscanf(p + 1, "second=%d", &n2) 코드는
// p + 1 포인터가 가리키는 문자열에서 "second=" 뒤에 오는 정수 값을 %d 포맷으로 읽어서 n2 변수에 저장
// buf가 "first=1&second=2"인 경우,
// p 변수에는 & 문자를 가리키는 포인터 저장.
// p가 가리키는 위치를 \0으로 덮어 씌움으로써 buf 문자열을 두 개의 문자열로 분리
// 각각의 문자열에서 sscanf 함수를 사용하여 n1과 n2에 1과 2를 저장
}

  /* Make the response body */
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", 
	content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);
  
  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);
  exit(0);
}
/* $end adder */
