#ifndef DEFINES_H
#define DEFINES_H

// nagłówki
#include <unistd.h>

typedef struct {
	int child_id1;
	int child_id2;
	int queue_key;
	int int_in_fd;
	int int_out_fd;
} resources;

resources res;

// stałe
#define KEY_ESCAPE 27
#define KEY_RETURN 10
#define DEFAULT_QUEUE_KEY 666

#define MAX_MSG_COUNT 16

// różne hacki
#define member_size(type, member) sizeof(((type *)0)->member)

#endif // DEFINES_H
