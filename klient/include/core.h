#ifndef CORE_H
#define CORE_H

#include <string.h>
#include "../include/chat.h"

typedef struct {
	int cur_line;	// aktualna pozycja y na ekranie
	char nick[MAX_USER_NAME_LENGTH];
	char room[MAX_ROOM_NAME_LENGTH];
	char userlist[MAX_USER_COUNT_PER_SERVER * MAX_SERVER_COUNT][MAX_USER_NAME_LENGTH];
	int cur_user_pos;
	int cursor_x;
	int cursor_y;
} core_t;

void add_user_to_list(core_t *c, const char *name);

void quit();

// funkcje do komunikacji klient-klient i klient-serwer

#endif // CORE_H
