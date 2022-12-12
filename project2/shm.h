#include "utils.h"

/* define for shared memory */
#define PERMS 0666


class SharedMemory {

private:	
	int shmid;
	void* shmp;

public:

	SharedMemory(int key, int size){
		// open(create) shared memory for writing/reading
		if((shmid = shmget(key, size, IPC_CREAT | PERMS)) < 0){
			ERR("shmget");
		}
		if((shmp = shmat(shmid, NULL, 0)) == (void*)-1){
			ERR("shmat");
		}
		bzero(shmp, size);
	}

	void* attach(){
		// attach data from shared memory
		return shmp;
	}

	~SharedMemory(){		
		// close shared memory
		shmdt(shmp);
		// delete shared memory
		shmctl(shmid, IPC_RMID, NULL);
	}

};
