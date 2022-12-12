#include "utils.h"

#define MAX_BUF 15000

class SOCKS4_REQUEST{
public:
    /*
        0.0.0.x resolve by domain name
        +--+--+-------+-----+------+----+-----------+----+
        |VN|CD|DSTPORT|DSTIP|USERID|NULL|DOMAIN_NAME|NULL|
        +--+--+-------+-----+------+----+-----------+----+
         1  1    2       4            1               1
    */
    uint8_t VN;
    uint8_t CD;
    uint16_t DSTPORT;
    uint32_t DSTIP;
    string DOMAIN_NAME;
    string command;

    SOCKS4_REQUEST() {}

    void set(char buf[MAX_BUF]){
        VN = buf[0];
        CD = buf[1];
        DSTPORT = htons(*(uint16_t *) &buf[2]);
        DSTIP = htonl(*(uint32_t *) &buf[4]);

        if(DSTIP < 256){
            int pos = (size_t) strchr(buf+8, '\0') - (size_t) buf; // offset

            DOMAIN_NAME = string(buf + pos + 1);
        }
        else{
            DOMAIN_NAME = ip::address_v4(DSTIP).to_string();;
        }

        if(CD == 1) command = "CONNECT";
        else command = "BIND";
    }
};

class SOCKS4_REPLY{
public:
    /*
        +--+--+-------+-----+
        |VN|CD|DSTPORT|DSTIP|
        +--+--+-------+-----+
         1  1    2       4
    */
    uint64_t VN;
    uint64_t CD;
    uint64_t DSTPORT;
    uint64_t DSTIP;
    string command;

    void set(string _command, bool accept){
        command = _command;
        VN = (uint64_t) 0 << 56; // 0~7
        if(accept) CD = (uint64_t) 90 << 48; // 8~15
        CD = (uint64_t) 91 << 48; // 8~15
        if(strcmp("CONNECT", command) == 0) DSTPORT = (uint64_t) 0 << 32; // 16~31
        else DSTPORT_reply = reply_port << 32; // 16~31
        DSTIP = (uint64_t) 0; // 32~64
    }

    uint64_t getPacket(){
        uint64_t reply;

        reply = VN_reply | CD_reply | DSTPORT_reply | DSTIP_reply;
        reply = (((uint64_t) htonl(reply)) << 32) + htonl((reply) >> 32);

        return reply;
    }
};