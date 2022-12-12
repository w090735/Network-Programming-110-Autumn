// project 1
// shell program
#include <stdlib.h>
#include <stdio.h> // printf, scanf
#include <string.h>
#include <signal.h>
#include <sys/types.h> // child process status
#include <sys/wait.h> // wait
#include <sys/stat.h>
#include <sys/ioctl.h> // ioctl
#include <unistd.h> // fork, execvp
#include <fcntl.h>
#include <vector>
#include <iostream>

using namespace std;

#define LINE_MAX 15000 // max char in one line


// for cmd counting
// ls(0) | cat(1)
// noop(2)
// ls(3) |1
// number(4)
int line_count = 0;
bool hasPipe = false; // exist '|' or '!' in 1 line
bool pipeChar = true; // still has '|' or '!'

// prevent zombie process
void handler(int sig) {
    int status;
    // waitpid(pid, stat_loc, options), WNOHANG: no suspend
    // pid = -1: wait any child process
    while (waitpid(-1, &status, WNOHANG) > 0);
}

// for numbered-pipe
typedef struct Pipe{
    int num; // cmd num that pipe to (input)
    int pfd[2]; // pfd[0]: in, pfd[1]: out
} Pipe;
vector<Pipe> pipeQueue;


void processCmd(char** argv);
void exeCmd(char** argv);
void forkChild(char** argv, Pipe& numPipe, int pipeErr);
void input_parser(char** argv, char* cmd);
void read_numberPipe();
void write_numberPipe(int& cmdNum, char** argv);
void file_redirect(int cmdNum, char** argv);
void env_normal_cmd(int cmdNum, char** argv);


int main(){

    // init env
    // setenv(name, val, overwrite)
    setenv("PATH","bin:.",1);
    // install signal handler to catch signal when child exit
    // SIGCHLD: signal to parent when child terminates
    signal(SIGCHLD, handler);

    // save the std fd num
    int saved_stdin  = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stderr = dup(STDERR_FILENO);

    while(1){

        printf("%% ");
        char cmd[LINE_MAX]={0};

        // get cmd (if get eof or exit, exit shell)
        // max char size = 4096
        // return EOF flag
        if(!cin.getline(cmd, LINE_MAX)){ // LINE_MAX include '\0'
            printf("exit\n");
            break;
        }
        if(strcmp("exit",cmd) == 0){
            break;
        }
        // parse cmd and process
        char* argv[LINE_MAX]={0}; // store argv
        input_parser(argv, cmd);
        processCmd(argv);

        //return std fd
        dup2(saved_stdin , STDIN_FILENO);
        dup2(saved_stdout, STDOUT_FILENO);
        dup2(saved_stderr, STDERR_FILENO);
    }

    close(saved_stdin);
    close(saved_stdout);
    close(saved_stderr);

    return 0;
}


void processCmd(char** argv){
    // loop for ordinary pipe
    hasPipe = false;
    pipeChar = true;
    int cmdNum;
    for(cmdNum=0; pipeChar; cmdNum++){
        // no argv
        if(argv[cmdNum] == NULL) return;

        read_numberPipe();
        write_numberPipe(cmdNum, argv);
    }
    if(pipeChar || argv[0] == NULL){
        return;
    }
    else if(!hasPipe){
        cmdNum = 0;
    }
    else{
        cmdNum--;
    }

    file_redirect(cmdNum, argv);
    env_normal_cmd(cmdNum, argv);

    line_count++;
}


// no wait for write pipe
void forkChild(char** argv, Pipe& numPipe, int pipeErr){

    int pid;
    while((pid = fork())<0){ // make sure fork success
        usleep(1000);
    }

    if(pid == 0){
        // child: exe cmd and redirect stdout(stderr) to pipe
        close(numPipe.pfd[0]);
        dup2(numPipe.pfd[1], STDOUT_FILENO);
        close(numPipe.pfd[1]);
        if(pipeErr){
            dup2(STDOUT_FILENO, STDERR_FILENO);
        }       
        exeCmd(argv);
    }
    else{
        // parent: no need to wait
        line_count++;
    }

}


