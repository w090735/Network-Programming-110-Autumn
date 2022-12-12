export PATH := bin:./:$(PATH)

CXXFLAGS=-std=c++11 -Wall -pedantic -pthread -lboost_system
CGIFLAGS=-std=c++14 -lws2_32 -lwsock32 
CXX_INCLUDE_DIRS=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))

all: http_server.cpp console.cpp
	g++ http_server.cpp -o http_server $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)
	g++ console.cpp -o console.cgi $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

server: http_server.cpp
	g++ http_server.cpp -o ./http_server $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

client: console.cpp
	g++ console.cpp -o ./console $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

cgi:
	g++ main.cpp -o cgi_server $(CGIFLAGS)

clean:
	rm ./http_server
	rm ./console
	rm *.cgi

clean cgi:
	rm -f *.cgi
