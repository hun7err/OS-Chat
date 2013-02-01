#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include <time.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "defines.h"
#include "chat.h"

void gettime(char *buf, time_t *t);
int receive(int key, void *dest, int size, long type);
void get_key(int *key, char type);

#endif // COMMUNICATION_H
