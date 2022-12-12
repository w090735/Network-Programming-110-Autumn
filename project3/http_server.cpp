#include "utils.h"
#include "server.h"

int main(int argc, char* const argv[]){
    if(argc != 2){
        cerr << "Usage:" << argv[0] << " [port]" << endl;
        return 1;
    }

    try{
        unsigned short port = atoi(argv[1]);
        Server server(port);
        server.run();
    }catch (exception& e){
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}