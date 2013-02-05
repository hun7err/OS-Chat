// global
#include <ncurses.h>
#include <locale.h>
//#include <string.h>
//#include <stdlib.h>
#include <unistd.h>
#include <time.h>
// local
//#include "..include/window.h"
//#include "..include/defines.h"
#include "../include/text_processing.h"
#include "../include/communication.h"
#include "../include/sem.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/poll.h>
#include <mqueue.h>
#include <errno.h>
#include <signal.h>

void sighandler(int num) {
	if (num == SIGINT) {
		quit(&res);
	}
}

void get_key(int *key) {
	int qid = -1;
	/*for(qid = -1; qid == -1; (*key)++) {
		qid = msgget(*key, IPC_CREAT|IPC_EXCL|0777);
	}*/
	do {
		qid = msgget(*key, IPC_CREAT | IPC_EXCL | 0777);
		if(qid == -1) (*key)++;
		else break;
	} while (qid == -1);
}

int receive(int key, void *dest, int size, long type) {
	int msgid = msgget(key, 0777);
	int ret = msgrcv(msgid, dest, size, type, IPC_NOWAIT);
	return ret;
}

int main(int argc, char ** argv) {
	setlocale(LC_ALL, "");
	setlocale(LC_CTYPE, "C-UTF-8");	
	initscr();
	cbreak();

	//keypad(stdscr, TRUE);

	refresh();

	gui_t gui;
	core_t core;

	gui.mainwindow = create_window(LINES, COLS, 0, 0);
	gui.content = create_window(LINES-3, COLS-16-2, 1, 1);
	gui.user_list = create_window(LINES-3, 16, 1, COLS-16-1);

	strncpy(core.room, "(status)", 9);
	core.room[8] = '\0';
	core.cur_line = 0;
	core.cur_user_pos = 0;
	init_colors();
	draw_gui(gui.mainwindow, &core);

	core.cursor_x = 0;
	core.cursor_y = LINES-1;
	//core.serverkey = 0;
	char /*cur_time[9], userlist_buffer[1024],*/ buffer[1024];
	time_t now;
	//strftime(cur_time, 9, "%H:%M:%S", localtime(&now));
	//cur_time[8] = '\0';
	//char buf[512];
	add_info(&core, gui.content, "OS-Chat wystartowal", &now);
	add_info(&core, gui.content, "wpisz /help aby uzyskac pomoc", &now);
	//add_msg(&core, gui.content, "hun7er", "czesc", &now, 1);
	//add_msg(&core, gui.content, "hun7er", "]:>", &now, 1);
	//add_msg(&core, gui.content, "bot", "yo", &now, 0);

	//add_user_to_list(&core, "hun7er");
	//add_user_to_list(&core, "testowy");
	//add_content_line(&core, gui.content, 0, "-!- uzytkownik mescam dolaczyl do #global");
	
	scrollok(gui.content, 1);
	
	refresh();
	int int_queue_in, int_queue_out, ext_queue; // internal queue, external queue
	
	struct mq_attr attr;
	attr.mq_msgsize = MAX_MSG_SIZE;
	attr.mq_maxmsg = 10;
	//char buf[128];
	int e1 = 1, e2 = 1;
	
	do {
		sprintf(res.q_in, "/internal_in%d", e1);
		res.int_in_fd = mq_open(res.q_in, O_RDWR|O_CREAT|O_EXCL, 0777, &attr);
		e1++;
	} while(res.int_in_fd == -1);
	do {
		sprintf(res.q_out, "/internal_out%d", e2);
		res.int_out_fd = mq_open(res.q_out, O_RDWR|O_CREAT|O_EXCL, 0777, &attr);
		e2++;
	} while(res.int_out_fd == -1);

	int_queue_in = res.int_in_fd;
	int_queue_out = res.int_out_fd;

	if(int_queue_in == -1 || int_queue_out == -1) {
		endwin();
		printf("max msg size: %lu\n", MAX_MSG_SIZE);
		perror("Nie mozna utworzyc wewnetrznych kolejek komunikatow");
		printf("Numer bledu: %d\n", errno);
		return -1;
	}
	//sprintf(buf, "Kolejki: %s (in), %s (out)", res.q_in, res.q_out);
	//add_info(&core, gui.content, buf, &now);
	now = time(NULL);
	//add_private(&core, gui.content, "hun7er", "psssst. Kolego, masz na ziarno?", &now);

	int ret = pipe(server_key);
	if(ret == -1) {
		add_content_line(&core, gui.content, 0, "-!- Blad: nie mozna utworzyc potoku");
	}
	ret = pipe(room);
	if(ret == -1) {
		add_info(&core, gui.content, "BLAD: nie mozna utworzyc potoku", &now);
	}
	fcntl(server_key[0], F_SETFL, O_NDELAY);
	fcntl(server_key[1], F_SETFL, O_NDELAY);
	fcntl(room[0], F_SETFL, O_NDELAY);
	fcntl(room[1], F_SETFL, O_NDELAY);

	// internal: obrobione dane do wyswietlenia
	// external: "surowe" dane do obrobienia przez forki

	ext_queue = DEFAULT_QUEUE_KEY;
	get_key(&ext_queue);
	core.mykey = res.queue_key = ext_queue;
	core.serverkey = 0;
	//sprintf(buf, "DEBUG moj klucz kolejki: %d", ext_queue);
	//add_content_line(&core, gui.content, 0, buf);
	signal(SIGINT, SIG_DFL);
	//int child1 = 0, child2 = 0;
	if(!(res.child_id1 = fork())) {
		signal(SIGINT, SIG_DFL);
		int queue_in = mq_open(res.q_in, O_RDWR), skey = 0, queue_out = mq_open(res.q_out, O_RDWR);
		if(queue_in == -1) {
			perror("Nie mozna otworzyc wewnetrznej kolejki komunikatow (in)");
			return -1;
		}
		compact_message cmg; // m.in. do odpowiadania na HEARTBEAT
		standard_message smg; // m.in. wiadomosci prywatne/kanalowe
		user_list usr;
		message msg;
		// odbieranie
		
		struct timespec tim;
		tim.tv_sec = 0;
		tim.tv_nsec = 1000;	

		//close(server_key[1]);
		char roomname[512];
		close(room[1]);
		while(1) {
			int mid = msgget(ext_queue, 0777);
			char buf[16];
			if(mid == -1) {
				// wyslij komunikat do GUI że się posypała kolejka
				// spróbuj zaalokować nową
			} else {	
				if(receive(ext_queue, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), MSG_HEARTBEAT) != -1) {
					msg.type = M_HEARTBEAT;
					msg.source = cmg.content.value;
					int rcpt = msgget(cmg.content.value, 0777);
					cmg.content.value = core.mykey;
					msgsnd(rcpt, &cmg, sizeof(compact_message), IPC_NOWAIT);
					//mq_send(queue_out, (char*)&msg, MAX_MSG_SIZE, M_HEARTBEAT);
					mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_HEARTBEAT);
				}
				if(receive(ext_queue, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), MSG_JOIN) != -1) {
					int ret = 0;
					do {
						ret = read(room[0], &roomname, 512);
					} while(ret == 0);
					msg.type = M_JOIN;
					strcpy(msg.content.room, roomname);
					mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_JOIN);
				}
				if(receive(ext_queue, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), MSG_REGISTER) != -1) {
					int /*mid = -1, */len = -1, val;
					switch(cmg.content.value) {
						case 0:
							msg.type = M_REGISTER;
							strcpy(msg.content.name, cmg.content.sender);
							mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_REGISTER);
							//printf("ret = %d\n", ret);
							msg.type = M_JOIN;
							strcpy(msg.content.room, GLOBAL_ROOM_NAME);
							mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_JOIN);
							msg.type = M_USERLIST;
							msg.source = core.mykey;
							len = 0;
							do {
								len = read(server_key[0], &buf, 16);
							} while (len == 0);
							write(server_key[1], &buf, 16);
							
							val = atoi(buf);
							if(skey != val) skey = val;
							if(skey != 0) {
								msg.dest = skey;
								mq_send(queue_out, (char*)&msg, MAX_MSG_SIZE, M_USERLIST);
							}
								//msgsnd(mid, &cmg, /*member_size(compact_message, content)*/sizeof(compact_message), IPC_NOWAIT);
						break;
						case -1: // nick istnieje
							msg.type = M_ERROR;
							strcpy(msg.content.message, "Blad rejestracji: nick uzywany");
							mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_ERROR);
						break;
						case -2: // serwer pelen
							msg.type = M_ERROR;
							strcpy(msg.content.message, "Blad rejestracji: serwer pelen");
							mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_ERROR);
						break;
					}
				}
				if(receive(ext_queue, &smg, /*member_size(standard_message, content)*/sizeof(standard_message), MSG_ROOM) != -1) {
					msg.type = M_MESSAGE;
					strcpy(msg.content.message, smg.content.message);
					strcpy(msg.content.name, smg.content.sender);
					msg.content.date = smg.content.send_date;
					mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_MESSAGE);
				}
				if(receive(ext_queue, &smg, /*member_size(standard_message, content)*/sizeof(standard_message), MSG_PRIVATE) != -1) {
					msg.type = M_PRIVATE;
					strcpy(msg.content.message, smg.content.message);
					strcpy(msg.content.name, smg.content.sender);
					mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_PRIVATE);
				}
				if(receive(ext_queue, &usr, /*member_size(user_list, content)*/sizeof(user_list), MSG_LIST) != -1) {
					msg.type = M_USERLIST;
					int i = 0;
					for(; i < MAX_USER_LIST_LENGTH; i++) {
						strcpy(msg.content.list[i], usr.content.list[i]);
					}
					mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_USERLIST);
				}
			}
			nanosleep(&tim, NULL);
		}
		// tu będzie odbieranie msgrcv() komunikatów zwykłych (najlepiej w jakiejś zewnętrznej funkcji) + wysyłanie danych do kolejki wewnętrznej w razie potrzeby
		//}
	} else {	// interfejs uzytkownika
		if(!(res.child_id2 = fork())) {
			signal(SIGINT, SIG_DFL);
			struct pollfd ufds;
			int queue_out = mq_open(res.q_out, O_RDWR);
			if(queue_out == -1) {
				perror("Nie mozna otworzyc wewnetrznej kolejki komunikatow (out)");
				return -1;
			}
			ufds.fd = queue_out;
			ufds.events = POLLIN;
			while(1) {
				switch(poll(&ufds, 1, 10)) {
					default:
					if(ufds.revents && POLLIN)	// tu jakis callback do wysyłania np. get_and_send() w out_callback
						out_callback(queue_out);
						//key_callback(&gui, &core, buffer);
				}
				
			}
			// wysylanie
		} else {
			if(!(res.child_id3 = fork())) {
				int queue_out = mq_open(res.q_out, O_RDWR);
				if(queue_out == -1) {
					endwin();
					perror("Nie mozna otworzyc wewnetrznej kolejki");
					kill(getppid(), SIGKILL);
					exit(-1);
				}
				struct timespec tim;
				tim.tv_sec = 2;
				tim.tv_nsec = 0;
				//char buf[512];
				int skey = 0;
				//close(server_key[1]);
				while(1) {
					//sprintf(buf, "[DEBUG] core.serverkey: %d", core.serverkey);
					//add_content_line(&core, gui.content, 1, buf);
					char buf[16];
					read(server_key[0], &buf, 16);
					int val = atoi(buf);
					if(val != skey && skey > 0)	skey = val;
					if(skey > 0) {
						endwin();
						printf("skey: %d\n");
						exit(-1);
						message msg;
						msg.type = M_USERLIST;
						msg.source = core.mykey;
						msg.dest = skey;
						mq_send(queue_out, (char*)&msg, MAX_MSG_SIZE, M_USERLIST);
						sprintf(buf, "%d", skey);
						write(server_key[1], &buf, 16);
						//add_content_line(&core, gui.content, 1, "[DEBUG] Trying to reach server...");
					}
					nanosleep(&tim, NULL);
				}
			} else {
			signal(SIGINT, sighandler);
			int /*i = 0, */ufds_size = 2;
			struct pollfd ufds[2];

			ufds[0].fd = STDIN_FILENO;
			ufds[0].events = POLLIN;
			ufds[1].fd = int_queue_in;
			ufds[1].events = POLLIN;
	
			while(1) {
				//mvwprintw(gui.user_list, 0, 0, "%s", userlist_buffer);
				/*for(i = 0; i < LINES; i++) { // naprawic zeby tylko wyswietlalo gdy jest jakas zmiana
					mvwprintw(gui.user_list, i, 0, "%s", core.userlist[i]);
				}
				wrefresh(gui.user_list);*/
				wmove(gui.mainwindow, core.cursor_y, (strcmp(core.room, "(status)") == 0 ? strlen(core.room)+3+core.cursor_x : strlen(core.room)+4+core.cursor_x));
				wrefresh(gui.mainwindow);
				//refresh();
	
				switch(poll(ufds, ufds_size, 10)) {
					case -1: // np. resize terminala
						redraw(&gui, &core);
						//refresh();
					break;
					default:
						if(ufds[0].revents && POLLIN)
							key_callback(&gui, &core, buffer, int_queue_out);
						if(ufds[1].revents && POLLIN)
							in_callback(&gui, &core, int_queue_in);
				}
			} // while(1)
			}
		} // else
	} // else forka
	return 0;
}
