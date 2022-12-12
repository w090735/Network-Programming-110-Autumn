#include "utils.h"

class Request{
private:
    int session_index;
    string host;
    string port;
    string file_name;

public:
    Request(){
        host = "";
        port = "";
        file_name = "";
    }
};

class RequestList{
private:
    vector<Request> requestList;

public:
    void add(Request request){
        requestList.push_back(request);
    }

    void remove(int index){
        requestList.erase(requestList.begin() + index);
    }

    int size(){
        return requestList.size();
    }

    int exist(int index){
        return requestList[index].host.size() > 0;
    }

    string subQuery(stringstream& ss, int pos){
        string subq;

        ss >> subq;

        return subq.substr(pos);
    }

    void queryToReq(string query){
        if(QUERY_STRING.length() > 0){
            replace_all(QUERY_STRING, "&", " "); // replace "&" as " "
            stringstream ss(QUERY_STRING); // std::stringstream
            for(int i=0; i<MAX_SESSION; i++){
                Request request;

                request.session_index = i;
                request.host = subQuery(ss, 3);
                request.port = subQuery(ss, 3);
                request.file_name = subQuery(ss, 3);
                requestList.add(request);
            }
        }
    }

    Request& operator[](int index){
        return requestList[index];
    }
};
