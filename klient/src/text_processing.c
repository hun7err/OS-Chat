#include "../include/communication.h"
#include "../include/text_processing.h"
#include <stdio.h>
#include <errno.h>

int stoi(char *c) {
	char *tmp = c;
	int ret = 0;
	for(; *tmp != '\0'; tmp++) {
		ret = 10*ret + *tmp - 48;
	}
	return ret;
}

void key_callback(gui_t *g, core_t *c, char *buf, int outfd) {
	char chh/*, buff[128]*/;
	int count = read(STDIN_FILENO, &chh, 1);
	if(count == -1) {
		perror("Blad czytania z klawiatury");
		exit(1);
	}
	int ch = chh/*, ret*/;
	message msg;
	time_t t;
	switch(ch) {
		case 127: // BACKSPACE
			if(c->cursor_x > 0) {
				buf[c->cursor_x] = '\0';
				(c->cursor_x)--;
				mvwprintw(g->mainwindow, c->cursor_y, (strcmp(c->room, "(status)") == 0 ? strlen(c->room)+3+c->cursor_x : strlen(c->room)+4+c->cursor_x), "%c", ' ');
				wrefresh(g->mainwindow);
			}
		break;
		case 10: // \n
		case 13: // \r
			if(buf[0] == '/') parse_cmd(g, c, buf, outfd);
			else if(c->serverkey != 0) {
				strcpy(msg.content.room, c->room);
				msg.type = M_MESSAGE;
				strcpy(msg.content.message, buf);
				strcpy(msg.content.room, c->room);
				//msg.content.send_date = time(NULL);
				strcpy(msg.content.name, c->nick);
				msg.dest = c->serverkey;
				//add_info(c, g->content, "[DEBUG] Wiadomosc wyslana", &t);
				/*ret = */mq_send(outfd, (char*)&msg, MAX_MSG_SIZE, M_MESSAGE);
				//sprintf(buff, "[DEBUG] Wynik mq_send: %d, errno: %d", ret, errno);
				//add_info(c, g->content, buff, &t);
				//sprintf(buff, "Msg size: %lu, max msg size: %lu", sizeof(msg), MAX_MSG_SIZE);
				//add_info(c, g->content, buff, &t);
				t = time(NULL);
				add_msg(c, g->content, c->nick, buf, &t, 1);
				// wyslij wiadomosc np.
			}
			int i = 0;
			for(i = (strcmp(c->room, "(status)") == 0 ? strlen(c->room)+3 : strlen(c->room)+4); i < COLS; i++)
				mvwprintw(g->mainwindow, LINES-1, i, " ");
			wrefresh(g->mainwindow);
			buf = "";
			//sprintf(buf, "");
			c->cursor_x = 0;
		break;
		default:
			// jesli czytalny, to:
			if(ch >= 32 && ch <= 126) {
				buf[c->cursor_x] = ch;
				c->cursor_x++;
				buf[c->cursor_x] = '\0';
				mvwprintw(g->mainwindow, c->cursor_y, (strcmp(c->room, "(status)") == 0 ? strlen(c->room)+3 : strlen(c->room)+4), "%s", buf);
				wrefresh(g->mainwindow);
			}
	}
}

