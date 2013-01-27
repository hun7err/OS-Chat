#include "../include/window.h"

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

void draw_gui(WINDOW *win, core_t *c) {
	start_color();
	init_pair(1, COLOR_BLUE, COLOR_BLUE);
	wattron(win, COLOR_PAIR(1));
	// gorna i dolna belka
	int i = 0, j = 0;
	for(j = 0; j < LINES; j += LINES-2)
		for(i = 0; i < COLS; i++)
			mvwprintw(win, j, i, " "); // "%c", ' ')
	wattroff(win, COLOR_PAIR(1));
	init_pair(2, COLOR_WHITE, COLOR_BLUE);
	wattron(win, COLOR_PAIR(2));
	mvwprintw(win, 0, 1, "%s", " SOP2 chat v0.666 [pre-alpha]");
	wattroff(win, COLOR_PAIR(2));
	use_default_colors();
	mvwprintw(win, LINES-1, 0, "[%s]", c->room);
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
