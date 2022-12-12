#include "utils.h" // getline input_parser argvToStr getNumForPipe
#include "server.h" // init: socket/bind/listen getConn: accept/set user info close: close serverfd
#include "user.h"
#include "shell.h" // exeCmd
#include "pipe.h"

using namespace std; // std::vector


#define LINE_MAX 15000 // max char in one line

#define MULTI_PROC false // Pipe.write

int new_socket;
int DROP = open("/dev/null", O_RDWR);
int argc; // access by read_userPipe write_userPipe write_numberPipe
char origin_cmd[LINE_MAX];

UserList userList;
PipeList pipeList;

void broadcast(char text[256]);
void processCmd(char** argv, User& user);
void exe_npshell(User& user);
void read_numberPipe(User user);
void read_userPipe(char** argv, User user);
void write_numberPipe(char** argv, User user);
void write_userPipe(char** argv, User user);
void env_normal_cmd(int cmdNum, char** argv, User& user);

int exe_npshell(User& user){
    char cmd[LINE_MAX]={0};
    int cin_size;
        
    // get cmd
    memset(cmd, 0, sizeof(cmd));
    cin_size = getline(cmd, LINE_MAX); // ignore '\r'

    if(strcmp("exit", cmd) == 0){ // exit
        return -1;
    }

    strncpy(origin_cmd, cmd, LINE_MAX);
    // parse cmd and process
    char* argv[LINE_MAX]={0};
    input_parser(argv, cmd);
    processCmd(argv, user);

    return 0;
}

int main(int argc, char const *argv[]){
    // replace code by include "server.h"
    Server server;
    int server_fd;
    int pid;
    int fd=0, nfds;
    fd_set rfds, afds; // rfds: ready queue, afds: current sockets

    int port = htons(atoi(argv[1]));
    server.init(port);
    server_fd = server.getfd();
    setenv("PATH", "bin:.",1);

    // set afds
    nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(server_fd, &afds);

    while(true){
        dup2(DROP, fileno(stdin));
        dup2(DROP, fileno(stdout));
        dup2(DROP, fileno(stderr));

        memcpy(&rfds, &afds, sizeof(rfds));
        while(select(nfds, &rfds, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0) < 0 && errno == EINTR);

        fd = 0;
        while(!FD_ISSET(fd, &rfds)){
            fd = (fd + 1) % nfds;
        }

        if(fd == server_fd){
            // replace code by include "server.h", "user.h"
            User user;
            int curId;

            user = server.getConn();
            new_socket = user.fd;
            curId = userList.add(user); // (new) user.index = [0..n-1], (old) user.id = [1...n]
            user.env["PATH"] = "bin:.";

            FD_SET(new_socket, &afds);

            char welcome[] = "****************************************\n"
                             "** Welcome to the information server. **\n"
                             "****************************************\n";
            char login[256];
            char prompt[] = "% ";
            sprintf(login, "*** User '%s' entered from %s:%d. ***\n", user.name, user.ip, user.port);

            send(new_socket, welcome, strlen(welcome), 0);
            broadcast(login);
            send(new_socket, prompt, strlen(prompt), 0);
        }
        else{
            // replace code by include "user.h"
            int user_index = 0;
            int status;
            map<string, string> env;

            while(userList[user_index].fd != fd){
                user_index = (user_index + 1) % MAX_USER;
            }

            // inilialize env
            clearenv();
            env = userList[user_index].env;

            for(map<string, string>::iterator it=env.begin(); it!=env.end(); it++){
                setenv(it->first.c_str(), it->second.c_str(), 1);
            }

            // handle child process exit state
            signal(SIGCHLD, SIGCHLD_Handler);

            dup2(fd, fileno(stdin));
            dup2(fd, fileno(stdout));
            dup2(fd, fileno(stderr));

            // replace code by include "user.h"
            status = exe_npshell(userList[user_index]);
            if(status == -1){ // exit
                char logout[256];
                sprintf(logout, "*** User '%s' left. ***\n", userList[user_index].name);
                broadcast(logout);
                close(fd);
                for(int i=0; i<pipeList.size(); i++){
                    if(pipeList[i].to.index == userList[user_index].index || pipeList[i].from.index == userList[user_index].index+1){
                        close(pipeList[i].pfd[0]);
                        close(pipeList[i].pfd[1]);
                        pipeList.remove(i);
                        i--;
                    }
                }
                userList.remove(user_index);
                FD_CLR(fd, &afds);
                close(fileno(stdin));
                close(fileno(stdout));
                close(fileno(stderr));
                continue;
            }

            char prompt[] = "% ";
            send(fd, prompt, strlen(prompt), 0);
        }
    }
    
    close(server_fd);
    return 0;
}

