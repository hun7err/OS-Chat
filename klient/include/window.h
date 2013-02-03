#ifndef WINDOW_H
#define WINDOW_H

#include <ncurses.h>
#include "../include/core.h"

typedef struct {
	WINDOW *mainwindow;
	WINDOW *content;
	WINDOW *user_list;
} gui_t;

void init_colors(void);
void add_info(core_t *c, WINDOW *content, const char *msg, time_t *t);
void add_private(core_t *c, WINDOW *content, const char *nick, const char *msg, time_t *t);
void add_msg(core_t *c, WINDOW *content, const char *nick, const char *msg, time_t *t, char bold);
void redraw(gui_t *g, core_t *c);
void add_content_line(core_t *c, WINDOW *content, int x, const char *msg);
void draw_gui(WINDOW *win, core_t *c);
WINDOW *create_window(int height, int width, int starty, int startx);
void destroy_window(WINDOW *local_win);

#endif // WINDOW_H
