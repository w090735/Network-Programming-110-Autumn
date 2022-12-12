#include "utils.h"

#define MAX_SESSION 5

// replace code by include "request.h"
class HTML {
    private:
        RequestList requestList;

    public:
        // initialize member variable request_list
        HTML(RequestList list): requestList(list) {}

        string output(){
            return generate_html();
        }

        // replace specific char to html representation
        static string escape(string content){
            replace_all(content, "&", "&amp;");
            replace_all(content, "\n", "&NewLine;");
            replace_all(content, "\r", "");
            replace_all(content, "\"", "&quot;");
            replace_all(content, "'", "&apos;");
            replace_all(content, ">", "&gt;");
            replace_all(content, "<", "&lt;");
            return content;
        }

        static string output_shell(int index, string content){ // output of shell script
            content = escape(content);
            return "<script>document.getElementById('s"+to_string(index)+"').innerHTML += '"+content+"';</script>\n";
        }

        static string output_command(int index, string content){ // display shell script with bold
            content = escape(content);
            return "<script>document.getElementById('s"+to_string(index)+"').innerHTML += '<b>"+content+"</b>';</script>\n";
        }

    private:
        string generate_html();
        string html_head();
        string html_body();
        string table();
        string table_head();
        string table_body();
};


string HTML::generate_html(){
    return "<!DOCTYPE html>\n<html lang=\"en\">\n" + html_head() + html_body() + "</html>\n";
} 

string HTML::html_head(){
    return 
        "<head>\n"
        "<meta charset=\"UTF-8\" />\n"
        "<title>NP Project 3 Console</title>\n"
        "<link\n"
        "rel=\"stylesheet\"\n"
        "href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\n"
        "integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\n"
        "crossorigin=\"anonymous\"\n"
        "/>\n"
        "<link\n"
        "href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
        "rel=\"stylesheet\"\n"
        "/>\n"
        "<link\n"
        "rel=\"icon\"\n"
        "type=\"image/png\"\n"
        "href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n"
        "/>\n"
        "<style>\n"
        "* {\n"
        "    font-family: 'Source Code Pro', monospace;\n"
        "    font-size: 1rem !important;\n"
        "}\n"
        "body {\n"
        "   background-color: #212529;\n"
        "}\n"
        "pre {\n"
        "    color: #cccccc;\n"
        "}\n"
        "b {\n"
        "    color: #ffffff;\n"
        "}\n"
        "</style>\n"
        "</head>\n";
}

string HTML::html_body(){
    return "<body>\n" + table() + "</body>\n";
}

string HTML::table(){
    return "<table class=\"table table-dark table-bordered\">\n" + table_head() + table_body() + "</table>\n";
}

string HTML::table_head(){ // displat host name and port
    string elements = "";
    for(int i=0; i<MAX_SESSION; i++){
        if(requestList.exist(i)){
            elements += <th scope=\"col\">" + requestList[i].host + ":" + requestList[i].port + "</th>\n";
        }
    }
    return "<thead>\n<tr>\n" + elements + "</tr>\n</thead>\n";
}

string HTML::table_body(){ // display session number
    string elements = "";
    for(int i=0; i<MAX_SESSION; i++){
        if(requestList.exist(i)){
            elements += "<td><pre id=\"s" + to_string(i) + "\" class=\"mb-0\"></pre></td>\n";
        }
    }
    return "<tbody>\n<tr>\n" + elements + "</tr>\n</tbody>\n";
}
