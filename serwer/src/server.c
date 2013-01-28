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
#include <errno.h>

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

#define E_NICKEX -1
#define E_FULL -2

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
	} while(id == -1);
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
		shm_key = atoi(argv[1]);
		gettime(buff, &t);
		printf("%s Key kolejki: %d\n", buff, shm_key);
		shmid = shmget(shm_key, sizeof(shm_type), 0777);
	} else if(argc == 1) {
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

	semid = semget(sem_key, 3, 0777);
	p(semid, SERVER);
	shared = (shm_type*)shmat(shmid,NULL,0);
	if(shared == (void*)(-1)) {
		perror("Nie mozna podlaczyc segmentu pamieci wspoldzielonej");
		return -1;
	}
	int i = 0;
	for(; i < MAX_SERVER_COUNT; i++) {
		if(shared->servers[i].queue_key == -1) {
			shared->servers[i].queue_key = msg_key;
			break;
		}
	}
	shmdt(shared);
	v(semid, SERVER);

	char buf[512];
	sprintf(buf, "%s [%d] Serwer wystartowal, let the show begin!\n", buff, msg_key);
	if(logfile(buf,&sem_key) == -1) {
		printf("Blad zapisu do pliku logu\n");
		return -1;
	}
	if(!fork()) { // heartbeat
		while(1); // dodac wewnetrzna kolejke SysV, do ktorej proces glowny wrzuci odpowiedzi na heartbeaty ktore bedzie mozna tu odczytac
	} else {	// obsluga wiadomosci -> odbieranie/wysylanie
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
			int len = msgrcv(mid, &cmg, member_size(compact_message, content), -MSG_LEAVE, IPC_NOWAIT), ret;
			if(len != -1) {
				//printf("Odebralem compact_message\n");
				//printf("Zawartosc\nid: %d\nsender: %s\nvalue: %d\n", cmg.content.id, cmg.content.sender, cmg.content.value);
				int semid = -1, shmid = -1, mid = -1, i = 0, firstfree = -1, usercount = 0, val = 0;
				switch(cmg.type) {
					case MSG_REGISTER:
						gettime(buff, &t);
						sprintf(buf, "%s [%d] zarejestrowalem uzytkownika %s (kolejka: %d)\n", buff, msg_key, cmg.content.sender, cmg.content.value);
						logfile(buf, &sem_key);
						printf("%s Uzytkownik %s zarejestrowal sie z kluczem kolejki %d\n", buff, cmg.content.sender, cmg.content.value);
						semid = semget(sem_key, 3, 0777);
						shmid = shmget(shm_key, sizeof(shm_type), 0777);
						p(semid, CLIENT);
						shm_type* shared = (shm_type*)shmat(shmid, NULL, 0);
						if(shared == (void*)(-1)) {
							v(semid, CLIENT);
							perror("Nie mozna podlaczyc segmentu pamieci wspoldzielonej; wychodze...");
							return -1;
						}
						i = 0;
						for(; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
							if(shared->clients[i].queue_key == -1 && firstfree == -1) {
								firstfree = i;
							} else if(shared->clients[i].queue_key != -1) {
								if(shared->clients[i].server_queue_key == msg_key) usercount++;
								if(usercount >= MAX_USER_COUNT_PER_SERVER) {
									val = E_FULL;
									break;
								}
								if(shared->clients[i].queue_key == cmg.content.value) {
									val = E_NICKEX;
									break;
								}
							}
						}
						if(val == 0) {
							shared->clients[firstfree].queue_key = cmg.content.value;
							shared->clients[firstfree].server_queue_key = msg_key;
							strcpy(shared->clients[firstfree].name, cmg.content.sender);
							strcpy(shared->clients[firstfree].room, GLOBAL_ROOM_NAME);
						}
						shmdt(shared);
						v(semid, CLIENT);
						mid = msgget(cmg.content.value, 0777);
						printf("numer kolejki: %d\n", cmg.content.value);
						cmg.content.value = val;
						ret = msgsnd(mid, &cmg, member_size(compact_message, content), IPC_NOWAIT);
						if(ret == -1) {
							perror("Blad msgsnd");
							printf("Numer bledu: %d\n", errno);
						}
						// todo: jesli zwroci msgget -1, to wywal usera z listy
						// tutaj dodaj klienta do listy w SHM jesli mozna
						// jesli nie, zmien status na odpowiedni ERROR i odeslij
					break;
					case MSG_UNREGISTER:
						semid = semget(sem_key, 3, 0777);
						shmid = shmget(shm_key, sizeof(shm_type), 0777);
						p(semid, CLIENT);
						shared = (shm_type*)shmat(shmid, NULL, 0);
						if(shared == (void*)(-1)) {
							v(semid, CLIENT);
							perror("shmat MSG_UNREGISTER");
							return -1;
						}
						for(i = 0; i < MAX_SERVER_COUNT * MAX_USER_COUNT_PER_SERVER; i++) {
							if(shared->clients[i].queue_key == cmg.content.value) {
								shared->clients[i].queue_key = -1;
								shared->clients[i].server_queue_key = -1;
								strcpy(shared->clients[i].name, "");
								strcpy(shared->clients[i].room, "");
								break;
							}
						}
						shmdt(shared);
						v(semid, CLIENT);
						// tu wywal klienta z listy
					break;
					case MSG_JOIN:
						// zmien klientowi kanal i odeslij strukture ze status = 0
						semid = semget(sem_key, 3, 0777);
						shmid = shmget(shm_key, sizeof(shm_type), 0777);
						p(semid, CLIENT);
						shared = (shm_type*)shmat(shmid, NULL, 0);
						if(shared == (void*)(-1)) {
							v(semid, CLIENT);
							perror("shmat MSG_JOIN");
							return -1;
						}
						for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
							if(shared->clients[i].queue_key == cmg.content.value) {
								strcpy(shared->clients[i].room, cmg.content.sender);
								break;
							}
						}
						mid = msgget(cmg.content.value, 0777);
						cmg.content.value = 0;
						msgsnd(mid, &cmg, member_size(compact_message, content), IPC_NOWAIT);
						shmdt(shared);
						v(semid, CLIENT);
					break;
					case MSG_LEAVE:
						semid = semget(sem_key, 3, 0777);
						shmid = shmget(shm_key, sizeof(shm_type), 0777);
						p(semid, CLIENT);
						shared = (shm_type*)shmat(shmid, NULL, 0);
						if(shared == (void*)(-1)) {
							v(semid, CLIENT);
							perror("shmat MSG_LEAVE");
							return -1;
						}
						for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
							if(shared->clients[i].queue_key == cmg.content.value) {
								strcpy(shared->clients[i].room, GLOBAL_ROOM_NAME);
								break;
							}
						}
						mid = msgget(cmg.content.value, 0777);
						cmg.content.value = 0;
						msgsnd(mid, &cmg, member_size(compact_message, content), IPC_NOWAIT);
						shmdt(shared);
						v(semid, CLIENT);
						// przenies klienta z pokoju do globala
					break;
					//case MSG_LIST:
					//break;
					case MSG_HEARTBEAT:
						// odebrac odpowiedz i wrzucic do wewnetrznej kolejki na odpowiedzi heartbeatowe; fork() je sobie odczyta i zdecyduje czy ok czy nie
					break;
					default: // zaden z obslugiwanych typow ;_;
						gettime(buff, &t);
						sprintf(buf, "%s [%d] nie spodziewalem sie hiszpanskiej inkwizycji! typ: %lu\n", buff, msg_key, cmg.type);
						printf("%s Nie spodziewalem sie hiszpanskiej inkwizycji! compact_message, typ %lu\n", buff, cmg.type); // zakladam ze czytajacy ten kod zna skecz Monthy Pythona "Hiszpanska Inwizycja" ;-)
					break;
				}
			}
			mid = msgget(msg_key, 0777);
			standard_message smg; // nie, nie submachine gun :p
			int mlen = msgrcv(mid, &smg, member_size(compact_message, content), -MSG_SERVER, IPC_NOWAIT);
			if(mlen != -1) {
				switch(cmg.type) {
					case MSG_LIST:
						// zwroc liste userow
					break;
					case MSG_ROOM:
						printf("MSG_ROOM\n");
						// spakuj do MSG_SERVER i wyslij do wszystkich serwerow poza soba, jesli na serwerze istnieje pokoj do ktorego szla wiadomosc to wtedy do userow tego pokoju wyslij wiadomosc
					break;
					case MSG_SERVER:
						// odpakuj wiadomosc, sprawdz czy masz komu/gdzie wyslac
					break;
					case MSG_PRIVATE:
						// sprawdz czy user jest na serwerze, jesli tak to wyslij, potem spakuj do MSG_SERVER i podaj do wszystkich serwerow poza soba samym
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
