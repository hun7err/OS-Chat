#ifndef SIGNALS_H
#define SIGNALS_H

#include "defines.h"
#include "communication.h"
#include "chat.h"
#include "sem.h"
#include "log.h"
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

void sighandler(int signum);

#endif // SIGNALS_H