void out_callback(int fd) {
	message msg;
	int len;
	do {
		len = mq_receive(fd, (char*)&msg, MAX_MSG_SIZE, NULL);
	} while (len <= 0);
	
	if (len > -1) {
	//printf("o, mam cos!\n");
	int k = msg.dest, mid = -1;
	switch(msg.type) {
		case M_REGISTER:
			mid = msgget(k, 0777);
			if(mid != -1) {
				compact_message cmg;
				cmg.type = MSG_REGISTER;
				strcpy(cmg.content.sender, msg.content.name);
				cmg.content.value = msg.source;
				//msgsnd(mid, &cmg, member_size(compact_message,content), IPC_NOWAIT);
				msgsnd(mid, &cmg, sizeof(compact_message), IPC_NOWAIT);
			}
		break;
		case M_MESSAGE:
			mid = msgget(k, 0777);
			if(mid != -1) {
				standard_message smg;
				strcpy(smg.content.sender,msg.content.name);
				strcpy(smg.content.recipient, msg.content.room);
				smg.content.send_date = time(NULL);
				strcpy(smg.content.message, msg.content.message);
				smg.type = MSG_ROOM;
				//msgsnd(mid, &smg, member_size(standard_message, content), IPC_NOWAIT);
				msgsnd(mid, &smg, sizeof(standard_message), IPC_NOWAIT);
			}
		break;
		case M_USERLIST:
			mid = msgget(k, 0777);
			if(mid != -1) {
				compact_message cmg;
				cmg.type = MSG_LIST;
				cmg.content.value = msg.source;
				//msgsnd(mid, &cmg, member_size(compact_message, content), IPC_NOWAIT);
				msgsnd(mid, &cmg, sizeof(compact_message), IPC_NOWAIT);
			}
			// wyslij zapytanie MSG_LIST o liste
		break;
		case M_PRIVATE:
			mid = msgget(k, 0777);
			if(mid != -1) {
				standard_message smg;
				smg.type = MSG_PRIVATE;
				strcpy(smg.content.sender, msg.content.name);
				strcpy(smg.content.recipient, msg.content.room);
				smg.content.send_date = msg.content.date;
				strcpy(smg.content.message, msg.content.message);
				//msgsnd(mid, &smg, member_size(standard_message, content), IPC_NOWAIT);
				msgsnd(mid, &smg, sizeof(standard_message), IPC_NOWAIT);
			}
		break;
		case M_JOIN:
			mid = msgget(k, 0777);
			if(mid != -1) {
				standard_message smg;
				smg.type = MSG_JOIN;
				strcpy(smg.content.message, msg.content.room);
				strcpy(smg.content.sender, msg.content.name);
				//msgsnd(mid, &smg, member_size(standard_message, content), IPC_NOWAIT);
				msgsnd(mid, &smg, sizeof(standard_message), IPC_NOWAIT);
			}
		break;
		case M_LEAVE:
			mid = msgget(k, 0777);
			if(mid != -1) {
				compact_message cmg;
				cmg.type = MSG_LEAVE;
				cmg.content.value = msg.source;
				//msgsnd(mid, &cmg, member_size(compact_message, content), IPC_NOWAIT);
				msgsnd(mid, &cmg, sizeof(compact_message), IPC_NOWAIT);
			}
		break;
		case M_UNREGISTER:
			mid = msgget(k, 0777);
			if(mid != -1) {
				compact_message cmg;
				cmg.type = MSG_UNREGISTER;
				cmg.content.value = msg.source;
				//msgsnd(mid, &cmg, member_size(compact_message, content), IPC_NOWAIT);
				msgsnd(mid, &cmg, sizeof(compact_message), IPC_NOWAIT);
			}
		break;
		default:
		break;
	}
	}
}

void gettime(char *buf, time_t *t) {
	*t = time(NULL);
	strftime(buf, 10, "%H:%M:%S", localtime(t));
}

