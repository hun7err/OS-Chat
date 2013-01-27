#include "../include/text_processing.h"
#include <stdio.h>

void key_callback(gui_t *g, core_t *c, char *buf/*, int *cx, int *cy*/) {
	char chh;
	int count = read(STDIN_FILENO, &chh, 1);
	if(count == -1) {
		perror("Blad czytania z klawiatury");
		exit(1);
	}
	int ch = chh;
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
			if(buf[0] == '/') parse_cmd(g, c, buf);
			else {
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

void parse_cmd(gui_t *g, core_t *c, char *cmd) {
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
		quit(); // dopisać jakąś funkcję która będzie zwalniać zasoby i dopiero wychodzić
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
