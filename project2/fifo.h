#include "utils.h"

/* define for fifo */
#define PERMS 0666


class FIFO {
private:
    char path[30];

public:

	FIFO(int from, int to){
        sprintf(path, "user_pipe/%02d_%02d", from, to);
	}

    void create(){
        if(mkfifo(path, PERMS) < 0){
            ERR("mkfifo");
        }
    }

    int exist(){
        return (access(path, F_OK)==0);
    }

    void remove(){
        // unlink() will delete file until all fds closed
        unlink(path);
    }

    /* run either writeFd() and readFd() would block, need to run both of them */
    int writeFd(){
        return open(path, O_WRONLY);
    }

    int readFd(){
        return open(path, O_RDONLY);
    }

};
