#include "utils.h"
#include "firewall.h"
#include "dataForward.h"
#include "packet.h"

#define MAX_BUF 15000

class SOCKSSession: public std::enable_shared_from_this<SOCKSSession>{
private:
    /*
        src_socket: socket to FTP server(source)
        dst_socket: socket to FTP client(destination)
        buf: read buffer
    */
    IOService ioService;
    Socket src_socket;
    Socket dst_socket;
    auto endpoint_iterator;
    char buf[MAX_BUF];

    void setFTPClient(string domain_name, string port){
        Resolver resolver(ioService);
        endpoint_iterator = resolver.resolve(Resolver::query(domain_name), port);
    }

    // replace anonymous function
    void receive_handler(const ErrCode& ec, size_t length){
        cout << "len: " << length << endl;
        cout << "ec: " << ec << endl;
        if(!ec){
            // replace code by include "packet.h"
            SOCKS4_REQUEST socks4_request;
            string DSTIP_str, S_IP, S_PORT, Reply;

            socks4_request.set(buf);
            setFTPClient(socks4_request.DOMAIN_NAME, to_string(socks4_request.DSTPORT));

            DSTIP_str = endpoint_iterator->endpoint().address().to_string();
            S_IP = src_socket.remote_endpoint().address().to_string();
            S_PORT = to_string(src_socket.remote_endpoint().port());
            Reply;

            Firewall firewall;
            bool accept = firewall.isPermit(DSTIP_str, socks4_request.command);
            if(accept) Reply = "Accept";
            else Reply = "Reject";

            cout << "VN: " << unsigned(socks4_request.VN) << ",CD: " << unsigned(socks4_request.CD) <<
                    ",DSTPORT: " << to_string(socks4_request.DSTPORT) <<
                    ",DSTIP: " << DSTIP_str <<
                    ",DOMAIN_NAME: " << socks4_request.DOMAIN_NAME << endl;
            
            cout << "<S_IP>: " << S_IP << endl;
            cout << "<S_PORT>: " << S_PORT << endl;
            cout << "<D_IP>: " << DSTIP_str << endl;
            cout << "<D_PORT>: " << to_string(socks4_request.DSTPORT) << endl;
            cout << "<Command>: " << socks4_request.command << endl;
            cout << "<Reply>: " << Reply << endl;

            write(socks4_request);
        }
        else{
            if(ec != error::eof){
                cerr << "read error code: " << ec << endl;
            }
            close();
        }
    }

    void read(){
        auto self(shared_from_this());
        src_socket.async_receive(buffer(buf), receive_handler);
    }

    void write(SOCKS4_REQUEST socks4_request){
        SOCKS4_REPLY socks4_reply;
        uint64_t packet;

        socks4_reply.set(socks4_request.command, accept);
        packet = getPacket();

        if(accept){
            if(strcmp("CONNECT", socks4_request.command) == 0){
                // connect
                dst_socket.connect(endpoint_iterator->endpoint());
                    
                // reply
                src_socket.send(buffer((char*) &packet), sizeof(packet));

                DataForward dataForward(move(src_socket), move(dst_socket));
                dataForward.start();

                run();
            }
            else if(strcmp("BIND", socks4_request.command) == 0){
                Acceptor acceptor(ioService, Endpoint(tcp::v4(), 0));

                // reply 1
                src_socket.send(buffer((char*)&packet, sizeof(packet)));
                    
                // accept
                acceptor.accept(dst_socket);

                // reply 2
                src_socket.send(buffer((char*)&packet, sizeof(packet)));

                // data
                DataForward dataForward(move(src_socket), move(dst_socket));
                dataForward.start();

                run();
            }
            else{
                cerr << "command" << endl;
                close();
            }
        }
        else{
            // reply
            src_socket.send(buffer((char*) &packet), sizeof(packet));
            close();
        }
    }

    void close(){
        src_socket.close();
        dst_socket.close();
        exit(0);
    }
    
public:
    SOCKSSession(Socket socket, IOService _ioService):
        ioService(_ioService), src_socket(std::move(socket)), dst_socket(_ioService){}

    void start(){ read(); }

    void run(){
        ioService.run();
    }
};
