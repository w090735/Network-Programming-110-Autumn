#include "utils.h"
#include "sockssession.h"

class NPServer{
private:
    /*
        ioService: core I/O function
        acceptor: server create socket connection and accept client
        socket: client socket
        signalSet: signals registered in ioService
    */
    IOService ioService;
    Acceptor acceptor;
    Socket socket;
    SignalSet signalSet;

    // replace anonymous function
    void wait_handler(const ErrCode& ec, int signo){
        if(!ec){
            if(acceptor.is_open()){
                int status = 0;
                while(waitpid(-1, &status, WNOHANG) > 0);
                wait();
            }
        }
        else{
            cerr << "wait error code: " << ec << endl;
            wait();
        }
    }

    // replace anonymous function
    void accept_handler(const ErrCode& ec){
        if(!ec){
            ioService.notify_fork(IOService::fork_prepare);
            int pid = fork();
            if(pid < 0){
                cerr << "fork" << endl;
            }
            else if(pid == 0){
                ioService.notify_fork(IOService::fork_child);
                acceptor.close();
                signalSet.cancel();
                shared_ptr<SOCKSSession> sp = make_shared<SOCKSSession>(move(socket), ioService);
                sp->start();
            }
            else{
                ioService.notify_fork(IOService::fork_parent);
                socket.close();
                accept();
            }
        }
        else{
            cerr << "accept error code: " << ec << endl;
            accept();
        }
    }

    void wait(){
        signalSet.async_wait(wait_handler);
    }

    void accept(){
        acceptor.async_accept(socket, accept_handler);
    }

public:
    NPServer(short port) :
        acceptor(ioService, Endpoint(tcp::v4(), port)),
        socket(ioService),
        signalSet(ioService, SIGCHLD){
        wait();
        accept();
    }

    void run(){
        ioService.run();
    }
};
