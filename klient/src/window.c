#include "../include/window.h"

#define C_INFO1 3
#define C_STATUSBAR1 1
#define C_STATUSBAR2 2
#define C_NICK1 4
#define C_DEFAULT 5
#define C_PRIVATE 6

void init_colors(void) {
	start_color();
	use_default_colors();
	init_pair(C_STATUSBAR1, COLOR_BLUE, COLOR_BLUE);
	init_pair(C_STATUSBAR2, -1, COLOR_BLUE);
	init_pair(C_INFO1, COLOR_BLUE, -1);
	init_pair(C_NICK1, COLOR_CYAN, -1);
	init_pair(C_DEFAULT, -1, -1);
	init_pair(C_PRIVATE, COLOR_MAGENTA, -1);
}

void redraw(gui_t *g, core_t *c) {
	destroy_window(g->mainwindow);
	destroy_window(g->content);
	destroy_window(g->user_list);
	g->mainwindow = create_window(LINES-3,COLS-16-2,1,1);
	g->content = create_window(LINES-3, COLS-16-2, 1, 1);
	g->user_list = create_window(LINES-3,16,1,COLS-16-1);
	c->cursor_y = LINES-1;
	//int len = strlen(c->room);
	wrefresh(g->content); // tu się sypie [nie wiedzieć czemu :x]
	wrefresh(g->user_list);
	wmove(g->mainwindow, c->cursor_y, strlen(c->room)+3+c->cursor_x);
	draw_gui(g->mainwindow, c);
	wrefresh(g->mainwindow);
	refresh();
}

void add_content_line(core_t *c, WINDOW *content, int x, const char *msg) {
	if(c->cur_line > LINES-4)
		scroll(content);
	mvwprintw(content, (c->cur_line > LINES-4 ? LINES-4 : c->cur_line), x, msg);
	wrefresh(content);
	c->cur_line++;
}

void add_private(core_t *c, WINDOW *content, const char *nick, const char *msg, time_t *t) {
	if(c->cur_line > LINES-4)
		scroll(content);
	char cur_time[25];
	int cur_x = 0, cur_y = (c->cur_line > LINES-4 ? LINES-4 : c->cur_line);
	strftime(cur_time, 9, "%H:%M:%S", localtime(t));
	cur_time[8] = '\0';
	mvwprintw(content, cur_y, cur_x, cur_time);
	cur_x += 9;
	wattron(content, COLOR_PAIR(C_PRIVATE));
	wattron(content, A_BOLD);
	mvwprintw(content, cur_y, cur_x, "%s: ", nick);
	wattroff(content, A_BOLD);
	cur_x += strlen(nick)+2;
	mvwprintw(content, cur_y, cur_x, msg);
	wattroff(content, COLOR_PAIR(C_PRIVATE));
	wrefresh(content);
	c->cur_line++;
}

void add_info(core_t *c, WINDOW *content, const char *msg, time_t *t) {
	*t = time(NULL);
	if(c->cur_line > LINES-4)
		scroll(content);
	char cur_time[25];
	strftime(cur_time, 9, "%H:%M:%S", localtime(t));
	cur_time[8] = '\0';
	int cur_x = 0, cur_y = (c->cur_line > LINES-4 ? LINES-4 : c->cur_line);
	mvwprintw(content, cur_y, cur_x, cur_time);
	cur_x += 9;
	wattron(content, COLOR_PAIR(C_INFO1));
	mvwprintw(content, cur_y, cur_x, "-");
	wattroff(content, COLOR_PAIR(C_INFO1));
	cur_x++;
	wattron(content, COLOR_PAIR(C_DEFAULT));
	mvwprintw(content, cur_y, cur_x, "!");
	wattroff(content, COLOR_PAIR(C_DEFAULT));
	cur_x++;
	wattron(content, COLOR_PAIR(C_INFO1));
	mvwprintw(content, cur_y, cur_x, "-");
	wattroff(content, COLOR_PAIR(C_INFO1));
	cur_x += 2;
	wattron(content, A_BOLD);
	mvwprintw(content, cur_y, cur_x, msg);
	wattroff(content, A_BOLD);
	wrefresh(content);
	c->cur_line++;
}

