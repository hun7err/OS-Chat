#include "../include/communication.h"
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>

void quit(resources* res) {
	endwin();
	printf("Caught SIGINT: quitting...\n");
	kill(res->child_id1, SIGINT);
	kill(res->child_id2, SIGINT);
	int mid = msgget(res->queue_key, 0777);
	if(mid != -1) {
		msgctl(mid, IPC_RMID, NULL);
	}
	exit(0);
}
