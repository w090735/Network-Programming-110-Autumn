#include "utils.h"
#include "npserver.h"

int main(int argc, char* argv[]){
    if(argc != 2){
        cerr << "Usage: socks_server <port>\n";
        return 1;
    }
    try{
        unsigned short port = std::atoi(argv[1]);
        NPServer npserver(port);

        npserver.run();
    }
    catch(exception& e){
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}