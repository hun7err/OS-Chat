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
	} while (qid == -1);
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

	draw_gui(gui.mainwindow, &core);

	core.cursor_x = 0;
	core.cursor_y = LINES-1;
	char cur_time[9], userlist_buffer[1024], buffer[1024];
	time_t now = time(NULL);
	strftime(cur_time, 9, "%H:%M:%S", localtime(&now));
	cur_time[8] = '\0';
	char buf[512];
	sprintf(buf, "-!- client started at %s", cur_time);
	add_content_line(&core, gui.content, 0, buf);
	add_content_line(&core, gui.content, 0, "-!- type /help for help");

	
	//add_user_to_list(&core, "hun7er");
	//add_user_to_list(&core, "testowy");
	//add_content_line(&core, gui.content, 0, "-!- uzytkownik mescam dolaczyl do #global");
	
	scrollok(gui.content, 1);
	
	refresh();
	int int_queue_in, int_queue_out, ext_queue; // internal queue, external queue
	
	struct mq_attr attr;
	attr.mq_msgsize = MAX_MSG_SIZE;
	attr.mq_maxmsg = 10;
	
	res.int_in_fd = int_queue_in = mq_open("/internal_in", O_RDWR|O_CREAT, 0777, &attr);
	res.int_out_fd = int_queue_out = mq_open("/internal_out", O_RDWR|O_CREAT, 0777, &attr);

	if(int_queue_in == -1 || int_queue_out == -1) {
		endwin();
		printf("max msg size: %lu\n", MAX_MSG_SIZE);
		perror("Nie mozna utworzyc wewnetrznych kolejek komunikatow");
		printf("Numer bledu: %d\n", errno);
		return -1;
	}

	// internal: obrobione dane do wyswietlenia
	// external: "surowe" dane do obrobienia przez forki

	ext_queue = DEFAULT_QUEUE_KEY;
	get_key(&ext_queue);
	core.mykey = res.queue_key = ext_queue;
	sprintf(buf, "DEBUG moj klucz kolejki: %d", ext_queue);
	add_content_line(&core, gui.content, 0, buf);
	//int child1 = 0, child2 = 0;
	if(!(res.child_id1 = fork())) {
		if(!(res.child_id2 = fork())) {
			signal(SIGINT, SIG_DFL);
			struct pollfd ufds;
			int queue_out = mq_open("/internal_out", O_RDWR);
			if(queue_out == -1) {
				perror("Nie mozna otworzyc wewnetrznej kolejki komunikatow (out)");
				return -1;
			}
			ufds.fd = queue_out;
			ufds.events = POLLIN;
			while(1) {
				switch(poll(&ufds, 1, -1)) {
					default:
					if(ufds.revents && POLLIN)	// tu jakis callback do wysyłania np. get_and_send() w out_callback
						out_callback(queue_out);
						//key_callback(&gui, &core, buffer);
				}
				
			}
			// wysylanie
		} else {
			signal(SIGINT, SIG_DFL);
			int queue_in = mq_open("/internal_in", O_RDWR);
			if(queue_in == -1) {
				perror("Nie mozna otworzyc wewnetrznej kolejki komunikatow (in)");
				return -1;
			}
			compact_message cmg; // m.in. do odpowiadania na HEARTBEAT
			standard_message smg; // m.in. wiadomosci prywatne/kanalowe
			user_list usr;
			message msg;
			// odbieranie
			while(1) {
				int mid = msgget(ext_queue, 0777);
				if(mid == -1) {
					// wyslij komunikat do GUI że się posypała kolejka
					// spróbuj zaalokować nową
				} else {
					int len = msgrcv(mid, &cmg, member_size(compact_message, content), -MSG_SERVER, IPC_NOWAIT), ret;
					if(len != -1) {
						//printf("Mam wiadomosc\n");
						switch(cmg.type) {
							case MSG_HEARTBEAT:
							break;
							case MSG_REGISTER:
								switch(cmg.content.value) {
									case 0:
										msg.type = M_REGISTER;
										strcpy(msg.content.name, cmg.content.sender);
										mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_REGISTER);
										msg.type = M_JOIN;
										strcpy(msg.content.room, GLOBAL_ROOM_NAME);
										ret = mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_JOIN);
									break;
									case -1: // nick istnieje
										msg.type = M_ERROR;
										strcpy(msg.content.message, "Blad rejestracji: nick uzywany");
										ret = mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_ERROR);
									break;
									case -2: // serwer pelen
										msg.type = M_ERROR;
										strcpy(msg.content.message, "Blad rejestracji: serwer pelen");
										ret = mq_send(queue_in, (char*)&msg, MAX_MSG_SIZE, M_ERROR);
									break;
								}
							break;
							default:
							break;
						}
						if(ret == -1) {
							perror("Blad mq_send");
							printf("Numer bledu: %d\n");
							exit(-1);
						}
					}
					len = msgrcv(mid, &smg, member_size(standard_message, content), -MSG_SERVER, IPC_NOWAIT);
					if(len != -1) {
						switch(smg.type) {
							case MSG_ROOM:
							break;
							case MSG_PRIVATE:
							break;
							default:
							break;
						}
					}
					len = msgrcv(mid, &usr, member_size(user_list, content), -MSG_SERVER, IPC_NOWAIT);
					if(len != -1) {
						switch(usr.type) {
							case MSG_LIST:
							break;
							default:
							break;
						}
					}
				}
			}
			// tu będzie odbieranie msgrcv() komunikatów zwykłych (najlepiej w jakiejś zewnętrznej funkcji) + wysyłanie danych do kolejki wewnętrznej w razie potrzeby
		}
	} else {	// interfejs uzytkownika
		signal(SIGINT, sighandler);
		int i = 0, ufds_size = 2;
		struct pollfd ufds[2];

		ufds[0].fd = STDIN_FILENO;
		ufds[0].events = POLLIN;
		ufds[1].fd = int_queue_in;
		ufds[1].events = POLLIN;

		while(1) {
			mvwprintw(gui.user_list, 0, 0, "%s", userlist_buffer);
			for(i = 0; i < LINES; i++) { // naprawic zeby tylko wyswietlalo gdy jest jakas zmiana
				mvwprintw(gui.user_list, i, 0, "%s", core.userlist[i]);
			}
			wrefresh(gui.user_list);
			wmove(gui.mainwindow, core.cursor_y, strlen(core.room)+3+core.cursor_x);
			wrefresh(gui.mainwindow);
			//refresh();

			switch(poll(ufds, ufds_size, -1)) {
				case -1:
					redraw(&gui, &core);
					//refresh();
				break;
				default:
					if(ufds[0].revents && POLLIN)
						key_callback(&gui, &core, buffer, int_queue_out);
					if(ufds[1].revents && POLLIN)
						in_callback(&gui, &core, int_queue_in);
			}
		}
	}
	//}
	return 0;
}

/*void process_key(gui_t g, int ch, int *cx, int *cy) {
	switch(ch) {
		case KEY_BACKSPACE:
			if(*cx >= 12) {
				(*cx)--;
				mvwprintw(g.mainwindow, *cy, *cx, "%c", ' ');
				wrefresh(g.mainwindow);
			}
		break;
		case KEY_ESCAPE:
			endwin();
			exit(0);
		break;
		case KEY_RESIZE:
			destroy_window(g.mainwindow);
			destroy_window(g.content);
			destroy_window(g.user_list);
			g.mainwindow = create_window(LINES, COLS, 0, 0);
			draw_gui(g.mainwindow);
			g.content = create_window(LINES-3,COLS-16-1,1,1);
			g.user_list = create_window(LINES-3,16,1,COLS-16-1);
			*cy = LINES-1;
			// tutaj gdzies dac wypisanie bufora z tekstem
			move(*cy, *cx);
			wrefresh(g.mainwindow);
			wrefresh(g.content);
			wrefresh(g.user_list);
		break;
		default:
			(*cx)++;
		break;
	}
}*/
