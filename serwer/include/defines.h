#ifndef DEFINES_H
#define DEFINES_H

#define START_KEY 666
#define member_size(type,member) sizeof(((type *)0)->member)

#define SHM 0
#define SEM 1
#define MSG 2

#define E_NICKEX -1	// nick exists
#define E_FULL -2	// server full

typedef struct {
	int shm_key;
	int msg_key;
	int heartbeat_msg_key;
	int sem_key;
	int child;
} resource;

resource res;

#define TIME_LOG 0
#define TIME_CHAT 1

#endif // DEFINES_H
