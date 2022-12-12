#include "utils.h"
#include "session.h"

class Server{
private:
    /*
        ioService: core I/O function
        signalSet: signals registered in ioService
        acceptor: server create socket connection and accept client
        socket: client socket
    */
    IOService ioService;
    SignalSet signalSet;
    Acceptor acceptor;
    Socket socket;

    // replace anonymous function
    void wait_handler(const ErrCode& ec, int signo){
        if(!ec){
            if(acceptor.is_open()){ // check if server is connected
                int status = 0;
                while (waitpid(-1, &status, WNOHANG) > 0); // wait for child process
                wait(); // force to clean child process
            }
        }
    }

    // replace anonymous function
    void accept_handler(const ErrCode& ec){
        if(!ec){
            ioService.notify_fork(io_service::fork_prepare);
            if(fork() == 0){
                ioService.notify_fork(io_service::fork_child);
                acceptor.close(); // close socket fd
                signalSet.cancel(); // force to wait signal
                // split command
                shared_ptr<Session> sp = make_shared<Session>(move(socket));
                sp->start();
                // make_shared<Session>(move(socket))->start(); // create session class and start
            }
            else{
                ioService.notify_fork(io_service::fork_parent);
                socket.close(); // close socket fd
                accept(); // recursive call accept()
            }
        }
        else{
            ERR("accept", ec); // print error
            accept(); // recursive call accept()
        }
    }

    void wait(){
        signalSet.async_wait(wait_handler);
    }

    void accept(){
        acceptor.async_accept(socket, accept_handler);
    }

public:
    /*
        acceptor(&ex, &endpoint, reuse_addr=true){
            basic_socket_acceptor<Protocol> acceptor(my_context);
            acceptor.open(endpoint.protocol());
            if(reuse_addr) acceptor.set_option(socket_base::reuse_address(true));
            acceptor.bind(endpoint);
            acceptor.listen();
        }
        
        socket(&io_context): create socket, need open, connect/accept
    */
    Server(short port) : 
        ioService(), 
        signalSet(ioService, SIGCHLD), 
        acceptor(ioService, Endpoint(TCP::v4(), port)), // Endpoint(): sockaddr_in 
        socket(ioService){
        wait();
        accept();
    }

    void run(){
        ioService.run();
    }
};
