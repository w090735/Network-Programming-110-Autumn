#include "utils.h"
#include "user.h"

/* define for Server */
#define MAX_CONNECT 30

class Server{
	
private:
	
	int sockfd;
	struct sockaddr_in server_info;
	
public:
	
	/* 
		initialize server
	    port: server's port number 
	*/
	void init(int port){
		
		// create socket
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd < 0) ERR("socket err");
		
		// set server info
		bzero(&server_info,sizeof(server_info));
		server_info.sin_family = AF_INET;
		server_info.sin_addr.s_addr = INADDR_ANY;
		server_info.sin_port = htons(port);
		
		int optval = 1;
		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0){
			ERR("setsockopt SO_REUSEADDR err");
		}
		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0){
			ERR("setsockopt SO_REUSEPORT err");
		}

		// bind
		if(bind(sockfd,(struct sockaddr*)&server_info,sizeof(server_info)) < 0){
			ERR("bind err");
		}
		
		// listen to connection (up to MAX_CONNECT)
		if(listen(sockfd, MAX_CONNECT) < 0){
			ERR("listen err");
		}
		
		printf("Waiting for connection...\n");
	}
	

	/*
		blocking for waiting connection
		when connection established, return client fd
		return: user info
	*/
	User getConn(){
		struct sockaddr_in client_info;
		socklen_t addrlen = sizeof(client_info);
		int clientfd = accept(sockfd, (sockaddr*)&client_info, &addrlen);
		if(clientfd < 0){
			ERR("accept err");
		}
		else{
			User user;
			strcpy(user.ip, inet_ntoa(client_info.sin_addr));
			user.port = ntohs(client_info.sin_port);
			user.fd = clientfd;
			return user;
		}
	}
	

	int getfd(){
		return sockfd;
	}


	/* 
		close server fd 
	*/
	void close(){
		::close(sockfd);
	}
	

	/* 
		destructor: close fd 
	*/
	~Server(){
		close();
	}
	
};
