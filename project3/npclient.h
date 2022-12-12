#include "utils.h"
#include "html.h" // define request
#include "npsession.h"
#include "request.h"

class NPClient {
private:
    /*
        ioService: core I/O function
        request: request info
    */
    IOService ioService;
    Request request;

public:
    NPClient(): ioService() {}

    // replace anonymous function
    void connect_handler(const ErrCode& ec){
        if(!ec){
            // replace code by split command
            shared_ptr<Socket> socket = make_shared<Socket>(ioService);
            shared_ptr<NPSession> sp = make_shared<NPSession>(socket, request.session_index, request.file_name);
            sp->start();
        }
        else{
            ERR("connect", ec);
        }
    }

    void connect(Request _request){
        Resolver resolver(ioService);
        auto endpoint_iterator = resolver.resolve(Resolver::query(request.host, request.port)); // return socket vector

        request = _request;
        socket->async_connect(endpoint_iterator->endpoint(), connect_handler);
    }

    void run(){
        ioService.run();
    }
};
