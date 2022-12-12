#include "utils.h"
#include "html.h"

#define MAX_BUFFER 15001

// enable_shared_from_this: able to use shared_from_this()
// shared_from_this(): share object pointer between multiple instances
class NPSession : public enable_shared_from_this<NPSession>{
private:
    /*
        socket: client socket
        buf: read buffer
        index: session id
        file_name: test file name
        file: file stream
    */
    shared_ptr<Socket> socket;
    char buf[MAX_BUFFER];
    int index;
    string file_name;
    fstream file;

    // replace anonymous function
    void receive_handler(const ErrCode& ec, size_t length){
        if(!ec){
            cout.flush();
            cout << HTML::output_shell(index, buf);
            cout.flush();
            // replace code by safe string compare
            if(strcmp("%", buf) == 0){
                write();
            }
            else{
                read();
            }
        }
        else{
            ERR("console read", ec);
            close();
        }
    }

    // replace anonymous function
    void send_handler(const ErrCode& ec, size_t length){
        if(!ec){
            cout.flush();
            cout << HTML::output_command(index, buf);
            cout.flush();
            read();
        }
        else{
            ERR("console write", ec);
            close();
        }
    }

    void read(){
        auto self(shared_from_this()); // make sure the npsession is alive as long as the async receive is alive
        
        bzero(buf, MAX_BUFFER);
        socket->async_receive(buffer(buf, MAX_BUFFER-1), receive_handler);
    }
    
    void write(){
        auto self(shared_from_this()); // make sure the npsession is alive after the async send is done
        
        bzero(buf, MAX_BUFFER);
        if(file.getline(buf, MAX_BUFFER)){
            strcat(buf, "\n");
            socket->async_send(buffer(buf, strlen(buf)), send_handler);
        }
    }

    void close(){
        socket->close();
        file.close();
    }

public:
    NPSession(shared_ptr<Socket> _socket, int i, string _file_name) : socket(_socket), index(i), file_name(_file_name) {}
    
    void start(){ 
        file.open("test_case/" + file_name, ios::in);
        read();
    }
};

#endif