void exeCmd(char** argv){
    if(execvp(argv[0], argv)<0){
        fprintf(stderr, "Unknown command: [%s].\n", argv[0]);
        exit(1);
    }
    exit(0);
}


// set argv
void input_parser(char** argv, char* cmd){
    // split command to tokens
    int i=0;
    char* pch = strtok(cmd," \n"); // check " " or "\n"
    while(pch!=NULL){
        argv[i] = pch;
        pch = strtok(NULL," \n");
        i++;
    }
}

void read_numberPipe(){
    /*
        read numbered pipe from vector
    */
    for(int i=0; i<pipeQueue.size(); i++){
        if(pipeQueue[i].num == line_count){
            // redirect input from pipe 
            close(pipeQueue[i].pfd[1]);
            dup2(pipeQueue[i].pfd[0], STDIN_FILENO);
            close(pipeQueue[i].pfd[0]);
            pipeQueue.erase(pipeQueue.begin()+i);
            break;
        }
    }
}

void write_numberPipe(int& cmdNum, char** argv){
    /*
        write numbered pipe
    */
    // argv[i]: "|num" ,"!num"
    for(int i=cmdNum; argv[i]!=NULL; i++){
        if(argv[i][0]=='|' || argv[i][0]=='!'){// check numbered pipe
            hasPipe = true;
            pipeChar = true;
            int pipeErr = (argv[i][0]=='!');// check stderr pipe

            // get pipe number. if no number => |1
            int num;
            if(strlen(argv[i])==1){// check ordinary pipe
                num = 1;
            }
            else{
                sscanf(&argv[i][1], "%d", &num);
            }
            argv[i] = NULL; //discard "|n"

            char **leftCmd = &argv[cmdNum];
            cmdNum = i;

            // write cmd output to pipe and add the pipe info to vector
            int existed = 0;
            for(int j=0; j<pipeQueue.size(); j++){
                // if pipe is already existed, write to existed pipe.
                if(pipeQueue[j].num == line_count+num){
                    forkChild(leftCmd, pipeQueue[j], pipeErr);
                    existed = 1;
                    break;
                }
            }

            // if pipe isn't existed, write to a new pipe.
            if(!existed){
                Pipe numPipe;
                numPipe.num = line_count + num;
                if(pipe(numPipe.pfd)<0){
                    fprintf(stderr, "cannot create pipe!\n");
                    exit(1);
                }
                pipeQueue.push_back(numPipe);
                forkChild(leftCmd, numPipe, pipeErr);
            }

            break;
        }
        else{
            pipeChar = false;
        }
    }
}

void file_redirect(int cmdNum, char** argv){
    /*
        file output redirection
    */
    for(int i=cmdNum; argv[i]!=NULL; i++){
        if(strcmp(">",argv[i]) == 0){ 
            int fd = open(argv[i+1], O_RDWR | O_CREAT | O_TRUNC, 0644);
            argv[i] = NULL;   // discard ">"
            argv[i+1] = NULL; // discard filename
            dup2(fd, STDOUT_FILENO);
            break;
        }
    }    
}

void env_normal_cmd(int cmdNum, char** argv){
    /*
        execute command
    */
    // built-in cmd
    if(strcmp("setenv",argv[cmdNum]) == 0){
        setenv(argv[cmdNum+1],argv[cmdNum+2], 1);
    }
    else if(strcmp("printenv",argv[cmdNum]) == 0){
        for(int i=cmdNum+1; argv[i]!=NULL; i++){
            char* s = getenv(argv[i]);
            if(s != NULL){
                printf("%s\n", s);
            }
        }
    }
    else{// normal cmd
        // exe other cmds in PATH
        int pid;
        while((pid = fork())<0){
            usleep(1000);
        }
        
        if(pid == 0){
            // child: exe command
            exeCmd(&argv[cmdNum]);
        }
        else{
            // parent: wait for child
            int status;
            waitpid(pid, &status, 0);
        }

    }    
}
