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
		int semid = semget(res.sem_key, 3, 0777), shmid, i, count;
		if(semid != -1) {
			shmid = shmget(res.shm_key, sizeof(shm_type), 0777);
			if(shmid != -1) {
				shm_type* shared = (shm_type*)shmat(shmid, NULL, 0);
				if(shared != (void*)(-1)) {
					p(semid, SERVER);
					for(i = 0; i < MAX_SERVER_COUNT; i++) {
						if(shared->servers[i].queue_key == res.msg_key) shared->servers[i].queue_key = -1;
						else if(shared->servers[i].queue_key != -1) count++;
					}
					v(semid, SERVER);
					if(count == 1) {
						shmdt(shared);
						shmctl(shmid, IPC_RMID, NULL);
						gettime(curtime, &t);
						char buf[512];
						sprintf(buf, "%s [%d] SIGINT: koncze dzialanie...\n", buf, res.msg_key);
						logfile(buf, &semid);
						semctl(semid, 0, IPC_RMID, NULL);
					}
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
