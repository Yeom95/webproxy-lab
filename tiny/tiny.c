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
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  //입력한거 이상으로 넣거나 입력 안하면 종료
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  //listen 소켓 생성
  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    //accept하면 connect 소켓 생성
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

/*한 개의 HTTP 트랜잭션 처리*/
void doit(int fd){
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
  char filename[MAXLINE],cgiargs[MAXLINE];
  rio_t rio;

  /*Request line&header를 읽는다*/
  Rio_readinitb(&rio,fd);
  Rio_readlineb(&rio,buf,MAXLINE);
  printf("Request headers:\n");
  printf("%s",buf);
  //buf에 있는 값을 각각 method,uri,version에 저장
  sscanf(buf,"%s %s %s",method,uri,version);
  //strcasecmp는 값이 같을때만 0을 리턴
  //즉, GET을 제외한 나머지 명령일 경우 구현하지 않았으므로 해당 메세지를 보내고 리턴
  if(strcasecmp(method,"GET")){
    clienterror(fd,method,"501","Not implemented","Tiny does not implement this method");
    return;
  }

  /*GET에서 URI 파싱*/
  is_static = parse_uri(uri,filename,cgiargs);
  //stat은 정상적으로 파일 조회하면 sbuf에 주입받을 주소를 넣고 0 반환,오류라면 -1을 반환한다
  if(stat(filename,&sbuf) < 0){
    clienterror(fd,filename,"404","Not found","Tiny couldn't find this file");
    return;
  }
  /*
  S_ISREG : 일반 파일인지 여부
  S_IRUSR : 소유자만 읽기 가능
  S_IXUSR :소유자만 실행 가능
  */

  /*정적 서버*/
  if(is_static){
    //sbuf.st_mode=>파일의 종류 및 접근권한
    //일반 파일이 아니거나 읽기 불가능
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd,filename,"403","Forbidden","Tiny couldn't read the file");
      return;
    }
    serve_static(fd,filename,sbuf.st_size);
  }
  /*동적 서버*/
  else{
    //일반 파일이 아니거나 실행 불가능
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
      clienterror(fd,filename,"403","Forbidden","Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd,filename,cgiargs);
  }
}

/*에러 메세지를 클라이언트에게 보낸다*/
void clienterror(int fd,char *cause,char *errnum,char *shortmsg,char *longmsg){
  char buf[MAXLINE],body[MAXBUF];

  /*HTTP 응답 body 생성*/
  sprintf(body,"<html><title>Tiny Error</title>");
  sprintf(body,"%s<body bgcolor=""ffffff"">\r\n",body);
  sprintf(body,"%s%s: %s\r\n",body,errnum,shortmsg);
  sprintf(body,"%s<p>%s: %s\r\n",body,longmsg,cause);
  sprintf(body,"%s<hr><em>The Tiny Web Server</em>\r\n",body);

  /*HTTP 응답 출력*/
  sprintf(buf,"HTTP/1.0 %s %s\r\n",errnum,shortmsg);
  Rio_writen(fd,buf,strlen(buf));
  sprintf(buf,"Content-type: text/html\r\n");
  Rio_writen(fd,buf,strlen(buf));
  sprintf(buf,"Content-length: %d\r\n\r\n",(int)strlen(body));
  Rio_writen(fd,buf,strlen(buf));
  Rio_writen(fd,body,strlen(body));
}

/*요청 헤더를 읽은 뒤 무시한다*/
void read_requesthdrs(rio_t *rp){
  char buf[MAXLINE];

  Rio_readlineb(rp,buf,MAXLINE);
  while(strcmp(buf,"\r\n")){
    Rio_readlineb(rp,buf,MAXLINE);
    printf("%s",buf);
  }

  return;
}

/*HTTP URI를 분석한다*/
int parse_uri(char *uri,char *filename,char *cgiargs){
  char *ptr;

  /*정적 콘텐츠*/
  if(!strstr(uri,"cgi-bin")){
    strcpy(cgiargs,"");
    strcpy(filename,".");
    strcat(filename,uri);
    if(uri[strlen(uri)-1] == '/')
      strcat(filename,"home.html");
    return 1;
  }
  /*동적 콘텐츠*/
  else{
    ptr = index(uri,'?');
    if(ptr){
      strcpy(cgiargs,ptr+1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs,"");
    strcpy(filename,".");
    strcat(filename,uri);
    return 0;
  }
}

/*정적 컨텐츠를 클라이언트에게 서비스한다*/
void serve_static(int fd,char *filename,int filesize){
  int srcfd;
  char *srcp,filetype[MAXLINE],buf[MAXLINE];

  /*클라이언트에게 response 헤더를 전달*/
  get_filetype(filename,filetype);
  sprintf(buf,"HTTP/1.0 200 OK\r\n");
  sprintf(buf,"%sServer: Tiny Web Server\r\n",buf);
  sprintf(buf,"%sConnection: close\r\n",buf);
  sprintf(buf,"%sContent-length: %d\r\n",buf,filesize);
  sprintf(buf,"%sContent-type: %s\r\n\r\n",buf,filetype);
  Rio_writen(fd,buf,strlen(buf));
  printf("Response headers:\n");
  printf("%s",buf);

  /*클라이언트에게 response body를 전달*/
  srcfd = Open(filename,O_RDONLY,0);
  srcp = Mmap(0,filesize,PROT_READ,MAP_PRIVATE,srcfd,0);
  Close(srcfd);
  Rio_writen(fd,srcp,filesize);
  Munmap(srcp,filesize);
}

/*filename에서 file 생성*/
void get_filetype(char *filename,char *filetype){
  if(strstr(filename,".html"))
    strcpy(filetype,"text/html");
  else if(strstr(filename,".gif"))
    strcpy(filetype,"image/gif");
  else if(strstr(filename,".png"))
    strcpy(filetype,"image/png");
  else if(strstr(filename,".jpg"))
    strcpy(filetype,"image/jpeg");
  else
    strcpy(filetype,"text/plain");
}

void serve_dynamic(int fd,char *filename,char *cgiargs){
  char buf[MAXLINE], *emptylist[] = {NULL};

  /*HTTP response의 첫번쨰 부분 리턴*/
  sprintf(buf,"HTTP/1.0 200 OK\r\n");
  Rio_writen(fd,buf,strlen(buf));
  sprintf(buf,"Server: Tiny Web Server\r\n");
  Rio_writen(fd,buf,strlen(buf));

  if(Fork() == 0){
    setenv("QUERY-STRING",cgiargs,1);
    Dup2(fd,STDOUT_FILENO);
    Execve(filename,emptylist,environ);
  }
  Wait(NULL);
}


