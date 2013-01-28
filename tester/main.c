#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdio.h>
#include "../klient/include/chat.h"
#include "sem.h"

int main(int argc, char ** argv) {
	if (argc < 2) {
		printf("Sposob uzycia: %s [klucz shm]\n", argv[0]);
		return -1;
	}
	int shm_key = atoi(argv[1]);
	//int semid = semget(sem_key, 3, 0777);
	int shmid = shmget(shm_key, sizeof(shm_type), 0777);
	shm_type* shared = (shm_type*)shmat(shmid, NULL, 0);
	if(shared == (void*)(-1)) {
		perror("Blad shmat");
		return -1;
	}
	int sem_key = shared->key_semaphores;
	int semid = semget(sem_key, 3, 0777);
	p(semid, CLIENT);
	int i = 0;
	printf("aktualnie podlaczeni klienci:\n\n");
	printf("nazwa_usera [klucz_kolejki,serwer] kanal\n");
	for(; i < 30; i++) printf("-");
	printf("\n");
	for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER;i++) {
		if(shared->clients[i].queue_key != -1) {
			printf("%s [%d,%d] %s\n", shared->clients[i].name, shared->clients[i].queue_key, shared->clients[i].server_queue_key, shared->clients[i].room);
		}
	}
	v(semid, CLIENT);
	printf("\nserwery:\n\n");
	p(semid, SERVER);
	for(i = 0; i < MAX_SERVER_COUNT; i++) {
		if(shared->servers[i].queue_key != -1) {
			printf("%d\n", shared->servers[i].queue_key);
		}
	}
	v(semid, SERVER);
	shmdt(shared);
	return 0;
}
