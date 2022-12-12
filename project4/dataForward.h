#include "utils.h"

#define MAX_BUF 15000

class DataForward{
private:
    /*
        socket_to_dst: socket to FTP server(source)
        socket_to_src: socket to FTP client(destination)
        data_to_dst: data to FTP server(source)
        data_to_src: data to FTP client(destination)
        direction:
            0: write to src
            1: write to dst
            2: read from src
            3: read from dst
    */
    Socket socket_to_dst;
    Socket socket_to_src;
    char data_to_dst[MAX_BUF];
    char data_to_src[MAX_BUF];
    int direction;

    // replace anonymous function
    void receive_handler(const ErrCode& ec, size_t length){
        if(!ec){
            if(direction == 0) write_to_src(length);
            else write_to_dst(length);
        }
        else{
            if(ec != error::eof){
                if(direction == 0) cerr << "read_from_dst error code: " << ec << endl;
                else cerr << "read_from_src error code: " << ec << endl;
            }
            close();
        }
    }

    // replace anonymous function
    void send_handler(const ErrCode& ec, size_t length){
        if(!ec){
            if(direction == 2) read_from_src();
            else read_from_dst();
        }
        else{
            if(ec != error::eof){
                if(direction == 2) cerr << "write_to_dst error code: " << ec << endl;
                else cerr << "write_to_src error code: " << ec << endl;
            }
            close();
        }
    }

    void read_from_dst(){
        direction = 0; // prepare write to src
        bzero(data_to_src, MAX_BUF);
        socket_to_src.async_receive(buffer(data_to_src, MAX_BUF), receive_handler);
    }

    void read_from_src(){
        direction = 1; // prepare write to dst
        bzero(data_to_dst, MAX_BUF);
        socket_to_dst.async_receive(buffer(data_to_dst, MAX_BUF), receive_handler);
    }

    void write_to_dst(size_t length){
        direction = 2; // prepare read from src
        socket_to_src.async_send(buffer(data_to_dst, length), send_handler);
    }

    void write_to_src(size_t length){
        direction = 3; // prepare read from dst
        socket_to_dst.async_send(buffer(data_to_src, length), send_handler);
    }

    void close(){
        socket_to_dst.close();
        socket_to_src.close();
        exit(0);
    }

public:
    DataForward(Socket src_socket, Socket dst_socket):
        socket_to_dst(move(src_socket)), socket_to_src(move(dst_socket)) {}

    void start(){
        read_from_src();
        read_from_dst();
    }
};