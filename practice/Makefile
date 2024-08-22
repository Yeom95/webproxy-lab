#gcc를 컴파일러로 사용
CC = gcc
#C 컴파일러를 최적화레벨 2,디버깅 포함, 경고메세지 출력하여 디렉토리를 포함 경로에 추가하여 "상위 폴더에서" 헤더파일을 찾게한다
CFLAGS = -O2 -g -Wall -I ..
#POSIX 스레드 라이브러리를 링크
LIB = -lpthread

#실행 파일 이름을 변수화
TARGET = hostinfo

#makefile 기본 실행시 hostinfo cgi 실행
all: $(TARGET) #cgi

$(TARGET): $(TARGET).o ../csapp.o
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).o ../csapp.o $(LIB)

$(TARGET).o : $(TARGET).c
	$(CC) $(CFLAGS) -c $(TARGET).c

../csapp.o : ../csapp.c
	$(CC) $(CFLAGS) -c ../csapp.c -o ../csapp.o

#cgi가 cgi-bin 디렉토리로 이동하여 그 안에서 또 다른 Makefile을 실행한다
#cgi:
#	(cd cgi-bin;make)

clean:
	rm -f *.o $(TARGET) *~
#	(cd cgi-bin;make clean)
	rm -f ../csapp.o