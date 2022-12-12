#include "utils.h" // getline input_parser argvToStr getNumForPipe
#include "user.h"
#include "pipe.h"
#include "shell.h" // exeCmd SIGCHLD_Handler
#include "server.h" // init: socket/bind/listen getConn: accept/set user info close: close serverfd
#include "shm.h" // attach
#include "fifo.h" // create/open/delete FIFO

#define LINE_MAX 16384
#define MAX_MSG 2048

// SHM KEY
#define SHM_KEY_USERLIST 0x8888
#define SHM_KEY_MSG 0x7777

// custom signal
#define SIGMSG SIGUSR1 // sig for send msg to user
#define SIGRUP SIGUSR2 // sig for read user pipe

#define MULTI_PROC true

Server server;
PipeList pipeList;
int MAIN_PID = getpid();
int NULLOUT = open("/dev/null", O_RDWR);

// for user pipe
int userPipeTable[MAX_USER] = {0};

// set user list shm
SharedMemory userList_SHM(SHM_KEY_USERLIST, sizeof(UserList));
UserList& userList = *(UserList*)userList_SHM.attach();

// set msg shm
SharedMemory msg_SHM(SHM_KEY_MSG, sizeof(MAX_MSG));
char* msg = (char*)msg_SHM.attach();

char origin_cmd[LINE_MAX]={0};
int argc;

void processCmd(char** argv, User& me);
int env_normal_cmd(User& me, char** argv);
void broadcast();
void clearUserPipe(User user);
int exe_npshell(User& user);
void read_numberPipe(User me);
void read_userPipe(char** argv, User me);
void write_userPipe(char** argv, User me);
void write_numberPipe(char** argv, User me);

// signal handler
static void SIGRUP_Handler(int sig, siginfo_t *s_t, void *p);
void SIGMSG_Handler(int signo);
void SIGINT_Handler(int signo);

int exe_npshell(User& user){
    // get cmd (if get eof or exit, exit shell)
    char cmd[LINE_MAX]={0};
    
    if(getline(cmd, LINE_MAX)<0){
        printf("exit\n");
        strcpy(cmd, "exit");
    }
    if(strcmp("exit",cmd)==0){
        return -1;
    }

    // parse cmd and process
    strcpy(origin_cmd, cmd);
    char* argv[LINE_MAX]={0};
    input_parser(argv, cmd);
    processCmd(argv, user);
    
    return 0;
}

int main(int argc, char** argv){

    if(argc < 2){
        printf("usage: ./np_multi_proc <port>\n");
        return 1;
    }

    // set server
    int port = atoi(argv[1]);
    server.init(port);

    // init env
    clearenv();
    setenv("PATH", "bin:.",1);

    // init user_pipe
    mkdir("user_pipe", 0755);

    // set sig
    signal(SIGCHLD, SIGCHLD_Handler);
    signal(SIGMSG,  SIGMSG_Handler);
    signal(SIGINT,  SIGINT_Handler);
    struct sigaction sa;    
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO; // for sigval
    sa.sa_sigaction = SIGRUP_Handler;
    if (sigaction(SIGRUP, &sa, NULL) < 0){
        ERR("sigaction");
    }

    while(1){
        
        // for new connection
        usleep(2000);
        int index = userList.add(server.getConn());
        User& user = userList[index];

        int pid = fork();
        if(pid < 0){
            ERR("fork");
        }
        else if(pid == 0){
            int status;
            
            // child: close server
            server.close();
            signal(SIGINT, SIG_DFL);
            
            // save pid
            user.pid = getpid();
            
            // redirect STDIN/STDOUT/STDERR to client
            dup2(user.fd, STDIN_FILENO);
            dup2(user.fd, STDOUT_FILENO);
            dup2(user.fd, STDERR_FILENO);
            
            printf("****************************************\n");
            printf("** Welcome to the information server. **\n");
            printf("****************************************\n");

            // broadcast login
            sprintf(msg, "*** User '%s' entered from %s:%d. ***", user.name, user.ip, user.port);
            broadcast();

            while(1){
                printf("%% ");
                fflush(stdout);

                status = exe_npshell(user);
                if(status == -1){
                    // broadcast logout
                    sprintf(msg, "*** User '%s' left. ***", user.name);
                    broadcast();
                    // clear & close
                    close(user.fd);
                    clearUserPipe(user);
                    userList.remove(user.index);
                }
                
                // redirect STDIN/STDOUT/STDERR to client
                dup2(user.fd, STDIN_FILENO);
                dup2(user.fd, STDOUT_FILENO);
                dup2(user.fd, STDERR_FILENO);
            }

            exit(0);
        }
        else{
            //parent: close client
            close(user.fd);
        }

    }

    return 0;
}


