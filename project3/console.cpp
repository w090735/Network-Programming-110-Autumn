#include "utils.h"
#include "html.h" // define request
#include "npsession.h"
#include "request.h"

int main(){

    string http_header = "Content-type: text/html\r\n\r\n";
    cout << http_header;

    // replace code by include "request.h"
    RequestList requestList;
    string QUERY_STRING = getenv("QUERY_STRING");

    requestList.queryToReq(QUERY_STRING);

    // show session host/port/session id
    // replace by include "request.h"
    HTML html(requestList);
    // HTML html(request_list);
    cout << html.output();
    cout.flush();

    NPClient npclient;
    for(int i=0; i<MAX_SESSION; i++){
        if(requestList.exist(i)){
            npclient.connect(requestList[i]);
        }
    }
    npclient.run();
}