void in_callback(gui_t *g, core_t *c, int fd) {
	//mpact_message m1;
	message msg;
	time_t now;
	//char curtime[10];
	int len;
	//add_content_line(c, g->content, 0, "DEBUG: sprawdzam kolejke");
	do {
		len = mq_receive(fd, (char*)&msg, MAX_MSG_SIZE, NULL);
	} while(len <= 0);
	//add_content_line(c, g->content, 0, "DEBUG: mam wiadomosc");
		int i = 0, j = 0;
		char buf[1024];
		//gettime(curtime, &now);
		switch(msg.type) {
			case M_ERROR:
				sprintf(buf, "%s", msg.content.message);
				add_info(c, g->content, buf, &now);
			break;
			case M_REGISTER:
				sprintf(buf, "Pomyslnie polaczono jako %s", msg.content.name);
				strcpy(c->nick, msg.content.name);
				add_info(c, g->content, buf, &now);
			break;
			case M_JOIN:
				sprintf(buf, "Dolaczasz do #%s", msg.content.room);
				sprintf(c->room, "%s", msg.content.room);
				add_info(c, g->content, buf, &now);
				for(i = 0; i < COLS; i++)
					mvwprintw(g->mainwindow, LINES-1, i, " ");
				mvwprintw(g->mainwindow, LINES-1, 0, "[#%s]", c->room);
				wmove(g->mainwindow, c->cursor_y, strlen(c->room)+3+c->cursor_x);
				wrefresh(g->mainwindow);
			break;
			case M_USERLIST:
				//add_content_line(c, g->content, 0, "[DEBUG] jest lista userow!");
				for(i = 0; i < 16; i++) {
					for(j = 0; j < LINES-1; j++) {
						mvwprintw(g->user_list, j, i, " ");
					}
				}
				for(i = 0; i < MAX_USER_LIST_LENGTH; i++)
					strcpy(c->userlist[i], "");
				for(i = 0; i < MAX_USER_LIST_LENGTH; i++) {
					strcpy(c->userlist[i], msg.content.list[i]);
				}
				for(i = 0; i < (MAX_USER_LIST_LENGTH < LINES ? MAX_USER_LIST_LENGTH : LINES); i++) {
					if(strcmp(c->userlist[i], "") == 0) break;
					mvwprintw(g->user_list, i, 0, "%s", c->userlist[i]);
				}
				wrefresh(g->user_list);
			break;
			case M_MESSAGE:
				add_msg(c, g->content, msg.content.name, msg.content.message, &(msg.content.date), 0);
				//wrefresh(g->content);
			break;
			case M_PRIVATE:
				add_private(c, g->content, msg.content.name, msg.content.message, &(msg.content.date));
			break;
			case M_LEAVE:
				strcpy(c->room, "global");
				add_info(c, g->content, "Powrociles do #global", &now);
				for(i = 0; i < COLS; i++)
					mvwprintw(g->mainwindow, LINES-1, i, " ");
				mvwprintw(g->mainwindow, LINES-1, 0, "[#%s]", c->room);
				wmove(g->mainwindow, c->cursor_y, strlen(c->room)+3+c->cursor_x);
				wrefresh(g->mainwindow);
			break;
			case M_HEARTBEAT:
				/*sprintf(buf, "%d", c->serverkey);
				add_private(c,g->content,buf,"heartbeat",&now);
				sprintf(buf, "%d", msg.source);
				add_private(c,g->content,buf,"moj numer kolejki",&now);*/
			break;
			default:
			break;
		}
	
}