void processCmd(char** argv, User& me){

    if(argv[0]==NULL) return;

    /*
        run built-in function
    */
    if(env_normal_cmd(me, argv)){
        me.cmdCnt++;
        return;
    }

    read_numberPipe(me);
    read_userPipe(argv, me);
    write_userPipe(argv, me);
    write_numberPipe(argv, me);

    /*
        execute other cmds in PATH
    */
    int pid;
    while((pid = fork())<0){
        usleep(1000);
    }
    
    if(pid == 0){
        // child: exe command
        exeCmd(argv);
    }
    else{
        // parent: wait for child
        int status;
        waitpid(pid, &status, 0);
    }

    me.cmdCnt++;

}


int env_normal_cmd(User& me, char** argv){

    if(strcmp("setenv", argv[0]) == 0){
        if(argv[1] == NULL || argv[2] == NULL){
            printf("usage: setenv <NAME> <VALUE>\n");
        }
        else{
            setenv(argv[1],argv[2],1);
        }
        return 1;
    }

    else if(strcmp("printenv", argv[0]) == 0){
        if(argv[1] == NULL){
            printf("usage: printenv <NAME>\n");
        }
        else{
            for(int i=1; argv[i]!=NULL; i++){
                char* s = getenv(argv[i]);
                if(s!=NULL){
                    printf("%s\n", s);
                }
            }
        }
        return 1;
    }

    else if(strcmp("who",argv[0])==0){
        printf("<ID>\t<nickname>\t<IP:port>\t<indicate me>\n");
        for(int i=0;i<MAX_USER;i++){
            User user = userList[i];
            if(user.exist){
                printf("%d\t%s\t%s:%d\t%s\n", i+1, user.name, user.ip, user.port, (me==user)?"<-me":"");
            }
        }
        return 1;
    }

    else if(strcmp("yell",argv[0])==0){
        if(argv[1]==NULL){
            printf("usage: yell <MESSAGE>\n");
        }
        else{
            sprintf(msg, "*** %s yelled ***: ", me.name);
            argvToStr(msg, &argv[1]);
            broadcast();
        }
        return 1;
    }

    else if(strcmp("tell",argv[0])==0){
        if(argv[1]==NULL){
            printf("usage: tell <ID> <MESSAGE>\n");
        }
        else{
            int index = atoi(argv[1]);
            if(index==0){
                printf("usage: tell <ID> <MESSAGE>\n");
            }
            else{
                User user = userList[index-1];
                if(user.exist){
                    sprintf(msg, "*** %s told you ***: ", me.name);
                    argvToStr(msg, &argv[2]); // cat argv to 1 string
                    kill(user.pid, SIGMSG);
                }
                else{
                    printf("*** Error: user #%d does not exist yet. ***\n", index);
                }
            }
        }
        return 1;
    }

    else if(strcmp("name",argv[0])==0){
        if(argv[1]==NULL){
            printf("usage: name <NAME>\n");
        }
        else{
            char name[SIZE_NAME]={0};
            argvToStr(name, &argv[1]); // cat argv to 1 string

            for(int i=0;i<MAX_USER;i++){
                User user = userList[i];
                if(user.exist && strcmp(user.name, name)==0){
                    printf("*** User '%s' already exists. ***\n", user.name);
                    return 1;
                }
            }

            strncpy(me.name, name, SIZE_NAME);
            sprintf(msg, "*** User from %s:%d is named '%s'. ***", me.ip, me.port, me.name);
            broadcast();
        }
        return 1;
    }
    
    //not built-in cmd
    else return 0;
}


void broadcast(){
    kill(MAIN_PID, SIGMSG);
    for(int i=0;i<MAX_USER;i++){
        User user = userList[i];
        if(user.exist && user.pid!=-1){
            kill(user.pid, SIGMSG);
        }
    }
}


void clearUserPipe(User user){
    //clear user_pipe fifo
    for(int i=0;i<MAX_USER;i++){
        FIFO fifo(user.index, i);
        if(fifo.exist()){
            fifo.remove();
        }
    }
    for(int i=0;i<MAX_USER;i++){
        FIFO fifo(i, user.index);
        if(fifo.exist()){
            fifo.remove();
        }
    }
}

void read_numberPipe(User me){
    /*
        read number-pipe from vector
    */
    int index;
    if((index = pipeList.exist(me.cmdCnt)) >= 0){
        //if pipe exist, read from pipe.
        pipeList[index].read();
        pipeList.remove(index);
    }
}

void read_userPipe(char** argv, User me){
    /*
        read from user pipe
    */
    argc = 0;
    while(argv[argc++]!=NULL);
    
    for(int i=0;i<argc;i++){
        if(argv[i]==NULL) continue;
        if(argv[i][0]=='<'){
            // get user number       
            int num = getNumForPipe(&argv[i][1]);
            User user = userList[num-1];

            // check user
            if(!user.exist){
                dup2(NULLOUT, STDIN_FILENO);
                dprintf(STDERR_FILENO, "*** Error: user #%d does not exist yet. ***\n", num);
                argv[i] = NULL; // discard "<n"
                break;
            }

            // check user pipe fifo
            FIFO fifo(user.index, me.index);
            if(!fifo.exist()){
                dup2(NULLOUT, STDIN_FILENO);
                dprintf(STDERR_FILENO, "*** Error: the pipe #%d->#%d does not exist yet. ***\n", user.index+1, me.index+1);
                argv[i] = NULL; // discard "<n"
                break;
            }

            // broadcast
            sprintf(msg, "*** %s (#%d) just received from %s (#%d) by '%s' ***", me.name, me.index+1, user.name, user.index+1, origin_cmd);
            broadcast();
            
            // redirect input from user pipe
            int fd = userPipeTable[user.index];
            userPipeTable[user.index] = 0;
            dup2(fd, STDIN_FILENO);
            close(fd);

            // remove fifo
            fifo.remove();

            argv[i] = NULL; // discard "<n"
            break;
        }
    }
}

