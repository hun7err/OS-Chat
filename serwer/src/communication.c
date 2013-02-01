#include "../include/communication.h"

void gettime(char *buf, time_t *t) {
	*t = time(NULL);
	strftime(buf, 30, "%Y-%m-%dT%H:%M:%S%z", localtime(t));
}

int receive(int key, void *dest, int size, long type) {
	int mid = msgget(key, 0777);
	int ret = msgrcv(mid, dest, size, type, IPC_NOWAIT);
	return ret;
}

void get_key(int *key, char type) {
	int id = -1;
	do {
		switch(type) {
			case SHM:
				id = shmget(*key, sizeof(shm_type), IPC_CREAT | IPC_EXCL | 0777);
			break;
			case SEM:
				id = semget(*key, 3, IPC_CREAT | IPC_EXCL | 0777);
			break;
			case MSG:
				id = msgget(*key, IPC_CREAT | IPC_EXCL | 0777);
			break;
		}
		if(id == -1) (*key)++;
		else break;
	} while(id == -1);
	switch(type) {
		case SHM:
			res.shm_key = *key;
		break;
		case SEM:
			res.sem_key = *key;
		break;
		case MSG:
			res.msg_key = *key;
		break;
	}
}
