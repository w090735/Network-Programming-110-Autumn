#include "utils.h"

class Firewall{
private:
    /*
        config: config file name
        file: file stream
        connect_ips: connect ip list
        bind_ips: bind ip list
    */
    const string config = "socks.conf";
    fstream file;
    vector<string> connect_ips, bind_ips;

public:
    void setIP(string content){
        /*
            content: <op> <ip>
            <op>:
                'b': bind
                'c': connect
            mode:
                -1: <op>
                 0: bind ip
                 1: connect ip
        */
        string content;
        int mode = -1;

        while(!file.eof()){
            file >> content;
            if(mode != -1){
                if(mode == 0){
                    bind_ips.push_back(content);
                    mode = -1;
                }
                else if(mode == 1){
                    connect_ips.push_back(content);
                    mode = -1;
                }
            }
            else{
                if(content.compare("b") == 0){
                    mode = 0;
                }
                else if(content.compare("c") == 0){
                    mode = 1;
                }
            }
            cout << content << endl; 
        }

        for(int i=0; i<connect_ips.size(); i++){
            cout << connect_ips[i] << " ";
        }
        cout << endl;

        for(int i=0; i<bind_ips.size(); i++){
            cout << bind_ips[i] << " ";
        }
        cout << endl;
    }

    Firewall(){
        file.open(config, ios::in);
        if(file.fail()){
            cerr << "cannot open " << config << endl;
        }
        else{
            setIP();
        }
    }

    ~Firewall(){
        file.close();
    }

    bool isPermit(string ip, string mode){
        if(mode.compare("BIND") == 0){
            for(int i=0; i<bind_ips.size(); i++){
                if(bind_ips[i][0] == '*'){
                    return true;
                }
                const char* str = ip.c_str();
                regex base_regex(bind_ips[i]);
                cmatch base_match;
                if(regex_match(str, base_match, base_regex)){
                    return true;
                }
            }
        }
        else if(mode.compare("CONNECT") == 0){
            for(int i=0; i<connect_ips.size(); i++){
                if(connect_ips[i][0] == '*'){
                    return true;
                }
                const char* str = ip.c_str();
                regex base_regex(connect_ips[i]);
                cmatch base_match;
                if(regex_match(str, base_match, base_regex)){
                    return true;
                }
            }
        }
        return false;
    }
};