void write_userPipe(char** argv, User me){
    /*
        write to user pipe or file output redirection
    */
    for(int i=0; i<argc; i++){
        if(argv[i] == NULL) continue;
        if(strcmp(">",argv[i]) == 0){
            // redirect output to file
            int fd = open(argv[i+1], O_RDWR | O_CREAT | O_TRUNC, 0644);
            argv[i] = argv[i+1] = NULL;   //discard ">" and file name
            dup2(fd, STDOUT_FILENO);
            break;
        }
        if(argv[i][0]=='>'){
            // get user pipe number.     
            int num = getNumForPipe(&argv[i][1]);
            User user = userList[num-1];

            // check user
            if(!user.exist){
                dup2(NULLOUT, STDOUT_FILENO);
                dprintf(STDERR_FILENO, "*** Error: user #%d does not exist yet. ***\n", num);
                argv[i] = NULL; //discard ">n"
                break;
            }

            // check user pipe
            FIFO fifo(me.index, user.index);
            if(fifo.exist()){
                dup2(NULLOUT, STDOUT_FILENO);
                dprintf(STDERR_FILENO, "*** Error: the pipe #%d->#%d already exists. ***\n", me.index+1, user.index+1);
                argv[i] = NULL; //discard ">n"
                break;
            }
            else{
                fifo.create();
            }

            // broadcast
            sprintf(msg, "*** %s (#%d) just piped '%s' to %s (#%d) ***", me.name, me.index+1, origin_cmd, user.name, user.index+1);
            broadcast();

            // send sig with writer & reader index to reader to read user pipe
            union sigval mysigval;
            mysigval.sival_int = me.index<<16 | user.index; // first 16bit for writer, last 16bit for reader
            if (sigqueue(user.pid, SIGRUP, mysigval) < 0){
                ERR("sigqueue");
            }

            // redirect output to user pipe
            int fd = fifo.writeFd();
            dup2(fd, STDOUT_FILENO);
            close(fd);

            argv[i] = NULL; //discard ">n"
            break;
        }
    }
}

static void SIGRUP_Handler(int sig, siginfo_t *s_t, void *p){
    int num = s_t->si_int; //int < from-16bit | to-16bit >
    int from = num>>16;
    int to = num%(1<<16);
    FIFO fifo(from,to);
    userPipeTable[from] = fifo.readFd();
}

void write_numberPipe(char** argv, User me){
    /*
        write number-pipe
    */
    for(int i=0; i<argc; i++){
        if(argv[i] == NULL) continue;
        if(argv[i][0] == '|' || argv[i][0] == '!'){

            int pipeErr = (argv[i][0]=='!');

            // get pipe number. if no number => |1
            int num = getNumForPipe(&argv[i][1]);
            argv[i] = NULL; //discard "|n"

            char** leftCmd = &argv[0];
            char** rightCmd = &argv[i+1];

            int index;
            if((index = pipeList.exist(me.cmdCnt+num)) >= 0){
                // if pipe is already existed, write to existed pipe.
                pipeList[index].write(leftCmd, pipeErr, MULTI_PROC);
            }
            else{
                // if pipe isn't existed, write to a new pipe.
                Pipe pipe;
                pipe.num = me.cmdCnt + num;
                pipe.from = me;
                pipe.to = me;
                pipe.create();
                pipe.write(leftCmd, pipeErr, MULTI_PROC);
                pipeList.add(pipe);
            }

            // recursively process right side of "|" cmd
            me.cmdCnt++;
            processCmd(rightCmd, me);
            return;
        }
    }
}

void SIGMSG_Handler(int signo) {
    //for sending msg (from SHM)
    printf("%s\n", msg);
}


void SIGINT_Handler(int signo){
    //for server closed
    for(int i=0;i<MAX_USER;i++){
        User user = userList[i];
        if(user.exist){
            close(userList[i].fd);
            kill(user.pid, SIGTERM);
        }
    }
    server.close();
    
    //clear user pipe
    for(int i=0;i<MAX_USER;i++){
        for(int j=0;j<MAX_USER;j++){
            FIFO fifo(i, j);
            if(fifo.exist()){
                fifo.remove();
            }
        }
    }   
    rmdir("user_pipe");

    printf("\rServer closed.\n");
    exit(0);
}