void add_msg(core_t *c, WINDOW *content, const char *nick, const char *msg, time_t *t, char bold) {
	//*t = time(NULL);
	if(c->cur_line > LINES-4)
		scroll(content);
	char cur_time[25];
	strftime(cur_time, 9, "%H:%M:%S", localtime(t));
	cur_time[8] = '\0';
	int cur_x = 0, cur_y = (c->cur_line > LINES-4 ? LINES-4 : c->cur_line);
	//start_color();
	//wattron(content, COLOR_PAIR(C_DEFAULT));
	mvwprintw(content, cur_y, cur_x, cur_time);
	cur_x += 9;
	//wattroff(content, COLOR_PAIR(C_DEFAULT));
	wattron(content, COLOR_PAIR(C_NICK1));
	mvwprintw(content, cur_y, cur_x, "<");
	cur_x++;
	wattroff(content, COLOR_PAIR(C_NICK1));
	wattron(content, COLOR_PAIR(C_DEFAULT));
	if(bold == 1) wattron(content, A_BOLD);
	mvwprintw(content, cur_y, cur_x, nick);
	if(bold == 1) wattroff(content, A_BOLD);
	wattroff(content, COLOR_PAIR(C_DEFAULT));
	cur_x += strlen(nick);
	wattron(content, COLOR_PAIR(C_NICK1));
	mvwprintw(content, cur_y, cur_x, "> ");
	wattroff(content, COLOR_PAIR(C_NICK1));
	cur_x += 2;
	wattron(content, COLOR_PAIR(C_DEFAULT));
	mvwprintw(content, cur_y, cur_x, msg);
	wattroff(content, COLOR_PAIR(C_DEFAULT));
	//use_default_colors();
	wrefresh(content);
	c->cur_line++;
}

void draw_gui(WINDOW *win, core_t *c) {
	//start_color();
	//init_pair(1, COLOR_BLUE, COLOR_BLUE);
	wattron(win, COLOR_PAIR(C_STATUSBAR1));
	// gorna i dolna belka
	int i = 0, j = 0;
	for(j = 0; j < LINES; j += LINES-2)
		for(i = 0; i < COLS; i++)
			mvwprintw(win, j, i, " "); // "%c", ' ')
	wattroff(win, COLOR_PAIR(C_STATUSBAR1));
	//init_pair(2, COLOR_WHITE, COLOR_BLUE);
	wattron(win, COLOR_PAIR(C_STATUSBAR2));
	mvwprintw(win, 0, 1, "%s", " SOP2 chat v0.666 [work in progress]");
	wattroff(win, COLOR_PAIR(C_STATUSBAR2));
	use_default_colors();
	int val = strcmp(c->room, "(status)");
	mvwprintw(win, LINES-1, 0, (val == 0 ? "[%s]" : "[#%s]"), c->room);
	wrefresh(win);
}

WINDOW *create_window(int height, int width, int starty, int startx) {
	WINDOW *local_win;
	local_win = newwin(height, width, starty, startx);
	/*
		for the record:
		trzeba obslugiwac recznie wszystkie "pisalne" klawisze i wyswietlac je na ekranie/kasowac przy BACKSPACE i strzalkami przesuwac kursor po oknie (lewo-prawo), gora-dol ignorowac
		* buforowac wiadomosci w jakims potoku zeby wyswietlac LINES-1 - 2 = LINES-3 linie najnowsze ale w kolejnosci odwrotnej (jak w irssi)
		* ogarnac ten burdel w kodzie ;f
		* dodac funkcje draw_gui(void) i wywolywac ja w create_window wlasnie
		* dodac funkcje wypisujace odpowiednie rzeczy jak np. komunikaty aplikacji klienta w -!- 
		* dorobic tryb verbose wypisujacy dodatkowe informacje do debuggowania
	*/
	wrefresh(local_win);
	return local_win;
}

void destroy_window(WINDOW *local_win) {
	wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
	wrefresh(local_win);
	delwin(local_win);
}
