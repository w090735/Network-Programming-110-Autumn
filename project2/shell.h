#include "utils.h"

void exeCmd(char** argv){
	if(execvp(argv[0], argv)<0){
		dprintf(STDERR_FILENO, "Unknown command: [%s].\n", argv[0]);
		exit(1);
	}
	exit(0);
}


// prevent zombie process
void SIGCHLD_Handler(int signo) {
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0);
}
