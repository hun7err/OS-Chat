#ifndef DEFINES_H
#define DEFINES_H

// nagłówki
#include <unistd.h>

typedef struct {
	int child_id1;
	int child_id2;
	int queue_key;
} resources;

resources res;

// stałe
#define KEY_ESCAPE 27
#define KEY_RETURN 10
#define DEFAULT_QUEUE_KEY 666

// różne hacki
#define member_size(type, member) sizeof(((type *)0)->member)

#endif // DEFINES_H
