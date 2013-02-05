// global includes
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
// local includes
#include "../include/chat.h"
#include "../include/sem.h"
#include "../include/defines.h"
#include "../include/log.h"
#include "../include/communication.h"
#include "../include/signals.h"

int main(int argc, char ** argv) {
	char buff[30];
	time_t t;
	int semid, shmid, msgid, msg_key = START_KEY, sem_key = START_KEY, shm_key = START_KEY, heartbeat_msg_key = START_KEY;

	short sem[3] = {1, 1, 1};
	// 0 - lista serwerow, 1 - lista klientow, 2 - plik logu (zdefiniowane w chat.h)
	
	shm_type *shared;
	if(argc > 1) {
		shm_key = atoi(argv[1]);
		gettime(buff, &t);
		printf("%s Key SHM: %d\n", buff, shm_key);
		res.shm_key = shm_key;
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
		//res.sem_key = sem_key;
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
			strcpy(shared->clients[i].room, "");
			strcpy(shared->clients[i].name, "");
		}
		v(semid, CLIENT);
	} else sem_key = shared->key_semaphores;

	res.sem_key = sem_key;
	res.shm_key = shm_key;

	if(shmdt(shared) == -1) {
		perror("Blad odlaczania SHM");
		return -1;
	}	
	
	if((shared = (shm_type*)shmat(shmid,NULL,0)) == (void*)(-1)) {
		perror("Blad podlaczania segmentu");
		return -1;
	}
	
	//signal(SIGINT, sighandler);

	gettime(buff, &t);
	printf("%s Online. SHM key: %d, SEM key: %d\n", buff, shm_key, sem_key);
	printf("%s Tworzenie kolejki komunikatow\n", buff);
	
	get_key(&msg_key, MSG);
	get_key(&heartbeat_msg_key, MSG);
	msgid = msgget(msg_key, 0777);
	//printf("heartbeat key: %d\n", heartbeat_msg_key);
	res.heartbeat_msg_key = heartbeat_msg_key;
	gettime(buff, &t);
	printf("%s Mam kolejke, klucz: %d. Let the show begin!\n", buff, msg_key);
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
	signal(SIGINT, SIG_DFL);
	if(!(res.child = fork())) { // heartbeat
		signal(SIGINT, SIG_DFL);
		struct timespec tim;
		tim.tv_sec = 2;
		tim.tv_nsec = 0;
		
		while(1) {
			int mid = msgget(res.heartbeat_msg_key, 0777), i = 0;
			if(mid == -1) {
				perror("HEARTBEAT: nie mozna pobrac id kolejki");
				exit(-1);
			}
			compact_message cmg;
			int shmid = shmget(res.shm_key, sizeof(shm_type), 0777),
				vals[MAX_USER_COUNT_PER_SERVER],
				ret = 1, j = 0;
			if(shmid != -1) {
				//printf("shmid != -1\n");
				int semid = semget(res.sem_key, 3, 0777);
				if(semid != -1) {
					shm_type* shared = (shm_type*)shmat(shmid, NULL, 0);
					if(shared != (void*)(-1)) {
						for(i = 0; i < MAX_USER_COUNT_PER_SERVER; i++) {
							vals[i] = 0;
						}
						i = 0;
						//printf("przed p(semid, CLIENT)\n");
						p(semid, CLIENT);
						//printf("po opuszczeniu semafora CLIENT\n");
						for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
							if(shared->clients[i].queue_key != -1 && shared->clients[i].server_queue_key == msg_key) {
								printf("Heartbeat do %d\n", shared->clients[i].queue_key);
								cmg.type = MSG_HEARTBEAT;
								cmg.content.value = msg_key;
								vals[j] = shared->clients[i].queue_key;
								j++;
								int rcpt = msgget(shared->clients[i].queue_key, 0777);
								msgsnd(rcpt, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), IPC_NOWAIT);
							}
						}
						v(semid, CLIENT);
					}
					shmdt(shared);
				}
			}
			// dodac wewnetrzna kolejke SysV, do ktorej proces glowny wrzuci odpowiedzi na heartbeaty ktore bedzie mozna tu odczytac
			nanosleep(&tim, NULL);
			char curtime[25];
			time_t t;
			gettime(curtime, &t);
			printf("%s HEARTBEAT: sprawdzam\n", curtime);
			mid = msgget(res.heartbeat_msg_key, 0777);
			if(mid != -1) {
				compact_message cmg;
				
				//printf("odczytuje z %d\n", res.heartbeat_msg_key);
				do {
					ret = msgrcv(mid, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), MSG_HEARTBEAT, IPC_NOWAIT);
					//printf("ret: %d, cmg.content.value: %d\n", ret, cmg.content.value);
					if(ret != -1) {
						for(i = 0; i < MAX_USER_COUNT_PER_SERVER; i++) {
							if(vals[i] == cmg.content.value) vals[i] = 0;
						}
					}
					i++;
				} while(ret != -1);
				shmid = shmget(res.shm_key, sizeof(shm_type), 0777);
				if(shmid == -1) {
					perror("Blad shmget");
					exit(-1);
				}
				semid = semget(res.sem_key, 3, 0777);
				if(semid == -1) {
					perror("Blad semget");
					exit(-1);
				}
				shm_type* shared = (shm_type*)shmat(shmid, NULL, 0);
				if(shared == (void*)(-1)) {
					perror("Nie mozna podlaczyc SHM");
					exit(-1);
				}
				p(semid, CLIENT);
				for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
					for(j = 0; j < MAX_USER_COUNT_PER_SERVER; j++) {
						if(shared->clients[i].queue_key == vals[j] && vals[j] > 0) {
							gettime(curtime, &t);
							printf("%s Usuwam %d\n", curtime, vals[j]);
							strcpy(shared->clients[i].room, "");
							strcpy(shared->clients[i].name, "");
							shared->clients[i].queue_key = -1;
							shared->clients[i].server_queue_key = -1;
						}
					}
				}
				v(semid, CLIENT);

				//printf("odebrano:\n");
				//for(i = 0; i < 10; i++) printf("%d ", vals[i]);
				//printf("\n");
				
				//for(
				//p(semid, CLIENT);
				/*for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
					int found = 0;
					for(j = 0; j < MAX_USER_COUNT_PER_SERVER; j++) {
						if(shared->clients[i].queue_key == vals[j] && shared->clients[i].queue_key != -1) {
							found = 1;
							break;
						}
					}
					if(!found) {
						
					}
				}*/
				//v(semid, CLIENT);
				//shmdt(shared);
				p(semid, SERVER);
				//printf("Wysylam HEARTBEAT_SERVER\n");
				for(i = 0; i < MAX_SERVER_COUNT; i++) {
					if(shared->servers[i].queue_key != -1 && shared->servers[i].queue_key != msg_key) {
						int rcpt = msgget(shared->servers[i].queue_key, 0777);
						cmg.type = MSG_HEARTBEAT_SERVER;
						cmg.content.value = msg_key;
						msgsnd(rcpt, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), IPC_NOWAIT);
						//printf("wysylam do %d\n", shared->servers[i].queue_key);
					}
				}
				v(semid, SERVER);
				shmdt(shared);
				nanosleep(&tim, NULL);
				mid = msgget(res.heartbeat_msg_key, 0777);
				if(mid != -1) {
				compact_message cmg;
				int vals[MAX_SERVER_COUNT];
				int ret = 1;
				for(i = 0; i < MAX_SERVER_COUNT; i++) {
					vals[i] = 0;
				}
				i = 0;
				//printf("odczytuje z %d\n", res.heartbeat_msg_key);
				do {
					ret = msgrcv(mid, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), MSG_HEARTBEAT, IPC_NOWAIT);
					//printf("ret: %d, cmg.content.value: %d\n", ret, cmg.content.value);
					if(ret != -1) vals[i] = cmg.content.value;
					i++;
				} while(ret != -1);
				//printf("odebrano:\n");
				//for(i = 0; i < 10; i++) printf("%d ", vals[i]);
				//printf("\n");
				shmid = shmget(res.shm_key, sizeof(shm_type), 0777);
				if(shmid == -1) {
					perror("Blad shmget");
					exit(-1);
				}
				semid = semget(res.sem_key, 3, 0777);
				if(semid == -1) {
					perror("Blad semget");
					exit(-1);
				}
				shm_type* shared = (shm_type*)shmat(shmid, NULL, 0);
				if(shared == (void*)(-1)) {
					perror("Nie mozna podlaczyc SHM");
					exit(-1);
				}
				p(semid, SERVER);
				for(i = 0; i < MAX_SERVER_COUNT; i++) {
					int j = 0, found = 0;
					for(j = 0; j < MAX_USER_COUNT_PER_SERVER; j++) {
						if(shared->servers[i].queue_key == vals[j]) {
							found = 1;
							break;
						}
					}
					/*if(!found && shared->servers[i].queue_key != msg_key) {
						//strcpy(shared->clients[i].room, "");
						//strcpy(shared->clients[i].name, "");
						shared->servers[i].queue_key = -1;
						//shared->clients[i].server_queue_key = -1;
					}*/
				}
				v(semid, SERVER);
				shmdt(shared);
				}
			}
		}
	} else {	// obsluga wiadomosci -> odbieranie/wysylanie
		signal(SIGINT, sighandler);
		gettime(buff, &t);
		sprintf(buf, "%s [%d] Wchodze w petle odbierania/wysylania\n", buff, msg_key);
		if(logfile(buf,&sem_key) == -1) {
			printf("Blad zapisu do pliku logu\n");
			return -1;
		}
		compact_message cmg;
		standard_message smg;
		server_message svmg;

		struct timespec tim;
		tim.tv_sec = 0;
		tim.tv_nsec = 1000;

		while(1) {
			int semid = -1, shmid = -1, mid = -1, i = 0, firstfree = -1, usercount = 0, val = 0;
			
			if(receive(msg_key, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), MSG_REGISTER) != -1) {
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
						if(shared->clients[i].queue_key == cmg.content.value || strcmp(shared->clients[i].name, cmg.content.sender) == 0) {
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
				//printf("numer kolejki: %d\n", cmg.content.value);
				cmg.content.value = val;
				int ret = msgsnd(mid, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), IPC_NOWAIT);
				if(ret == -1) {
					perror("Blad msgsnd");
					printf("Numer bledu: %d\n", errno);
				}
				// todo: jesli zwroci msgget -1, to wywal usera z listy
				// tutaj dodaj klienta do listy w SHM jesli mozna
				// jesli nie, zmien status na odpowiedni ERROR i odeslij

			}
			if(receive(msg_key, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), MSG_HEARTBEAT_SERVER) != -1) {
				char curtime[25];
				time_t t;
				gettime(curtime, &t);
				printf("%s MSG_HEARTBEAT_SERVER od %d\n", curtime, cmg.content.value);
				int rcpt = msgget(cmg.content.value, 0777), hb = msgget(res.heartbeat_msg_key, 0777);
				msgsnd(hb, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), IPC_NOWAIT);
				cmg.content.value = msg_key;
				//msgsnd(rcpt, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), IPC_NOWAIT);
				//semid = semget(sem_key, 3, 0777);
				//shmid = shmget(shm_key, sizeof(shm_type), 0777);
			}
			if(receive(msg_key, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), MSG_UNREGISTER) != -1) {
				char curtime[25];
				time_t t;
				gettime(curtime, &t);
				printf("%s MSG_UNREGISTER od %d\n", curtime, cmg.content.value);
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
			}
			if(receive(msg_key, &smg, /*member_size(standard_message, content)*/sizeof(standard_message), MSG_JOIN) != -1) {
				// zmien klientowi kanal i odeslij strukture ze status = 0
				char curtime[25];
				time_t t;
				gettime(curtime, &t);
				printf("%s MSG_JOIN od %s do %s\n", curtime, smg.content.sender, smg.content.message);
				semid = semget(sem_key, 3, 0777);
				shmid = shmget(shm_key, sizeof(shm_type), 0777);
				p(semid, CLIENT);
				shared = (shm_type*)shmat(shmid, NULL, 0);
				if(shared == (void*)(-1)) {
					v(semid, CLIENT);
					perror("shmat MSG_JOIN");
					return -1;
				}
				int rcpt;
				for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
					if(strcmp(shared->clients[i].name, smg.content.sender) == 0) {
						strcpy(shared->clients[i].room, smg.content.message);
						rcpt = shared->clients[i].queue_key;
						break;
					}
				}
				cmg.content.value = 0;
				cmg.type = MSG_JOIN;
				//printf("Odbiorca to %s, odsylam pod %d\n", smg.content.sender, rcpt);
				mid = msgget(rcpt, 0777);
				msgsnd(mid, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), IPC_NOWAIT);
				shmdt(shared);
				v(semid, CLIENT);
			}
			if(receive(msg_key, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), MSG_LEAVE) != -1) {
				char curtime[25];
				time_t t;
				gettime(curtime, &t);
				printf("%s MSG_LEAVE od %d\n", curtime, cmg.content.value);
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
				msgsnd(mid, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), IPC_NOWAIT);
				shmdt(shared);
				v(semid, CLIENT);
				// przenies klienta z pokoju do globala
			}
			if(receive(msg_key, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), MSG_HEARTBEAT) != -1) {
				char curtime[25];
				time_t t;
				gettime(curtime, &t);
				printf("%s MSG_HEARTBEAT od %d\n", curtime, cmg.content.value);
				// obsluga heartbeat - odpowiedzi (dac tutaj wewnetrzna kolejke SysV i do niej sobie wysylac te odpowiedzi, pozniej odbierac je w watku wysylajacym)
				int internal = msgget(res.heartbeat_msg_key, 0777);
				int ret = msgsnd(internal, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), IPC_NOWAIT);
				printf("msgsnd do wewnetrznej: %d, odebrane od %d\n", ret, cmg.content.value);
			}
			if(receive(msg_key, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), MSG_LIST) != -1) {
				// odeslij w user_list liste userow
				char curtime[25];
				time_t t;
				gettime(curtime, &t);
				printf("%s MSG_LIST od %d\n", curtime, cmg.content.value);
				user_list usr;
				usr.type = MSG_LIST;
				int shmid = shmget(res.shm_key, sizeof(shm_type), 0777);
				if(shmid != -1) {
					shm_type* shared = (shm_type*)shmat(shmid, NULL, 0);
					if(shared == (void*)(-1)) {
						perror("Nie mozna podlaczyc SHM");
						return -1;
					}
					int semid = semget(res.sem_key, 3, 0777), i, cur = 0;
					if(semid == -1) {
						perror("Nie mozna pobrac id semaforow");
						return -1;
					}
					p(semid, CLIENT);
					char room[512];
					for(i = 0; i < MAX_SERVER_COUNT * MAX_USER_COUNT_PER_SERVER; i++) {
						if(shared->clients[i].queue_key == cmg.content.value || strcmp(shared->clients[i].name, cmg.content.sender) == 0) {
							strcpy(room, shared->clients[i].room);
							break;
						}
					}
					for(i = 0; i < MAX_SERVER_COUNT * MAX_USER_COUNT_PER_SERVER; i++) {
						if(cur > MAX_USER_LIST_LENGTH) break;
						if(shared->clients[i].queue_key != -1 && strcmp(shared->clients[i].room, room) == 0) {
							strcpy(usr.content.list[cur], shared->clients[i].name);
							cur++;
						}
					}
					v(semid, CLIENT);
					for(i = cur; i < MAX_USER_LIST_LENGTH; i++) strcpy(usr.content.list[cur], "");	// good guy hun7er clears your empty user fields.
					shmdt(shared);
					int rcpt = msgget(cmg.content.value, 0777);
					msgsnd(rcpt, &usr, /*member_size(user_list, content)*/sizeof(user_list), IPC_NOWAIT);
				}
			}
			if(receive(msg_key, &smg, /*member_size(standard_message, content)*/sizeof(standard_message), MSG_ROOM) != -1) {
				char curtime[25];
				time_t t;
				gettime(curtime, &t);
				printf("%s MSG_ROOM od %d\n", curtime, cmg.content.value);
				server_message svm;
				svm.type = MSG_SERVER;
				svm.content.msg = smg;
				/* tu wyslij do pozostalych svm */
				int shmid = shmget(res.shm_key, sizeof(shm_type), 0777);
				if(shmid != -1) {
					shm_type *shared = (shm_type*)shmat(shmid, NULL, 0);
					if(shared == (void*)(-1)) {
						perror("Nie mozna podlaczyc SHM");
						return -1;
					}
					int semid = semget(res.sem_key, 3, 0777), i;
					if(semid == -1) {
						perror("Nie mozna pobrac id semaforow");
						return -1;
					}
					p(semid, CLIENT);
					for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
						if(shared->clients[i].server_queue_key == msg_key && strcmp(shared->clients[i].name, smg.content.sender) != 0 && strcmp(shared->clients[i].room, smg.content.recipient) == 0) {
							int mid = msgget(shared->clients[i].queue_key, 0777);
							msgsnd(mid, &smg, /*member_size(standard_message, content)*/sizeof(standard_message), IPC_NOWAIT);
							//printf("wysylam do %d\n", shared->clients[i].queue_key);
						}
					}
					v(semid, CLIENT);
					p(semid, SERVER);
					for(i = 0; i < MAX_SERVER_COUNT; i++) {
						if(shared->servers[i].queue_key != -1 && shared->servers[i].queue_key != msg_key) {
							int mid = msgget(shared->servers[i].queue_key, 0777);
							msgsnd(mid, &svm, /*member_size(server_message, content)*/sizeof(server_message), IPC_NOWAIT);
						}
					}
					v(semid, SERVER);
					shmdt(shared);
				}
				// spakuj do MSG_SERVER i wyslij do wszystkich serwerow poza soba, jesli na serwerze istnieje pokoj do ktorego szla wiadomosc to wtedy do userow tego pokoju wyslij wiadomosc
			}
			if(receive(msg_key, &svmg, /*member_size(server_message, content)*/sizeof(server_message), MSG_SERVER) != -1) {
				char curtime[25];
				time_t t;
				gettime(curtime, &t);
				printf("%s MSG_SERVER od %d\n", curtime, cmg.content.value);
				smg = svmg.content.msg;
				int shmid = shmget(res.shm_key, sizeof(shm_type), 0777);
				// pobierz sobie semafory i shm
				if(shmid == -1) {
					perror("Nie mozna pobrac SHM");
					return -1;
				}
				shm_type* shared = (shm_type*)shmat(shmid, NULL, 0);
				if(shared == (void*)(-1)) {
					perror("Nie mozna podpiac SHM");
					return -1;
				}
				int semid = semget(res.sem_key, 3, 0777), i;
				if(semid == -1) {
					perror("Nie mozna pobrac id semaforow");
					return -1;
				}
				p(semid, CLIENT);
				switch(smg.type) {
					case MSG_ROOM:
						//printf("Wiadomosc do kanalu: %s\n", smg.content.recipient);
						// podepnij shm, opusc semafory i sprawdz serwery
						for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
							if(strcmp(shared->clients[i].room, smg.content.recipient) == 0 && shared->clients[i].server_queue_key == msg_key) {
								int mid = msgget(shared->clients[i].queue_key, 0777);
								msgsnd(mid, &smg, /*member_size(standard_message, content)*/sizeof(standard_message), IPC_NOWAIT);
								//printf("przeslano do %d\n", shared->clients[i].queue_key);
							}
						}
					break;
					case MSG_PRIVATE:
						for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
							if(strcmp(shared->clients[i].name, smg.content.recipient) == 0 && shared->clients[i].server_queue_key == msg_key) {
								int mid = msgget(shared->clients[i].queue_key, 0777);
								msgsnd(mid, &smg, /*member_size(standard_message, content)*/sizeof(standard_message), IPC_NOWAIT);
							}
						}
						// podobnie ale klientow
					break;
					default:
					break;
				}
				v(semid, CLIENT);
				shmdt(shared);
				// odpakuj wiadomosc, sprawdz czy masz komu/gdzie wyslac
			}
			if(receive(msg_key, &smg, /*member_size(standard_message, content)*/sizeof(standard_message), MSG_PRIVATE) != -1) {
				char curtime[25];
				time_t t;
				gettime(curtime, &t);
				printf("%s MSG_PRIVATE od %d\n", curtime, cmg.content.value);
				//server_message svm;
				//svm.type = MSG_SERVER;
				//svm.content.msg = smg;
				/* tu wyslij do pozostalych svm */
				int shmid = shmget(res.shm_key, sizeof(shm_type), 0777);
				if(shmid != -1) {
					shm_type *shared = (shm_type*)shmat(shmid, NULL, 0);
					if(shared == (void*)(-1)) {
						perror("Nie mozna podlaczyc SHM");
						return -1;
					}
					int semid = semget(res.sem_key, 3, 0777), i;
					if(semid == -1) {
						perror("Nie mozna pobrac id semaforow");
						return -1;
					}
					p(semid, CLIENT);
					for(i = 0; i < MAX_SERVER_COUNT*MAX_USER_COUNT_PER_SERVER; i++) {
						if(/*shared->clients[i].server_queue_key == */msg_key && strcmp(shared->clients[i].name, smg.content.recipient) == 0) {
							int mid = msgget(shared->clients[i].queue_key, 0777);
							msgsnd(mid, &smg, /*member_size(standard_message, content)*/sizeof(standard_message), IPC_NOWAIT);
							//printf("wysylam do %d\n", shared->clients[i].queue_key);
							break;
						}
					}
					v(semid, CLIENT);
					/*p(semid, SERVER);
					for(i = 0; i < MAX_SERVER_COUNT; i++) {
						if(shared->servers[i].queue_key != -1 && shared->servers[i].queue_key != msg_key) {
							int mid = msgget(shared->servers[i].queue_key, 0777);
							msgsnd(mid, &svm, member_size(server_message, content), IPC_NOWAIT);
						}
					}
					v(semid, SERVER);*/
					shmdt(shared);
				}

				// sprawdz czy user jest na serwerze, jesli tak to wyslij, potem spakuj do MSG_SERVER i podaj do wszystkich serwerow poza soba samym
			}
			nanosleep(&tim, NULL);
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

/*
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
}*/
