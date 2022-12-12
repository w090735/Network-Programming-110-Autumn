#include "utils.h"

// replace code by clearly define member variable
class Env{
private:
    map<string, string> env;

    map<string, string> request_parser(string http_request){
        map<string, string> request_map;
        vector<string> header, temp;
        int pos;

        // implement in boost/algorithm/string.hpp
        // split http_request into vector in header
        // eCompress=token_compress_on: merge all delimeters

        split(header, http_request, is_any_of("\r\n"), token_compress_on);
        // header[0]: GET /{console_name}.cgi?... HTTP/1.1
        // header[1]: Host: host_name
        // get host
        request_map["HTTP_HOST"] = header[1].substr(6);
        
        // get method & uri & protocol & query
        split(temp, header[0], is_any_of(" "), token_compress_on);
        request_map["REQUEST_METHOD"]  = temp[0];
        request_map["REQUEST_URI"]     = temp[1];
        request_map["SERVER_PROTOCOL"] = temp[2];

        pos = temp[1].find("?");
        if(pos >= 0){
            request_map["QUERY_STRING"] = temp[1].substr(pos+1);
        }
        else{
            request_map["QUERY_STRING"] = "";
        }

        return request_map;
    }

public:
    Env(Endpoint server_info, Endpoint remote_info, string http_request){
        // server info
        env["SERVER_ADDR"] = server_info.address().to_string();
        env["SERVER_PORT"] = to_string(server_info.port());

        // remote info
        env["REMOTE_ADDR"] = remote_info.address().to_string();
        env["REMOTE_PORT"] = to_string(remote_info.port());

        // http request
        map<string, string> temp = request_parser(http_request);
        env["HTTP_HOST"]       = temp["HTTP_HOST"];
        env["REQUEST_METHOD"]  = temp["REQUEST_METHOD"];
        env["REQUEST_URI"]     = temp["REQUEST_URI"];
        env["SERVER_PROTOCOL"] = temp["SERVER_PROTOCOL"];
        env["QUERY_STRING"]    = temp["QUERY_STRING"];
    }

    string operator[](string key){
        return env[key];
    }
}
