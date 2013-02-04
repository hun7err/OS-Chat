#include "../include/signals.h"

void sighandler(int signum) {
	if(signum == SIGINT) {
		time_t t;
		char curtime[30];
		gettime(curtime, &t);
		printf("\n%s Przechwycono SIGINT: koncze dzialanie\n", curtime);
		kill(res.child, SIGINT);
		int msgid = msgget(res.msg_key, 0777);
		if(msgctl(msgid, IPC_RMID, NULL) == -1) {
			perror("Nie mozna usunac kolejki");
			printf("Error nr: %d", errno);
			exit(-1);
		}
		int hmsgid = msgget(res.heartbeat_msg_key, 0777);
		if(msgctl(hmsgid, IPC_RMID, NULL) == -1) {
			perror("Nie mozna usunac kolejki (heartbeat)");
			printf("Error nr: %d, key kolejki: %d, id: %d\n", errno, res.heartbeat_msg_key, hmsgid);
			//exit(-1);
		}
		//printf("po probie usuniecia kolejki\n");
		int semid = semget(res.sem_key, 3, 0777), shmid, i, count;
		if(semid != -1) {
			//printf("semid != -1\n");
			shmid = shmget(res.shm_key, sizeof(shm_type), 0777);
			if(shmid != -1) {
				//printf("shmid != -1\n");
				shm_type* shared = (shm_type*)shmat(shmid, NULL, 0);
				if(shared != (void*)(-1)) {
					//printf("mozna podpiac SHM\n");
					//int tab[3];
					//semctl(semid, 0, GETALL, tab);
					//printf("wartosci semaforow: %d (SERVER), %d (CLIENT), %d (LOG)\n", tab[SERVER], tab[CLIENT], tab[LOG]);
					p(semid, SERVER);
					//printf("semafor opuszczony\n");
					for(i = 0; i < MAX_SERVER_COUNT; i++) {
						if(shared->servers[i].queue_key != -1) count++;
						if(shared->servers[i].queue_key == res.msg_key) shared->servers[i].queue_key = -1;
					}
					printf("Przejrzalem serwery, pozostalo: %d\n", count);
					//printf("Ilosc serwerow: %d", count);
					v(semid, SERVER);
					shmdt(shared);
					if(count == 1) {
						shmctl(shmid, IPC_RMID, NULL);
						gettime(curtime, &t);
						char buf[512];
						sprintf(buf, "%s [%d] SIGINT: koncze dzialanie...\n", buf, res.msg_key);
						logfile(buf, &semid);
						semctl(semid, 0, IPC_RMID, NULL);
					} else exit(0);
				} else {
					perror("Nie mozna podlaczyc SHM");
					exit(-1);
				}
			} else {
				perror("Blad SHM");
				exit(-1);
			}
		} else {
			perror("Blad SEM");
			exit(-1);
		}
		exit(0);
	}
}
