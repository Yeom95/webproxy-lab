#include "csapp.h"

int main(int argc,char **argv){
    //addrinfo 구조체를 담은 listp,addrinfo 포인터인 p,addrinfo의 그외 특성을 담은 hints 설정
    struct addrinfo *p,*listp,hints;
    //버퍼 설정
    char buf[MAXLINE];
    //addrinfo 반환값인 records,검색 옵션인 flags 설정
    int record,flags;

    if(argc != 2){
        fprintf(stderr, "usage : %s <domain name>\n",argv[0]);
        exit(0);
    }

    memset(&hints,0,sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if((record = getaddrinfo(argv[1],NULL,&hints,&listp)) != 0){
        fprintf(stderr,"getaddrinfo error : %s\n" , gai_strerror(record));
        exit(1);
    }

    flags = NI_NUMERICHOST;

    for(p = listp ; p ; p = p->ai_next){
        Getnameinfo(p->ai_addr,p->ai_addrlen,buf,MAXLINE,NULL,0,flags);
        printf("%s\n",buf);
    }

    Freeaddrinfo(listp);

    exit(0);
}