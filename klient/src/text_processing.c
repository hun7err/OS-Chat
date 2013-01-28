#include "../include/communication.h"
#include "../include/text_processing.h"
#include <stdio.h>

int stoi(char *c) {
	char *tmp = c;
	int ret = 0;
	for(; *tmp != '\0'; tmp++) {
		ret = 10*ret + *tmp - 48;
	}
	return ret;
}

void key_callback(gui_t *g, core_t *c, char *buf, int outfd) {
	char chh;
	int count = read(STDIN_FILENO, &chh, 1);
	if(count == -1) {
		perror("Blad czytania z klawiatury");
		exit(1);
	}
	int ch = chh;
	message msg;
	switch(ch) {
		case 127: // BACKSPACE
			if(c->cursor_x > 0) {
				buf[c->cursor_x] = '\0';
				(c->cursor_x)--;
				mvwprintw(g->mainwindow, c->cursor_y, strlen(c->room)+3+c->cursor_x, "%c", ' ');
				wrefresh(g->mainwindow);
			}
		break;/*
		case KEY_ESCAPE:
			endwin();
			exit(0);
		break;*/
		case 10:
		case 13: // ENTER (wtf)
			if(buf[0] == '/') parse_cmd(g, c, buf, outfd);
			else {
				strcpy(msg.content.room, c->room);
				msg.type = M_MESSAGE;
				strcpy(msg.content.message, buf);
				mq_send(outfd, (char*)&msg, MAX_MSG_SIZE, M_MESSAGE);
				// wyslij wiadomosc np.
			}
			int i = 0;
			for(;i<COLS-strlen(c->room)+4; i++)
				mvwprintw(g->mainwindow, c->cursor_y, strlen(c->room)+3+i, " ");
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
			mvwprintw(g->mainwindow, c->cursor_y, strlen(c->room)+3, "%s", buf);
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
				msgsnd(mid, &cmg, member_size(compact_message,content), 0);
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
				msgsnd(mid, &smg, member_size(standard_message, content), 0);
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
	char curtime[10];
	int len;
	//add_content_line(c, g->content, 0, "DEBUG: sprawdzam kolejke");
	do {
		len = mq_receive(fd, (char*)&msg, MAX_MSG_SIZE, NULL);
	} while(len <= 0);
	//add_content_line(c, g->content, 0, "DEBUG: mam wiadomosc");
		int i = 0;
		char buf[1024];
		gettime(curtime, &now);
		switch(msg.type) {
			case M_ERROR:
				sprintf(buf, "%s -!- %s", curtime, msg.content.message);
				add_content_line(c, g->content, 0, buf);
			break;
			case M_REGISTER:
				sprintf(buf, "%s -!- Pomyslnie polaczono jako %s", curtime, msg.content.name);
				strcpy(c->nick, msg.content.name);
				add_content_line(c, g->content, 0, buf);
			break;
			case M_JOIN:
				sprintf(buf, "%s -!- Dolaczasz do %s", curtime, msg.content.room);
				sprintf(c->room, "#%s", msg.content.room);
				add_content_line(c, g->content, 0, buf);
				for(i = 0; i < COLS; i++)
					mvwprintw(g->mainwindow, LINES-1, i, " ");
				mvwprintw(g->mainwindow, LINES-1, 0, "[%s]", c->room);
				wmove(g->mainwindow, c->cursor_y, strlen(c->room)+3+c->cursor_x);
				wrefresh(g->mainwindow);
			break;
			default:
			break;
		}
	
}

void parse_cmd(gui_t *g, core_t *c, char *cmd, int outfd) {
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
	for(temp++; *temp != ' ' && *temp != '\0'; temp++) {
		carg2[i] = *temp;
		i++;
	}
	carg2[i] = '\0';

	if(strcmp(cmain, "help") == 0) {
	//add_content_line(&core, gui.content, 0, buf);
		add_content_line(c, g->content, 0, "-!- Dostepne komendy:");
		add_content_line(c, g->content, 0, "-!- /help - wyswietla ta pomoc");
		add_content_line(c, g->content, 0, "-!- /connect klucz_serwera nazwa - laczy z serwerem o kluczu podanym jako pierwszy argument");
		add_content_line(c, g->content, 0, "-!- logujac z nazwa uzytkownika bedaca drugim argumentem");
		add_content_line(c, g->content, 0, "-!- /disconnect - rozlacza z serwerem");
		add_content_line(c, g->content, 0, "-!- /join kanal - dolacza do kanalu \"kanal\"");
		add_content_line(c, g->content, 0, "-!- /leave - wychodzi z kanalu przekierowujac na global, wywolana z global rozlacza");
		add_content_line(c, g->content, 0, "-!- /msg nazwa tresc - wysyla prywatna wiadomosc do uzytkownika o nazwie \"nazwa\"");
		add_content_line(c, g->content, 0, "-!- /quit - wylacza komunikator");
	} else if(strcmp(cmain, "quit") == 0) {
		quit(&res); // dopisać jakąś funkcję która będzie zwalniać zasoby i dopiero wychodzić
	} else if(strcmp(cmain, "connect") == 0) {
		if(strcmp(carg1, "") == 0 || strcmp(carg2, "") == 0) {
			add_content_line(c, g->content, 0, " -!- Za malo argumentow dla komendy connect");
		} else {
		char buf[128];
		sprintf(buf, "-!- Connecting to message queue %d as %s", stoi(carg1), carg2);
		add_content_line(c, g->content, 0, buf);
		message msg;
		msg.type = M_REGISTER;
		strcpy(msg.content.name, carg2);
		msg.dest = stoi(carg1);
		msg.source = c->mykey; // POPRAWIC! KONIECZNIE! ma byc klucz z res
		add_content_line(c, g->content, 0, "-!- Sending message through internal queue...");
		int len = mq_send(outfd, (char*)&msg, MAX_MSG_SIZE, M_REGISTER);
		if(len == -1) {
			perror("Blad mq_send w 136 linii text_processing.c");
			exit(-1);
		}
		add_content_line(c, g->content, 0, "-!- wiadomosc poprawnie wyslana");
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
