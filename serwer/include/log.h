#ifndef LOG_H
#define LOG_H
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <stdio.h>
#include "sem.h"
#include "chat.h"

int logfile(const char *message, int *semid);

#endif // LOG_H
