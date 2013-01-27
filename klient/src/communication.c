#include "../include/communication.h"
#include <mqueue.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>

void quit(resources* res) {
	endwin();
	printf("Caught SIGINT: quitting...\n");
	kill(res->child_id1, SIGINT);
	kill(res->child_id2, SIGINT);
	mq_close(res->int_in_fd);
	mq_close(res->int_out_fd);
	mq_unlink("/internal_in");
	mq_unlink("/internal_out");
	int mid = msgget(res->queue_key, 0777);
	if(mid != -1) {
		msgctl(mid, IPC_RMID, NULL);
	}
	exit(0);
}