void parse_cmd(gui_t *g, core_t *c, char *cmd, int outfd) {
	close(room[0]);
	time_t t;
	char *temp = cmd+1, *cmain = NULL, *carg1 = NULL, *carg2 = NULL;	// temporary, command main, command arg1, command arg2
	cmain = calloc(64, sizeof(char));
	carg1 = calloc(128, sizeof(char));
	carg2 = calloc(128, sizeof(char));
	int i = 0;
	for(; *temp != ' ' && *temp != '\0'; temp++) {
		cmain[i] = *temp;
		i++;
	}
	cmain[i] = '\0';
	i = 0;
	for(temp++; *temp != ' ' && *temp != '\0'; temp++) {
		carg1[i] = *temp;
		i++;
	}
	carg1[i] = '\0';
	i = 0;
	for(temp++; *temp != '\0'; temp++) {
		carg2[i] = *temp;
		i++;
	}
	carg2[i] = '\0';

	if(strcmp(cmain, "help") == 0) {
		add_info(c, g->content, "", &t);
		add_info(c, g->content, "Dostepne komendy:", &t);
		add_info(c, g->content, "", &t);
		add_info(c, g->content, "/help - wyswietla ta pomoc", &t);
		add_info(c, g->content, "/list - lista uzytkownikow w pokoju", &t);
		add_info(c, g->content, "/connect klucz_serwera nazwa - laczy z serwerem", &t);
		add_info(c, g->content, "o podanym kluczu", &t);
		add_info(c, g->content, "/disconnect - rozlacza z serwerem", &t);
		add_info(c, g->content, "/join kanal - dolacza do kanalu \"kanal\"", &t);
		add_info(c, g->content, "/leave - wychodzi z kanalu na #global", &t);
		add_info(c, g->content, "/msg nick tresc - wysyla prywatna wiadomosc", &t);
		add_info(c, g->content, "/quit - wylacza aplikacje", &t);
	} else if(strcmp(cmain, "quit") == 0) {
		// tutaj jeszcze unregister najpierw
		message msg;
		msg.type = M_UNREGISTER;
		msg.dest = c->serverkey;
		c->serverkey = 0;
		msg.source = c->mykey;
		mq_send(outfd,(char*)&msg, MAX_MSG_SIZE, M_UNREGISTER);
		quit(&res); // dopisać jakąś funkcję która będzie zwalniać zasoby i dopiero wychodzić
	} else if(strcmp(cmain, "msg") == 0) {
		if(strcmp(carg1, "") == 0 || strcmp(carg2, "") == 0) {
			//add_content_line(c, g->content, 0, " -!- Za malo argumentow dla komendy connect");
			add_info(c, g->content, "Za malo argumentow dla 'msg'", &t);
		} else {
			message msg;
			msg.type = M_PRIVATE;
			strcpy(msg.content.name, c->nick);
			strcpy(msg.content.room, carg1);
			strcpy(msg.content.message, carg2);
			msg.source = c->mykey;
			msg.dest = c->serverkey;
			mq_send(outfd, (char*)&msg, MAX_MSG_SIZE, M_PRIVATE);
		}
	} else if(strcmp(cmain, "disconnect") == 0) {
		/*
		sprintf(buf, "Dolaczasz do #%s", msg.content.room);
				sprintf(c->room, "%s", msg.content.room);
				add_info(c, g->content, buf, &now);
				for(i = 0; i < COLS; i++)
					mvwprintw(g->mainwindow, LINES-1, i, " ");
				mvwprintw(g->mainwindow, LINES-1, 0, "[#%s]", c->room);
				wmove(g->mainwindow, c->cursor_y, strlen(c->room)+3+c->cursor_x);
				wrefresh(g->mainwindow);

		*/
		message msg;
		msg.type = M_UNREGISTER;
		msg.dest = c->serverkey;
		c->serverkey = 0;
		msg.source = c->mykey;
		mq_send(outfd,(char*)&msg, MAX_MSG_SIZE, M_UNREGISTER); 
		strcpy(c->room, "(status)");
		add_info(c,g->content,"Rozlaczono", &t);
		for(i = 0; i < COLS; i++)
			mvwprintw(g->mainwindow, LINES-1, i, " ");
		mvwprintw(g->mainwindow, LINES-1, 0, "[%s]", c->room);
		wmove(g->mainwindow, c->cursor_y, strlen(c->room)+3+c->cursor_x);
		wrefresh(g->mainwindow);
		for(i = 0; i < MAX_USER_LIST_LENGTH; i++) strcpy(c->userlist[i], "");
		int j = 0;
		for(i = 0; i < 16; i++) {
			for(j = 0; j < LINES-1; j++) {
				mvwprintw(g->user_list, j, i, " ");
			}
		}
		wrefresh(g->user_list);
		// MSG_UNREGISTER do serwera
	} else if(strcmp(cmain, "join") == 0) {
		//char buf[128];
		//sprintf(buf, "join, carg1: %s, carg2: %s", carg1, carg2);
		//add_info(c,g->content, buf, &t);
		// musi miec jeden argument minimum, MSG_JOIN do serwera
		if(strcmp(carg1, "") == 0) {
			add_info(c, g->content, "za malo argumentow dla 'join'", &t);
		} else {
			message msg;
			msg.type = M_JOIN;
			msg.dest = c->serverkey;
			//msg.source = c->mykey;
			strcpy(msg.content.room, carg1);
			write(room[1], &(msg.content.room), 512);
			//char buf[512];
			//strcpy(buf, carg1);
			//read(room[0], &buf, 512);
			//write(room[1], &buf, 512);
			//add_info(c, g->content, buf, &t);
			strcpy(msg.content.name, c->nick);
			mq_send(outfd,(char*)&msg, MAX_MSG_SIZE, M_JOIN); 
		}
	} else if(strcmp(cmain, "leave") == 0) {
		message msg;
		msg.type = M_LEAVE;
		msg.dest = c->serverkey;
		msg.source = c->mykey;
		mq_send(outfd,(char*)&msg, MAX_MSG_SIZE, M_LEAVE);
		// wyslij MSG_LEAVE do serwera
	} else if(strcmp(cmain, "list") == 0) {
		char buf[512];
		strcpy(buf, "Aktualnie na ");
		sprintf(buf, "%s#%s:", buf, c->room);
		add_info(c,g->content,buf,&t);
		strcpy(buf, "");
		int i = 0;
		for(i = 0; i < MAX_USER_LIST_LENGTH; i++) {
			if(strcmp(c->userlist[i], "") == 0) break;
			sprintf(buf, "%s %s", buf, c->userlist[i]);
		}
		add_info(c,g->content,buf,&t);
	} else if(strcmp(cmain, "connect") == 0) {
		if(strcmp(carg1, "") == 0 || strcmp(carg2, "") == 0) {
			//add_content_line(c, g->content, 0, " -!- Za malo argumentow dla komendy connect");
			add_info(c, g->content, "Za malo argumentow dla 'connect'", &t);
		} else {
		char buf[128];
		sprintf(buf, "Lacze sie z kolejka %d jako %s", atoi(carg1), carg2);
		add_info(c, g->content, buf, &t);
		message msg;

		msg.type = M_REGISTER;
		strcpy(msg.content.name, carg2);
		msg.dest = atoi(carg1);
		c->serverkey = msg.dest;
		close(server_key[0]);
		write(server_key[1], carg1, strlen(carg1));
		msg.source = c->mykey; // POPRAWIC! KONIECZNIE! ma byc klucz z res
		int len = mq_send(outfd, (char*)&msg, MAX_MSG_SIZE, M_REGISTER);
		if(len == -1) {
			add_info(c, g->content, "Blad mq_send w 136 linii text_processing.c", &t);
			exit(-1);
		}
		//add_content_line(c, g->content, 0, "-!- wiadomosc poprawnie wyslana");
		//c->serverkey = stoi(carg1);
		// wyslanie lokalnie
		}
	} else {
		char buf[512];
		sprintf(buf, " -!- Unknown command '%s'", cmain);
		add_content_line(c, g->content, 0, buf);
	}

	// wydziel argument jako druga czesc komendy
	// tutaj switchuj komendy (pierwsza czesc) i przetwarzaj je
	//mvwprintw(g->content, 3, 0, "%s", cmain);
	//mvwprintw(g->content, 4, 0, "%s", carg1);
	//mvwprintw(g->content, 5, 0, "%s", carg2);
	wrefresh(g->content);
	free(cmain);
	free(carg1);
	free(carg2);
	cmain = NULL;
	carg1 = NULL;
	carg2 = NULL;
	temp = NULL;
	//free(command);
	//free(cmain);
	//command = NULL;
	//cmain = NULL;
}
