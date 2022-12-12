#include "server.h"
#include "shell.h" // SIGCHLD_Handler
#include "user.h"
#include "utils.h" // input_parser
#include "pipe.h"

using namespace std; // std::cin std::vector


#define LINE_MAX 15000 // max char in one line

#define MULTI_PROC false

int new_socket;
int argc;

User user;
PipeList pipeList;

void processCmd(char** argv);
void read_numberPipe();
void write_numberPipe(char** argv);
void file_redirect(char** argv);
void env_normal_cmd(char** argv);

// replpace code by include "utils.h" & loop outside function
int exe_npshell(){
    char cmd[LINE_MAX]={0};
    int cin_size;

    memset(cmd, 0, sizeof(cmd));
    cin_size = getline(cmd, LINE_MAX);

    if(strcmp("exit", cmd) == 0){
        return -1;
    }

    input_parser(argv, cmd);
    processCmd(argv);

    return 0;
}

// atoi(const char*), so set argv const
int main(int argc, char const *argv[]){
    // replace code by include "server.h"
    Server server;
    int server_fd;
    int pid;

    int port = htons(atoi(argv[1]));
    server.init(port);
    server_fd = server.getfd();
    
    setenv("PATH", "bin:.", 1);
    signal(SIGCHLD, SIGCHLD_Handler);
    
    // replace code by include "server.h" & "user.h"
    // only 1 user
    while(true){
        user = server.getConn();
        new_socket = user.fd;

        while((pid = fork()) < 0){
            usleep(1000);
        }

        if(pid == 0){
            int status;

            signal(SIGCHLD, SIGCHLD_Handler);
            server.close();

            dup2(new_socket, STDIN_FILENO);
            dup2(new_socket, STDOUT_FILENO);
            dup2(new_socket, STDERR_FILENO);

            while(true){
                char prompt[] = "% ";
                send(new_socket, prompt, strlen(prompt), 0);
                exe_npshell();
            }
            close(new_socket);

            dup2(new_socket, STDIN_FILENO);
            dup2(new_socket, STDOUT_FILENO);
            dup2(new_socket, STDERR_FILENO);

            exit(0);
        }
        else{
            close(new_socket);
        }
    }
    server.close();
    
    return 0;
}

// replace code by recursive call & add global user info
void processCmd(char** argv){
    if(argv[0] == NULL) return;

    if(env_normal_cmd(argv)){
        user.cmdCnt++;
        return;
    }

    read_numberPipe();
    file_redirect(argv);
    write_numberPipe(argv);

    int pid;
    while((pid = fork()) < 0){
        usleep(1000);
    }

    if(pid == 0){
        exeCmd(argv);
    }
    else{
        int status;
        waitpid(pid, &status, 0);
    }

    user.cmdCnt++;
}

// replace code by global user info & "pipe.h"
void read_numberPipe(){
    int index;
    if((index = pipeList.exist(user.cmdCnt)) >= 0){
        pipeList[index].read();
        pipeList.remove(index);
    }
}

// replace code by include "pipe.h"
void write_numberPipe(char** argv){
    for(int i=0; i<argc; i++){
        if(argv[i] == NULL) continue;
        if(argv[i][0] == '|' || argv[i][0] == '!'){
            int pipeErr = (argv[i][0] == '!');

            int num = getNumForPipe(&argv[i][1]);
            argv[i] = NULL; // discard "|n"

            char** leftCmd = &argv[0];
            char** rightCmd = &argv[i+1];

            int index;
            if((index = pipeList.exist(user.cmdCnt+num))){
                pipeList[index].write(leftCmd, pipeErr, MULTI_PROC);
            }
            else{
                Pipe pipe;
                pipe.num = user.cmdCnt + num;
                pipe.from = user;
                pipe.to = user;
                pipe.create();
                pipe.write(leftCmd, pipeErr, MULTI_PROC);
                pipeList.add(pipe);
            }

            user.cmdCnt++;
            processCmd(rightCmd);
            return;
        }
    }
}

// replace code by global user info
void file_redirect(char** argv){
    int argc = 0;

    while(argv[argc] != NULL) argc++;

    for(int i=0; i<argc; i++){
        if(argv[i] == NULL) return;
        if(strcmp(">", argv[i]) == 0){
            int fd = open(argv[i+1], O_RDWR | O_CREAT | O_TRUNC, 0644);
            argv[i] = argv[i+1] = NULL; // discard ">" & filename
            dup2(fd, STDOUT_FILENO);
            break;
        }
    }
}

// replace code by global user info
int env_normal_cmd(char** argv){
    if(strcmp("setenv", argv[0]) == 0){
        if(argv[1] == NULL || argv[2] == NULL){
            char err[] = "usage: setenv <NAME> <VALUE>\n";
            send(new_socket, err, strlen(err), 0);
        }
        else{
            setenv(argv[1], argv[2], 1);
        }
        return 1;
    }
    else if(strcmp("printenv", argv[0]) == 0){
        if(argv[1] == NULL){
            char err[] = "usage: printenv <NAME>\n";
            send(new_socket, err, strlen(err), 0)''
        }
        else{
            for(int i=1; argv[i]!=NULL; i++){
                char* s = getenv(argv[i]);
                if(s != NULL){
                    s[strlen(s)] = '\n';
                    s[strlen(s)+1] = '\0';
                    send(new_socket, s, strlen(s), 0);
                }
            }
        }
        return 1;
    }
    else return 0;
}
