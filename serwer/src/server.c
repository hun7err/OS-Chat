#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
//#include <sys/types.h>
#include <sys/sem.h>
//#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../include/chat.h"
#include "../include/sem.h"
#include <signal.h>
#include <string.h>

#define START_KEY 666

#define member_size(type,member) sizeof(((type *)0)->member)

void gettime(char *buf, time_t *t);

int msgid = 0;

void sighandler(int signum) {
	if(signum == SIGINT) {
		printf("Przechwycono SIGINT: wychodze...\n");
		// 1. usun swoja kolejke komunikatow
		if(msgctl(msgid, IPC_RMID, NULL) == -1) {
			perror("Nie mozna usunac kolejki komunikatow");
			exit(-1);
		}
		// sprawdz czy sa inne serwery
		// jak nie to usun SHM
		// i usun SEM
	}
}

int logfile(const char *message, int *semid);

#define SHM 0
#define SEM 1
#define MSG 2

void get_key(int *key, char type) {
	int id = -1;
	while(1) {
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
	}
}

int main(int argc, char ** argv) {
	char buff[30];
	time_t t;
	int semid, shmid, msgid, msg_key = START_KEY, sem_key = START_KEY, shm_key = START_KEY;

	short sem[3] = {1, 1, 1};
	// 0 - lista serwerow, 1 - lista klientow, 2 - plik logu

	//semid = semget(sem_key, 3, IPC_CREAT | IPC_EXCL | 0777);
	
	shm_type *shared;
	if(argc > 1) {
		int shm_key = atoi(argv[1]);
		shmid = shmget(shm_key, sizeof(shm_type), 0777);
	} else {
		get_key(&sem_key, SEM);
		get_key(&shm_key, SHM);
	}
	shmid = shmget(shm_key, sizeof(shm_type), 0777|IPC_CREAT);
	// up: utworz pamiec wspoldzielona lub podepnij istniejaca
	shared = (shm_type*)shmat(shmid, NULL, argc > 1 ? SHM_RDONLY : 0);
	// up: jesli podano klucz do repo to ustaw w RDONLY; jak nie to nie ustawiaj nic
	if(shared == (void*)-1) {
		perror("Blad podlaczania SHM");
		return -1;
	}

	if(argc == 1) {
		shared->key_semaphores = sem_key;
		semid = semget(sem_key, 3, IPC_CREAT|0777);
		if(semctl(semid, 0, SETALL, sem) == -1) {
			perror("Nie mozna ustawic semaforow");
			return -1;
		}
		int i = 0;
		p(semid, SERVER);
		for(; i < MAX_SERVER_COUNT; i++) {
			shared->servers[i].queue_key = -1;
		}
		v(semid, SERVER);
		p(semid, CLIENT);
		for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
			shared->clients[i].queue_key = -1;
		}
		v(semid, CLIENT);
	} else sem_key = shared->key_semaphores;

	if(shmdt(shared) == -1) {
		perror("Blad odlaczania SHM");
		return -1;
	}	
	
	if((shared = (shm_type*)shmat(shmid,NULL,0)) == (void*)(-1)) {
		perror("Blad podlaczania segmentu");
		return -1;
	}
	
	signal(SIGINT, sighandler);

	gettime(buff, &t);
	printf("%s Online. SHM key: %d, SEM key: %d\n", buff, shm_key, sem_key);
	printf("%s Tworzenie kolejki komunikatow\n", buff);
	
	get_key(&msg_key, MSG);
	msgid = msgget(msg_key, 0777);
	gettime(buff, &t);
	printf("%s Mam kolejke, klucz: %d. Time for rock'n'roll!\n", buff, msg_key);
	//p(semid, 2);

	char buf[512];
	sprintf(buf, "%s [%d] Serwer wystartowal, let the show begin!\n", buff, msg_key);
	if(logfile(buf,&sem_key) == -1) {
		printf("Blad zapisu do pliku logu\n");
		return -1;
	}
	if(!fork()) {
		while(1);
		// heartbeat
	} else {
		gettime(buff, &t);
		sprintf(buf, "%s [%d] Wchodze w petle odbierania/wysylania\n", buff, msg_key);
		if(logfile(buf,&sem_key) == -1) {
			printf("Blad zapisu do pliku logu\n");
			return -1;
		}
		compact_message cmg;
		while(1) {
			int mid = msgget(msg_key, 0777);
			if(mid == -1) {
				get_key(&msg_key, MSG);
				mid = msgget(msg_key, 0777);
			}
			int len = msgrcv(mid, &cmg, member_size(compact_message, content), -MSG_LEAVE, IPC_NOWAIT);
			if(len != -1) {
				//printf("Odebralem compact_message\n");
				//printf("Zawartosc\nid: %d\nsender: %s\nvalue: %d\n", cmg.content.id, cmg.content.sender, cmg.content.value);
				switch(cmg.type) {
					case MSG_REGISTER:
						gettime(buff, &t);
						sprintf(buf, "%s [%d] zarejestrowalem uzytkownika %s (kolejka: %d)", buff, msg_key, cmg.content.sender, cmg.content.value);
						logfile(buf, &sem_key);
						printf("%s Uzytkownik %s zarejestrowal sie z kluczem kolejki %d\n", buff, cmg.content.sender, cmg.content.value);
						//printf("Dostalem MSG_REGISTER!\n");
					break;
					case MSG_UNREGISTER:
					break;
					case MSG_JOIN:
					break;
					case MSG_LEAVE:
					break;
					case MSG_LIST:
					break;
					case MSG_HEARTBEAT:
					break;
					default:
					break;
				}
			}
		}
		// glowna czesc
	}

	//v(semid, 2);
	gettime(buff, &t);

	// dopisz sie do listy serwerow

	// p na semaforze od SHM
	// dodaj id semaforow
	// v na semaforze od SHM
	//while(1);

	// najpierw tutaj tworz semafory, jak nie uda sie to wyjdz - nie blokuj gdy nie masz semaforow
	gettime(buff, &t);
	sprintf(buf, "%s [%d] Serwer kopnal w kalendarz :(\n", buff, msgid);
	logfile(buf,&sem_key);
	printf("%s Serwer konczy dzialanie\n", buff);
	
	return 0;
}

void gettime(char *buf, time_t *t) {
	//char buff[30];
	*t = time(NULL);
	strftime(buf, 30, "%Y-%m-%dT%H:%M:%S%z", localtime(t));
}

int logfile(const char *message, int *sem_key) {
	int semid = 0;
	semid = semget(*sem_key, 3, 0777);
	if(semid == -1) {
		perror("Blad pobierania id semaforow");
		return -1;
	}
	p(semid, LOG);
	int fd = open(LOGFILE, O_RDWR | O_CREAT, 0777);
	if(fd == -1) return -1;
	if(lseek(fd,0,SEEK_END) == -1) return -1;
	if(write(fd, message, strlen(message)) == -1) return -1;
	close(fd);
	v(semid, LOG);
	return 0;
}
