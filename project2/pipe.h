#include "utils.h"
#include "shell.h"
#include "server.h"
#include "user.h"

extern Server server;

class Pipe{
public:
	User from; // only used for single proc
	User to; // only used for single proc
	int num; // if num<0 -> user-pipe, num!=0 -> number-pipe
	int pfd[2];

	void create(){
		if(pipe(pfd)<0){
			ERR("pipe");
		} 
	}

	void read(){
		// redirect input from pipe 
		close(pfd[1]);
		dup2(pfd[0], STDIN_FILENO);
		close(pfd[0]);
	}

	void write(char** argv, int pipeErr, bool multiProc){

		int pid;
		while((pid = fork())<0){
			usleep(1000);
		}

		if(pid == 0){
			// child: exe cmd and redirect stdout(stderr) to pipe
			if(multiProc) server.close(); // single_proc don't close
			close(pfd[0]);
			dup2(pfd[1], STDOUT_FILENO);
			close(pfd[1]);
			if(pipeErr){
				dup2(STDOUT_FILENO, STDERR_FILENO);
			}
			exeCmd(argv);
		}
		else{
			// parent: no need to wait
		}

	}

};

class PipeList{
private:
	vector<Pipe> pipeList;

public:
	void add(Pipe pipe){
		pipeList.push_back(pipe);
	}

	void remove(int index){
		pipeList.erase(pipeList.begin()+index);
	}

	int size(){
		return pipeList.size();
	}
	
	/* 
		check the existence by cmdCnt, user(from/to)
		return: index (if not exist, return -1)
	*/
	int exist(int cmdCnt){
		for(int j=0;j<pipeList.size();j++){
			Pipe pipe = pipeList[j];
			if(pipe.num == cmdCnt){
				return j;
			}
		}
		return -1;
	}

	int exist(User to, int cmdCnt){
		for(int j=0;j<pipeList.size();j++){
			Pipe pipe = pipeList[j];
			if(pipe.to==to && pipe.num == cmdCnt){
				return j;
			}
		}
		return -1;
	}

	int exist(User from, User to){
		for(int j=0;j<pipeList.size();j++){
			Pipe pipe = pipeList[j];
			if(pipe.from==from && pipe.to==to){
				return j;
			}
		}
		return -1;
	}

	int exist(User from, User to, int cmdCnt){
		for(int j=0;j<pipeList.size();j++){
			Pipe pipe = pipeList[j];
			if(pipe.from==from && pipe.to==to && pipe.num == cmdCnt){
				return j;
			}
		}
		return -1;
	}


	Pipe& operator[](int index){
		return pipeList[index];
	}

};
