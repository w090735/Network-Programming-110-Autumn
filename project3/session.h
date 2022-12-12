#include "utils.h"
#include "env.h"

#define MAX_BUFFER 1024

class Session : public enable_shared_from_this<Session>{ // shared pointer point to same session object

private:
    /*
        socket: client socket
        request_buffer: request command
        response_status: http response
    */
    Socket socket;
    char request_buffer[MAX_BUFFER];
    string response_status = "HTTP/1.1 200 OK\r\n";

    // replace anonymous function
    void receive_handler(const ErrCode& ec, size_t length){
        if(!ec){
            cout << "===============REQUEST==============" << endl;
            cout << request_buffer << endl;
            cout.flush();
            write(response_status.length());
        }
        else{
            ERR("server read", ec);
            socket.close();
            exit(0);
        }
    }

    // separate code to additional function
    string get_program_name(Env env){
        int pos = env["REQUEST_URI"].find("?"); // parse request URI
        string program;

        if(pos >= 0){
            program = "." + env["REQUEST_URI"].substr(0, pos-1);
        }
        else{
            program = "." + env["REQUEST_URI"];
        }

        return program;
    }

    // replace anonymous function
    void send_handler(const ErrCode& ec, size_t length){
        if(!ec){
            Env env(socket.local_endpoint(), socket.remote_endpoint(), request_buffer); // create env object
            int sockfd;
            string path, program;

            for(auto obj : env){
                cout << obj.first << ": " << obj.second << endl;
                setenv(obj.first.c_str(), obj.second.c_str(), 1); // set environment variable with request
            }

            sockfd = socket.native_handle();
            dup2(sockfd, STDOUT_FILENO); // redirect stdout to client fd

            cout << "child" << endl;
            cout << "request uri: " << env["REQUEST_URI"] << endl;
            cout << "response string: " << response_status << endl;
            cout.flush();

            // replace by declare variable on top of function
            path = getenv("PATH");
            setenv("PATH", (path + ":.").c_str(), 1); // add current directory to path
            program = get_program_name(env);
            execlp(program.c_str(), program.c_str(), NULL);

            // exit after execlp done if success
            perror("execlp");
            exit(0); // force to exit child process if occur error
        }
        else{
            ERR("server write", ec);
            socket.close(); // close socket fd
            exit(0); // force to exit child process if occur error
        }
    }

    void read(){
        auto self(shared_from_this()); // make sure the session is alive after async receive is done
        socket.async_receive(buffer(request_buffer), receive_handler);
    }

    void write(size_t length) {
        auto self(shared_from_this()); // make sure the session is alive after async send is done
        socket.async_send(buffer(response_status, length), send_handler);
    }

public:
    Session(Socket _socket) : socket(move(_socket)) {} // create socket
    void start() { read(); }
};