// replace by include "user.h"
void broadcast(char text[256]){
    for(int i=0; i<MAX_USER; i++){
        if(userList[i].exist){
            send(userList[i].fd, text, strlen(text), 0);
        }
    }
}

void processCmd(char** argv, User& user){
    // replace by loop inside
    if(argv[0] == NULL) return;

    if(env_normal_cmd(user, argv)){
        user.cmdCnt++;
        return;
    }

    read_numberPipe(user);
    read_userPipe(argv, user);
    write_userPipe(argv, user);
    write_numberPipe(argv, user);

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

// replace by loop inside function & "pipe.h"
void read_numberPipe(User user){
    int index;

    if((index = pipeList.exist(user.cmdCnt)) >= 0){
        pipeList[index].read();
        pipeList.remove(index);
    }
}

// replace code by loop inside function & "pipe.h"
void read_userPipe(char** argv, User user){
    argc = 0;
    while(argv[argc++] != NULL);

    for(int i=0; i<argc; i++){
        if(argv[i] == NULL) continue;
        if(argv[i][0] == '<'){
            // get user number
            int num = getNumForPipe(&argv[i][1]);
            if(num <= 0 || num > 30 || !userList[num-1].exist){
                char err[256];
                dup2(DROP, fileno(stdin));
                sprintf(err, "*** Error: user #%d does not exist yet. ***\n", num); 
                send(user.fd, err, strlen(err), 0);
                argv[i] = NULL; // discard "<n"
                break;
            }
            else{
                User sender = userList[num-1];
                int index;

                if((index = pipeList.exist(sender, user)) >= 0){
                    char inform[256];
                    memset(inform, 0, sizeof(inform));
                    sprintf(inform, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n", user.name, user.index+1, sender.name, sender.index+1, origin_cmd);
                    broadcast(inform);
                    close(pipeList[index].pfd[1]);
                    dup2(pipeList[index].pfd[0], fileno(stdin));
                    close(pipeList[index].pfd[0]);
                    pipeList.remove(index);
                    argv[i] = NULL; // discard >n
                }
                else{
                    char err[256];
                    memset(err, 0, sizeof(err));
                    dup2(DROP, fileno(stdin));
                    sprintf(err, "*** Error: the pipe #%d->#%d does not exist yet. ***\n", sender.index+1, user.index+1);
                    send(user.fd, err, strlen(err), 0);
                    argv[i] = NULL; // discard >n
                }
            }
        }
    }
}

// replace by loop inside function & "pipe.h"
void write_numberPipe(char** argv, User user){
    // argv[i]: "|num" ,"!num"
    for(int i=0; i<argc; i++){
        if(argv[i] == NULL) continue;
        if(argv[i][0] == '|' || argv[i][0] == '!'){ // check numbered pipe
            int pipeErr = (argv[i][0]=='!'); // check stderr pipe

            // get pipe number. if no number => |1
            int num = getNumForPipe(&argv[i][1]);
            argv[i] = NULL; // discard "|n"

            char **leftCmd = &argv[0];
            char** rightCmd = &argv[i+1];

            // write cmd output to pipe and add the pipe info to vector
            int index;
            if((index = pipeList.exist(user.cmdCnt+num)) >= 0){
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
            processCmd(rightCmd, user);
            return;
        }
    }
}

// replace code by loop inside function & "pipe.h"
void write_userPipe(char** argv, User user){
    // write user pipe
    for(int i=0; i<argc; i++){
        if(argv[i] == NULL) continue;
        if(strcmp(">", argv[i]) == 0){ // file redirect
            int fd = open(argv[i+1], O_RDWR | O_CREAT | O_TRUNC, 0644);
            argv[i] = argv[i+1] = NULL; // discard ">" & filename
            dup2(fd, STDOUT_FILENO);
            break;
        }
        if(argv[i][0] == '>'){
            int num = getNumForPipe(&argv[i][1]);
            if(num <= 0 || num > 30 || !userList[num-1].exist){
                char err[256];
                memset(err, 0, sizeof(err));
                dup2(DROP, fileno(stdout));
                sprintf(err, "*** Error: user #%d does not exist yet. ***\n", num); 
                send(user.fd, err, strlen(err), 0);
                argv[i] = NULL; // discard >n
                break;
            }
            else{
                User receiver = userList[num-1];
                int index;

                if((index = pipeList.exist(user, receiver)) >= 0){
                    char err[256];
                    dup2(DROP, fileno(stdout));
                    sprintf(err, "*** Error: the pipe #%d->#%d already exists. ***\n", user.index+1, receiver.index+1);
                    send(user.fd, err, strlen(err), 0);
                    argv[i] = NULL; // discard ">n"
                }
                else{
                    char inform[256];
                    Pipe pipe;

                    memset(inform, 0, sizeof(inform));
                    sprintf(inform, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", user.name, user.index+1, origin_cmd, receiver.name, receiver.index+1);
                    broadcast(inform);

                    pipe.num = -1;
                    pipe.to = receiver;
                    pipe.from = user;
                    pipe.create();
                    close(pipe.pfd[0]);
                    dup2(pipe.pfd[1], fileno(stdout));
                    close(pipe.pfd[1]);
                    pipeList.add(pipe);
                    argv[i] = NULL; // discard ">n"
                    break;
                }
            }
        }
    }
}

// replace by no cmdNum
int env_normal_cmd(User& user, char** argv){
    if(strcmp("setenv", argv[0]) == 0){
        if(argv[1] == NULL || argv[2] == NULL){
            char err[] = "usage: setenv <NAME> <VALUE>\n";
            send(user.fd, err, strlen(err), 0);
        }
        else{
            setenv(argv[1], argv[2], 1);
            user.env[argv[1]] = argv[2];
        }
        return 1;
    }
    else if(strcmp("printenv", argv[0]) == 0){
        if(argv[1] == NULL){
            char err[] = "usage: printenv <NAME>\n";
            send(user.fd, err, strlen(err), 0);
        }
        else{
            for(int i=1; argv[i]!=NULL; i++){
                char* s = getenv(argv[i]);
                if(s != NULL){
                    s[strlen(s)] = '\n';
                    s[strlen(s)+1] = '\0';
                    send(user.fd, s, strlen(s), 0);
                }
            }
        }
        return 1;
    }
    else if(strcmp("who", argv[0]) == 0){
        char col[] ="<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
        char user_data[256];
        send(user.fd, col, strlen(col), 0);
        for(int i=0; i<MAX_USER; i++){
            User currU = userList[i];
            if(currU.exist){
                if(currU.index == user.index){
                    sprintf(user_data, "%d\t%s\t%s:%d\t<-me\n", user.index+1, user.name, user.ip, user.port);
                }
                else{
                    sprintf(user_data, "%d\t%s\t%s:%d\n", currU.index+1, currU.name, currU.ip, currU.port);
                }
                send(user.fd, user_data, strlen(user_data), 0);
            }
        }
        return 1;
    }
    else if(strcmp("yell", argv[0]) == 0){
        if(argv[1] == NULL){
            char err[] = "usage: yell <MESSAGE>\n";
            send(user.fd, err, strlen(err), 0);
        }
        else{
            char msg[256];
            sprintf(msg, "*** %s yelled ***: ", user.name);
            argvToStr(msg, &argv[1]);
            broadcast(msg);
        }
        return 1;
    }
    else if(strcmp("tell", argv[0]) == 0){
        if(argv[1] == NULL || argv[2] == NULL){
            char err[] = "usage: tell <ID> <MESSAGE>\n";
            send(user.fd, err, strlen(err), 0);
        }
        else{
            int index = atoi(argv[1]);
            if(index <= 0){
                char err[] = "usage: tell <ID> <MESSAGE>\n";
                send(user.fd, err, strlen(err), 0);
            }
            else if(index > 30){
                char err[] = "usage: <ID> range 1~30\n";
                send(user.fd, err, strlen(err), 0);
            }
            else{
                User receiver = userList[index-1];
                char msg[256];
                if(receiver.exist){
                    sprintf(msg, "*** %s told you ***: ", user.name);
                    argvToStr(msg, &argv[2]);
                    send(receiver.fd, msg, strlen(msg), 0);
                }
                else{
                    char err[256];
                    sprintf(err, "*** Error: user #%s does not exist yet. ***\n", argv[cmdNum+1]);
                    send(user.fd, err, strlen(err), 0);
                }
            }
        }
        return 1;
    }
    else if(strcmp("name", argv[0]) == 0){
        if(argv[1] == NULL){
            char err[] = "usage: name <NAME>\n";
            send(user.fd, err, strlen(err), 0);
        }
        else{
            char name[SIZE_NAME];
            char rename[256];
            argvToStr(name, &argv[1]);

            for(int i=0; i<MAX_USER; i++){
                User currU = userList[i];
                if(currU.exist && strcmp(currU.name, name) == 0){
                    char err[64];
                    sprintf(err, "*** User '%s' already exists. ***\n", name);
                    send(user.fd, err, strlen(err), 0);
                    return 1;
                }
            }

            strncpy(user.name, name, SIZE_NAME);
            sprintf(rename, "*** User from %s:%d is named '%s'. ***\n", user.ip, user.port, user.name);
            broadcast(rename);
        }
        return 1;
    }
    else return 0; // not built-in cmd
}
