#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "window.h"
#include "defines.h"
#include "chat.h"
#include <stdlib.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <mqueue.h>

void quit(resources* res);

typedef enum {
	M_USERLIST = 1,
	M_LEAVE,
	M_JOIN,
	M_REGISTER,
	M_MESSAGE,
	M_PRIVATE,
	M_ERROR
} mtype;

#define MAX_MSG_SIZE sizeof(message)+1

typedef struct {
	mtype type;
	struct {
		char name[MAX_USER_NAME_LENGTH];
		char list[MAX_USER_LIST_LENGTH][MAX_USER_NAME_LENGTH];
		char room[MAX_ROOM_NAME_LENGTH];
		char message[512];
	} content;
	int dest;
	int source;
} message;

#endif // COMMUNICATION_H
