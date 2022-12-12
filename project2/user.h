#include "utils.h"

/* define for User */
#define SIZE_NAME 64
#define MAX_USER 30

class User {
public:
	// user info
	char name[SIZE_NAME] = "(no name)";
	char ip[16] = "";
	int port = -1;
	// list info
	int index = -1;
	int exist = false;
	// shell info
	map<string, string> env; // single proc server used only
	int cmdCnt = 0;
	int pid = -1; // distinct in multi proc server
	int fd = -1; // distinct in single proc server

	bool operator==(const User& user2){
		// ip, port should be unique
		return (strcmp(ip,user2.ip)==0) && (port==user2.port);
	}
};

class UserList{
private:
	/* use array so that it can be saved to shared memory */
	User userList[MAX_USER];

public:

	/*
	 	add user to smallest index
		return: index of the added user
	*/
	int add(User user){
		int i = 0;
		while(userList[i].exist) i++;
		user.index = i;
		user.exist = true;
		userList[i] = user;
		return i;
	}

	/* clear the user info at index */
	void remove(int index){
		User user;
		userList[index] = user;
		printf("remove user %d, exist: %d\n", index, userList[index].exist);
	}

	User& operator[](int index){
		return userList[index];
	}
};
