#ifndef DEFINES_H
#define DEFINES_H

// nagłówki
#include <unistd.h>

typedef struct {
	int child_id1;
	int child_id2;
	int child_id3;
	int queue_key;
	int int_in_fd;
	int int_out_fd;
	char q_in[128];
	char q_out[128];
} resources;

resources res;
int server_key[2];

// stałe
#define KEY_ESCAPE 27
#define KEY_RETURN 10
#define DEFAULT_QUEUE_KEY 666

#define MAX_MSG_COUNT 16

// różne hacki
#define member_size(type, member) sizeof(((type *)0)->member) // gdyby sie zdarzylo, ze jakims cudem ta funkcja wyciekla gdzies i ktos ma identyczna to ja mam do niej prawa przede wszystkim :[

#endif // DEFINES_H
