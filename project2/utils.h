/* C libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h> // errno
/* C++ libraries */
#include <vector>
#include <map>
#include <string>
using namespace std;

/* for debug */
#define ERR(msg) perror(msg), exit(1)


/* define const global */
#define true 1
#define false 0

/* for user input */
int getline(char* buf, int buf_size){
	int ret, size = 0;
	char c;
	while(ret = read(STDIN_FILENO, &c, 1)){
		// interrupt by signal
		if(ret < 0 && errno == EINTR)
			continue;

		// read err or ctrl-D pressed (EOF)
		if(ret < 0 || c == '\4')
			return -1;
		
		// ignore '\r', return when get '\n' or until max size
		if(c == '\r'){
			continue;
		}
		else if(c == '\n' || size >= buf_size-1){
			buf[size] = '\0';
			break;
		}
		else{
			buf[size++] = c;
		}
	}
	return size;
}


/* for string processing */
void argvToStr(char* dest, char** argv, int argc){
	for(int i=0;i<argc;i++){
		if(argv[i]!=NULL){
			strcat(dest, argv[i]);
			strcat(dest, " ");
		}
	}
	dest[strlen(dest)-1]='\0';
}

void argvToStr(char* dest, char** argv){
	int argc = 0;
	while(argv[argc]!=NULL) argc++;
	argvToStr(dest, argv, argc);
}

void input_parser(char** argv, char* src){
	int i=0;
	char* pch = strtok(src," \n\r");
	while(pch!=NULL){
		argv[i] = pch;
		pch = strtok(NULL," \n\r");
		i++;
	}
}

/* to get number for number/user pipe */
int getNumForPipe(char* str){
	int num;
	if(str[0] == '\0'){
		num = 1;
	}
	else{
		num = atoi(str);
	}
	return num;
}
