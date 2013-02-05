#include "sem.h"
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>

int p(int semid, int semnum) {
	struct sembuf op;
	op.sem_num = semnum;
	op.sem_op = -1;
	op.sem_flg = 0;

	if(semop(semid,&op,1) == -1) {
		perror("Blad opuszczania semafora");
		return -1;
	} else return 0;
}

int v(int semid, int semnum) {
	struct sembuf op;
	op.sem_num = semnum;
	op.sem_op = 1;
	op.sem_flg = 0;

	if(semop(semid,&op,1) == -1) {
		perror("Blad podnoszenia semafora");
		return -1;
	} else return 0;
}
